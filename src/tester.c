#include "psxcore_file.h"
#include "psxcore_hash.h"
#include "psxcore_vcd.h"
#include <ctype.h>
#include <debug.h>
#include <iopcontrol.h>
#include <kernel.h>
#include <libpad.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <sifrpc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>

#define MODULE(name) extern unsigned char name##_irx[] __attribute__((aligned(16))); extern unsigned int size_##name##_irx
MODULE(iomanX);
MODULE(fileXio);
MODULE(sio2man);
MODULE(freepad);
MODULE(bdm);
MODULE(bdmfs_fatfs);
MODULE(ps2dev9);
MODULE(ata_bd);
MODULE(usbd);
MODULE(usbmass_bd);

#define LIBRARY_PATH "mass0:/POPS"
#define BOOTSTRAP_PATH LIBRARY_PATH "/psxcore-runtime-bootstrap.elf"
#define GAME_LIMIT 128
#define PAGE_SIZE 7
#define SCREEN_COLUMNS 68
#define SCREEN_ROWS 24
#define TESTER_BASE 0x01000000u

typedef enum { BACKEND_ATA, BACKEND_USB } Backend;
typedef enum { CARDS_PRIVATE, CARDS_SHARED } CardMode;
typedef struct {
    char name[256];
    char path[256];
    uint64_t bytes;
    unsigned tracks;
    const char *issue;
} Game;
typedef struct {
    char pops[36];
    char iop[36];
    char card0[36];
    char card1[36];
    char bootstrap[36];
    int ready;
} Diagnostics;

static Game games[GAME_LIMIT];
static unsigned game_count;
static unsigned selected;
static int library_full;
static Backend backend;
static CardMode card_mode = CARDS_SHARED;
static int code_validation;
static Diagnostics diagnostics;
static char status_text[128];
static char screen[SCREEN_ROWS][SCREEN_COLUMNS];
static char previous_screen[SCREEN_ROWS][SCREEN_COLUMNS];
static uint32_t previous_colors[SCREEN_ROWS][SCREEN_COLUMNS];
static uint32_t previous_backgrounds[SCREEN_ROWS][SCREEN_COLUMNS];
static int screen_invalid = 1;
static int show_checks;
static unsigned matrix_phase;
static unsigned char pad_buffer[256] __attribute__((aligned(64)));
static unsigned previous_buttons;
static unsigned char card_buffer[2][NP_FILE_CARD_BYTES];

static size_t bounded_length(const char *text, size_t capacity) {
    size_t length = 0;
    while (length < capacity && text[length]) ++length;
    return length;
}

static uint16_t read_u16(const unsigned char *bytes) {
    return (uint16_t)bytes[0] | (uint16_t)bytes[1] << 8;
}

static uint32_t read_u32(const unsigned char *bytes) {
    return (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 |
           (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24;
}

static uint64_t file_size(const iox_stat_t *stat) {
    return ((uint64_t)stat->hisize << 32) | stat->size;
}

static int read_exact(int handle, void *buffer, unsigned length) {
    unsigned completed = 0;
    while (completed < length) {
        int count = fileXioRead(handle, (unsigned char *)buffer + completed,
                                length - completed);
        if (count <= 0 || (unsigned)count > length - completed) return 0;
        completed += (unsigned)count;
    }
    return 1;
}

static int write_exact(int handle, const void *buffer, unsigned length) {
    unsigned completed = 0;
    while (completed < length) {
        unsigned chunk = length - completed;
        if (chunk > 4096) chunk = 4096;
        int count = fileXioWrite(handle, (const unsigned char *)buffer + completed,
                                 chunk);
        if (count <= 0 || (unsigned)count > chunk) return 0;
        completed += (unsigned)count;
    }
    return 1;
}

static void card_paths(const Game *game, CardMode mode,
                       char paths[2][NP_FILE_PATH_BYTES]) {
    if (mode == CARDS_SHARED) {
        strcpy(paths[0], NP_FILE_CARD0);
        strcpy(paths[1], NP_FILE_CARD1);
        return;
    }
    NpSha256 hash;
    unsigned char digest[32];
    char key[65];
    char normalized[NP_FILE_PATH_BYTES];
    static const char hex[] = "0123456789abcdef";
    size_t length = strlen(game->path + 6);
    for (size_t index = 0; index < length; ++index)
        normalized[index] = (char)tolower((unsigned char)game->path[index + 6]);
    np_sha256_init(&hash);
    np_sha256_update(&hash, normalized, length);
    np_sha256_final(&hash, digest);
    for (unsigned index = 0; index < 32; ++index) {
        key[index * 2] = hex[digest[index] >> 4];
        key[index * 2 + 1] = hex[digest[index] & 15];
    }
    key[64] = 0;
    for (unsigned slot = 0; slot < 2; ++slot)
        snprintf(paths[slot], NP_FILE_PATH_BYTES, "/POPS/CARDS/%s-%u.vmc",
                 key, slot);
}

static void mounted_card_paths(const Game *game, CardMode mode,
                               char paths[2][NP_FILE_PATH_BYTES + 8]) {
    char relative[2][NP_FILE_PATH_BYTES];
    card_paths(game, mode, relative);
    for (unsigned slot = 0; slot < 2; ++slot)
        snprintf(paths[slot], NP_FILE_PATH_BYTES + 8, "mass0:%.255s", relative[slot]);
}

static int load_module(const char *name, unsigned char *bytes, unsigned size) {
    int result = -1;
    int module = SifExecModuleBuffer(bytes, size, 0, NULL, &result);
    if (module >= 0 && result == 0) return 1;
    snprintf(status_text, sizeof(status_text), "Module %s failed (%d/%d)",
             name, module, result);
    return 0;
}

#define LOAD(name) do { if (!load_module(#name, name##_irx, size_##name##_irx)) return 0; } while (0)

static int init_common(void) {
    SifInitRpc(0);
    unsigned attempts;
    for (attempts = 0; attempts < 300 && !SifIopReset("", 0); ++attempts)
        usleep(10000);
    if (attempts == 300) {
        snprintf(status_text, sizeof(status_text), "IOP reset could not start");
        return 0;
    }
    for (attempts = 0; attempts < 300 && !SifIopSync(); ++attempts)
        usleep(10000);
    if (attempts == 300) {
        snprintf(status_text, sizeof(status_text), "IOP reset timed out");
        return 0;
    }
    SifInitRpc(0);
    if (sbv_patch_enable_lmb() < 0 || sbv_patch_disable_prefix_check() < 0) {
        snprintf(status_text, sizeof(status_text), "IOP module loader unavailable");
        return 0;
    }
    LOAD(iomanX);
    LOAD(fileXio);
    LOAD(sio2man);
    LOAD(freepad);
    padInit(0);
    if (!padPortOpen(0, 0, pad_buffer)) {
        snprintf(status_text, sizeof(status_text), "Controller port unavailable");
        return 0;
    }
    return 1;
}

static unsigned pressed_buttons(void) {
    struct padButtonStatus buttons;
    int state = padGetState(0, 0);
    if (state != PAD_STATE_STABLE && state != PAD_STATE_FINDCTP1) {
        previous_buttons = 0;
        return 0;
    }
    if (!padRead(0, 0, &buttons)) return 0;
    unsigned held = 0xffffu ^ buttons.btns;
    unsigned pressed = held & ~previous_buttons;
    previous_buttons = held;
    return pressed;
}

static Backend choose_backend(void) {
    scr_clear();
    scr_printf("PSXCore Tester\n\nSelect the game storage device:\n\n");
    scr_printf("Cross: internal ATA drive\nSquare: USB drive\n\n");
    scr_printf("The selected device must appear as mass0:.\n");
    for (;;) {
        unsigned buttons = pressed_buttons();
        if (buttons & PAD_CROSS) return BACKEND_ATA;
        if (buttons & PAD_SQUARE) return BACKEND_USB;
        usleep(16000);
    }
}

static int init_storage(void) {
    LOAD(bdm);
    LOAD(bdmfs_fatfs);
    if (backend == BACKEND_ATA) {
        LOAD(ps2dev9);
        LOAD(ata_bd);
    } else {
        LOAD(usbd);
        LOAD(usbmass_bd);
    }
    if (fileXioInit() < 0) {
        snprintf(status_text, sizeof(status_text), "Storage service unavailable");
        return 0;
    }
    for (unsigned attempts = 0; attempts < 50; ++attempts) {
        int directory = fileXioDopen("mass0:/");
        if (directory >= 0) {
            fileXioDclose(directory);
            return 1;
        }
        usleep(100000);
    }
    snprintf(status_text, sizeof(status_text), "No mass0: volume found");
    return 0;
}

static int name_compare(const void *left, const void *right) {
    const unsigned char *first = (const unsigned char *)((const Game *)left)->name;
    const unsigned char *second = (const unsigned char *)((const Game *)right)->name;
    while (*first && *second) {
        int difference = tolower(*first) - tolower(*second);
        if (difference) return difference;
        ++first;
        ++second;
    }
    return tolower(*first) - tolower(*second);
}

static void inspect_game(Game *game) {
    iox_stat_t stat;
    if (fileXioGetStat(game->path, &stat) < 0 || !FIO_S_ISREG(stat.mode)) {
        game->issue = "missing or unreadable";
        game->tracks = 0;
        return;
    }
    game->bytes = file_size(&stat);
    game->tracks = 0;
    unsigned char header[NP_VCD_HEADER_BYTES];
    int handle = fileXioOpen(game->path, FIO_O_RDONLY, 0);
    if (handle < 0) {
        game->issue = "open failed";
        return;
    }
    int complete = read_exact(handle, header, sizeof(header));
    int closed = fileXioClose(handle);
    if (!complete || closed < 0) {
        game->issue = "header read failed";
        return;
    }
    NpVcdInfo info;
    game->issue = np_vcd_check(header, sizeof(header), game->bytes, &info);
    if (!game->issue) game->tracks = info.tracks;
}

static void scan_library(void) {
    game_count = 0;
    selected = 0;
    library_full = 0;
    int directory = fileXioDopen(LIBRARY_PATH);
    if (directory < 0) {
        snprintf(status_text, sizeof(status_text), "Cannot open %s", LIBRARY_PATH);
        memset(&diagnostics, 0, sizeof(diagnostics));
        return;
    }
    iox_dirent_t entry;
    int result;
    while ((result = fileXioDread(directory, &entry)) > 0) {
        size_t length = bounded_length(entry.name, sizeof(entry.name));
        if (length == sizeof(entry.name) || !FIO_S_ISREG(entry.stat.mode) ||
            !np_vcd_filename(entry.name)) continue;
        if (game_count == GAME_LIMIT) {
            library_full = 1;
            continue;
        }
        Game *game = &games[game_count];
        int written = snprintf(game->path, sizeof(game->path), "%s/%s",
                               LIBRARY_PATH, entry.name);
        if (written < 0 || (size_t)written >= sizeof(game->path)) continue;
        memcpy(game->name, entry.name, length - 4);
        game->name[length - 4] = 0;
        game->bytes = file_size(&entry.stat);
        game->tracks = 0;
        game->issue = NULL;
        inspect_game(game);
        ++game_count;
    }
    int closed = fileXioDclose(directory);
    qsort(games, game_count, sizeof(games[0]), name_compare);
    if (result < 0 || closed < 0)
        snprintf(status_text, sizeof(status_text), "Library read ended with an error");
    else if (library_full)
        snprintf(status_text, sizeof(status_text), "Showing first %u VCDs", GAME_LIMIT);
    else
        snprintf(status_text, sizeof(status_text), "%u VCD files found", game_count);
}

static int inspect_asset(const char *path, uint64_t expected_bytes, int kind,
                         char result[36]) {
    iox_stat_t stat;
    if (fileXioGetStat(path, &stat) < 0 || !FIO_S_ISREG(stat.mode)) {
        snprintf(result, 36, "missing");
        return 0;
    }
    if (file_size(&stat) != expected_bytes) {
        snprintf(result, 36, "wrong size");
        return 0;
    }
    unsigned char header[128];
    int handle = fileXioOpen(path, FIO_O_RDONLY, 0);
    if (handle < 0) {
        snprintf(result, 36, "unreadable");
        return 0;
    }
    int complete = read_exact(handle, header, sizeof(header));
    int closed = fileXioClose(handle);
    if (!complete || closed < 0) {
        snprintf(result, 36, "read failed");
        return 0;
    }
    if (kind == 1 && (memcmp(header, "\177ELF", 4) || header[4] != 1 ||
                      header[5] != 1 || read_u16(header + 18) != 8)) {
        snprintf(result, 36, "ELF header invalid");
        return 0;
    }
    if (kind == 2 && !np_file_card_header_valid(header, sizeof(header))) {
        snprintf(result, 36, "card header invalid");
        return 0;
    }
    snprintf(result, 36, "present; core checks at launch");
    return 1;
}

static int card_path_unused(const char *path) {
    static const char *const suffixes[] = {"", ".bak", ".new", ".prev"};
    iox_stat_t stat;
    char candidate[NP_FILE_PATH_BYTES + 16];
    for (unsigned index = 0; index < 4; ++index) {
        int written = snprintf(candidate, sizeof(candidate), "%s%s", path,
                               suffixes[index]);
        if (written < 0 || (size_t)written >= sizeof(candidate)) return 0;
        int result = fileXioGetStat(candidate, &stat);
        if (result != -2 && result != -4) return 0;
    }
    return 1;
}

static int card_source_stable(const char *path) {
    iox_stat_t stat;
    char candidate[NP_FILE_PATH_BYTES + 16];
    static const char *const suffixes[] = {".bak", ".new"};
    for (unsigned index = 0; index < 2; ++index) {
        int written = snprintf(candidate, sizeof(candidate), "%s%s", path,
                               suffixes[index]);
        if (written < 0 || (size_t)written >= sizeof(candidate)) return 0;
        int result = fileXioGetStat(candidate, &stat);
        if (result != -2 && result != -4) return 0;
    }
    return 1;
}

static int card_directory_ready(void) {
    const char *path = card_mode == CARDS_SHARED ?
                       LIBRARY_PATH "/ps1emu" : LIBRARY_PATH "/CARDS";
    iox_stat_t stat;
    int result = fileXioGetStat(path, &stat);
    if (result == -2 || result == -4) {
        if (fileXioMkdir(path, 0777) < 0) return 0;
        result = fileXioGetStat(path, &stat);
    }
    return result >= 0 && FIO_S_ISDIR(stat.mode);
}

static int card_read(const char *path, unsigned slot) {
    char result[36];
    if (!inspect_asset(path, NP_FILE_CARD_BYTES, 2, result)) return 0;
    int handle = fileXioOpen(path, FIO_O_RDONLY, 0);
    if (handle < 0) return 0;
    int complete = 1;
    for (unsigned offset = 0; offset < NP_FILE_CARD_BYTES; offset += 4096) {
        if (!read_exact(handle, card_buffer[slot] + offset, 4096)) {
            complete = 0;
            break;
        }
    }
    int closed = fileXioClose(handle);
    return complete && closed >= 0;
}

static int card_write_new(const char *path, unsigned slot) {
    int handle = fileXioOpen(path, FIO_O_WRONLY | FIO_O_CREAT | FIO_O_EXCL, 0666);
    if (handle < 0) return 0;
    int complete = fileXioLseek64(handle, 0, SEEK_END) == 0 &&
                   write_exact(handle, card_buffer[slot], NP_FILE_CARD_BYTES);
    int closed = fileXioClose(handle);
    if (!complete || closed < 0) return 0;
    handle = fileXioOpen(path, FIO_O_RDONLY, 0);
    if (handle < 0) return 0;
    unsigned char chunk[4096];
    for (unsigned offset = 0; offset < NP_FILE_CARD_BYTES; offset += sizeof(chunk)) {
        if (!read_exact(handle, chunk, sizeof(chunk)) ||
            memcmp(chunk, card_buffer[slot] + offset, sizeof(chunk))) {
            complete = 0;
            break;
        }
    }
    closed = fileXioClose(handle);
    return complete && closed >= 0;
}

static int ensure_selected_cards(void) {
    char target[2][NP_FILE_PATH_BYTES + 8];
    unsigned missing = 0;
    mounted_card_paths(&games[selected], card_mode, target);
    /* FatFs reports FR_NO_PATH for children of a missing directory. Make the
       directory available before checking whether individual cards exist. */
    if (!card_directory_ready()) {
        snprintf(status_text, sizeof(status_text), "Card directory unavailable");
        return 0;
    }
    /* Preflight both slots before writing either: existing saves and recovery
       files always take precedence over automatic blank-card creation. */
    for (unsigned slot = 0; slot < 2; ++slot) {
        iox_stat_t stat;
        int result = fileXioGetStat(target[slot], &stat);
        if (result == -2 || result == -4) {
            if (!card_path_unused(target[slot])) {
                snprintf(status_text, sizeof(status_text),
                         "Card %u needs recovery; existing files preserved", slot);
                return 0;
            }
            missing |= 1u << slot;
        } else {
            char state[36];
            if (result < 0 || !inspect_asset(target[slot], NP_FILE_CARD_BYTES,
                                             2, state)) {
                snprintf(status_text, sizeof(status_text),
                         "Card %u unavailable or invalid; nothing replaced", slot);
                return 0;
            }
        }
    }
    if (!missing) return 1;
    for (unsigned slot = 0; slot < 2; ++slot) {
        if (!(missing & (1u << slot))) continue;
        for (unsigned frame = 0; frame < NP_FILE_CARD_BYTES / 128; ++frame)
            np_file_card_format_frame(frame, card_buffer[slot] + frame * 128);
        if (!card_path_unused(target[slot]) || !card_write_new(target[slot], slot)) {
            snprintf(status_text, sizeof(status_text),
                     "Card %u create/verify failed; existing files preserved", slot);
            return 0;
        }
    }
    snprintf(status_text, sizeof(status_text), "Missing %s cards created and verified",
             card_mode == CARDS_SHARED ? "shared" : "per-game");
    return 1;
}

static void card_operation(int copy) {
    char target[2][NP_FILE_PATH_BYTES + 8];
    char source[2][NP_FILE_PATH_BYTES + 8];
    mounted_card_paths(&games[selected], card_mode, target);
    mounted_card_paths(&games[selected], card_mode == CARDS_SHARED ?
                       CARDS_PRIVATE : CARDS_SHARED, source);
    for (unsigned slot = 0; slot < 2; ++slot) {
        if (!card_path_unused(target[slot])) {
            snprintf(status_text, sizeof(status_text),
                     "Card %u exists or has recovery files; nothing changed", slot);
            return;
        }
        if (copy) {
            char result[36];
            if (!card_source_stable(source[slot]) ||
                !inspect_asset(source[slot], NP_FILE_CARD_BYTES, 2, result)) {
                snprintf(status_text, sizeof(status_text),
                         "Source card %u invalid or needs recovery", slot);
                return;
            }
        }
    }
    for (unsigned slot = 0; slot < 2; ++slot) {
        if (copy) {
            if (!card_read(source[slot], slot)) {
                snprintf(status_text, sizeof(status_text),
                         "Source card %u changed or unreadable", slot);
                return;
            }
        } else {
            for (unsigned frame = 0; frame < NP_FILE_CARD_BYTES / 128; ++frame)
                np_file_card_format_frame(frame, card_buffer[slot] + frame * 128);
        }
    }
    if (!card_directory_ready()) {
        snprintf(status_text, sizeof(status_text), "Card directory unavailable");
        return;
    }
    for (unsigned slot = 0; slot < 2; ++slot) {
        if (!card_path_unused(target[slot]) || !card_write_new(target[slot], slot)) {
            snprintf(status_text, sizeof(status_text),
                     "Card %u write/verify failed; inspect before retry", slot);
            return;
        }
    }
    snprintf(status_text, sizeof(status_text), "%s %s card pair verified",
             copy ? "Copied" : "Created",
             card_mode == CARDS_SHARED ? "shared" : "private");
}

static void card_manager(void) {
    if (!game_count) return;
    for (;;) {
        char target[2][NP_FILE_PATH_BYTES + 8];
        char source[2][NP_FILE_PATH_BYTES + 8];
        char state[2][36];
        mounted_card_paths(&games[selected], card_mode, target);
        mounted_card_paths(&games[selected], card_mode == CARDS_SHARED ?
                           CARDS_PRIVATE : CARDS_SHARED, source);
        for (unsigned slot = 0; slot < 2; ++slot)
            inspect_asset(target[slot], NP_FILE_CARD_BYTES, 2, state[slot]);
        scr_clear();
        scr_printf("PSXCORE TESTER - CARDS\n\n%s\nMode: %s\n\n",
                   games[selected].name,
                   card_mode == CARDS_SHARED ? "Shared pair" : "Per-game pair");
        scr_printf("Card 0: %s\nCard 1: %s\n\n", state[0], state[1]);
        scr_printf("X: create a blank pair if absent\n");
        scr_printf("Square: copy the %s pair if absent\n",
                   card_mode == CARDS_SHARED ? "per-game" : "shared");
        scr_printf("Circle: return\n\nNo existing card is overwritten.\n");
        scr_printf("%s\n", status_text);
        unsigned buttons;
        do { buttons = pressed_buttons(); usleep(16000); } while (!buttons);
        if (buttons & PAD_CIRCLE) return;
        int copy = (buttons & PAD_SQUARE) != 0;
        if (!(buttons & (PAD_CROSS | PAD_SQUARE))) continue;
        scr_clear();
        scr_printf("%s %s cards for:\n%s\n\n",
                   copy ? "Copy into" : "Create blank",
                   card_mode == CARDS_SHARED ? "shared" : "per-game",
                   games[selected].name);
        scr_printf("Targets:\n%s\n%s\n\n", target[0], target[1]);
        if (copy) scr_printf("Source:\n%s\n%s\n\n", source[0], source[1]);
        scr_printf("Existing files will never be replaced.\n");
        scr_printf("Cross: confirm    Circle: cancel\n");
        for (;;) {
            buttons = pressed_buttons();
            if (buttons & PAD_CIRCLE) break;
            if (buttons & PAD_CROSS) {
                card_operation(copy);
                break;
            }
            usleep(16000);
        }
    }
}

static int inspect_bootstrap(char result[36]) {
    iox_stat_t stat;
    if (fileXioGetStat(BOOTSTRAP_PATH, &stat) < 0 || !FIO_S_ISREG(stat.mode)) {
        snprintf(result, 36, "missing");
        return 0;
    }
    uint64_t bytes = file_size(&stat);
    if (bytes < 52 || bytes > 16 * 1024 * 1024) {
        snprintf(result, 36, "ELF size invalid");
        return 0;
    }
    int handle = fileXioOpen(BOOTSTRAP_PATH, FIO_O_RDONLY, 0);
    if (handle < 0) {
        snprintf(result, 36, "unreadable");
        return 0;
    }
    unsigned char header[52];
    int valid = read_exact(handle, header, sizeof(header));
    if (valid) {
        uint32_t headers_offset = read_u32(header + 28);
        unsigned header_count = read_u16(header + 44);
        uint32_t entry = read_u32(header + 24);
        valid = !memcmp(header, "\177ELF", 4) && header[4] == 1 &&
                header[5] == 1 && read_u16(header + 18) == 8 &&
                read_u16(header + 42) == 32 && header_count > 0 &&
                header_count <= 32 && headers_offset >= sizeof(header) &&
                (uint64_t)headers_offset + (uint64_t)header_count * 32 <= bytes &&
                entry < TESTER_BASE;
        unsigned load_count = 0;
        int entry_mapped = 0;
        for (unsigned index = 0; valid && index < header_count; ++index) {
            unsigned char segment[32];
            uint32_t offset = headers_offset + index * 32;
            valid = fileXioLseek64(handle, offset, SEEK_SET) == offset &&
                    read_exact(handle, segment, sizeof(segment));
            if (!valid || read_u32(segment) != 1) continue;
            uint32_t file_offset = read_u32(segment + 4);
            uint32_t address = read_u32(segment + 8);
            uint32_t physical = read_u32(segment + 12);
            uint32_t file_bytes = read_u32(segment + 16);
            uint32_t memory_bytes = read_u32(segment + 20);
            valid = address >= 0x000f0000u && address < TESTER_BASE &&
                    physical == address && memory_bytes > 0 &&
                    memory_bytes <= TESTER_BASE - address &&
                    file_bytes <= memory_bytes && file_offset <= bytes &&
                    file_bytes <= bytes - file_offset;
            if (valid && entry >= address && entry - address < file_bytes)
                entry_mapped = 1;
            ++load_count;
        }
        valid = valid && load_count > 0 && entry_mapped;
    }
    if (fileXioClose(handle) < 0) valid = 0;
    snprintf(result, 36, "%s", valid ? "ELF layout OK" : "ELF layout invalid");
    return valid;
}

static void refresh_diagnostics(void) {
    if (game_count) inspect_game(&games[selected]);
    int valid = game_count > 0 && !games[selected].issue;
    if (valid) valid &= ensure_selected_cards();
    char cards[2][NP_FILE_PATH_BYTES + 8];
    if (game_count) mounted_card_paths(&games[selected], card_mode, cards);
    valid &= inspect_asset(LIBRARY_PATH "/POPS.ELF", 3166988, 1,
                           diagnostics.pops);
    valid &= inspect_asset(LIBRARY_PATH "/IOPRP252.IMG", 265233, 0,
                           diagnostics.iop);
    if (game_count) {
        valid &= inspect_asset(cards[0], NP_FILE_CARD_BYTES, 2, diagnostics.card0);
        valid &= inspect_asset(cards[1], NP_FILE_CARD_BYTES, 2, diagnostics.card1);
    } else {
        strcpy(diagnostics.card0, "no game selected");
        strcpy(diagnostics.card1, "no game selected");
    }
    valid &= inspect_bootstrap(diagnostics.bootstrap);
    diagnostics.ready = valid;
}

static void put_text(unsigned row, unsigned column, const char *text) {
    if (row >= SCREEN_ROWS || column >= SCREEN_COLUMNS || !text) return;
    while (*text && column < SCREEN_COLUMNS) {
        unsigned char character = (unsigned char)*text++;
        screen[row][column++] = character >= 32 && character < 127 ? character : '?';
    }
}

static void put_label(unsigned row, const char *name, const char *value) {
    char line[69];
    snprintf(line, sizeof(line), "%s: %s", name, value);
    put_text(row, 0, line);
}

static void put_clipped(unsigned row, unsigned column, unsigned width,
                        const char *text) {
    if (row >= SCREEN_ROWS || column >= SCREEN_COLUMNS || !text) return;
    if (width > SCREEN_COLUMNS - column) width = SCREEN_COLUMNS - column;
    size_t length = strlen(text);
    for (unsigned index = 0; index < width && index < length; ++index) {
        unsigned char ch = (unsigned char)text[index];
        screen[row][column + index] = ch >= 32 && ch < 127 ? ch : '?';
    }
    if (length > width && width >= 3)
        memset(&screen[row][column + width - 3], '.', 3);
}

/* Word-wrap the selected title without splitting ordinary words. */
static void put_title(unsigned row, unsigned column, unsigned width,
                      unsigned lines, const char *title) {
    while (*title && lines) {
        size_t count = strlen(title);
        if (count > width) {
            count = width;
            for (unsigned i = width; i > 0; --i) {
                if (title[i] == ' ') { count = i; break; }
            }
        }
        if (lines == 1) {
            put_clipped(row, column, width, title);
            break;
        }
        char part[SCREEN_COLUMNS + 1];
        memcpy(part, title, count);
        part[count] = 0;
        put_text(row++, column, part);
        title += count;
        while (*title == ' ') ++title;
        --lines;
    }
}

/* Decorative code streams stay clear of names and controller hints. */
static int matrix_cell(unsigned row, unsigned column) {
    return row == 2 || (row == 20 && (show_checks || column < 40)) ||
           (!show_checks && column == 38 && row >= 5 && row <= 19);
}

static void put_matrix(void) {
    static const char glyphs[] = "01:/01+01.101";
    for (unsigned row = 0; row < SCREEN_ROWS; ++row) {
        for (unsigned column = 0; column < SCREEN_COLUMNS; ++column) {
            if (!matrix_cell(row, column)) continue;
            unsigned trail = (row + SCREEN_ROWS - matrix_phase % SCREEN_ROWS) % SCREEN_ROWS;
            uint32_t code = (trail + 1u) * 0x45d9f3bu ^ (column + 1u) * 0x27d4eb2du;
            code ^= code >> 16;
            if (code % 5u < 3u)
                screen[row][column] = glyphs[(code / 5u) % (sizeof(glyphs) - 1)];
        }
    }
}

/* PS2 debug colors use BGR byte order. */
static void cell_style(unsigned row, unsigned column,
                       uint32_t *foreground, uint32_t *background) {
    *foreground = 0xb6e9bf;
    *background = 0x040805;
    if (row < 2) *background = 0x08160a;
    if (row == 0) *foreground = 0xa0ff89;
    if (row == 1 || row == 19 || row == 21 || row >= 22)
        *foreground = 0x6a9d72;
    if (row == 3) {
        *foreground = 0x6a9d72;
        if ((!show_checks && column < 15) || (show_checks && column >= 17 && column < 36)) {
            *background = 0x184522;
            *foreground = 0xbaffaf;
        }
    }
    if (!show_checks && column >= 40 && row >= 5 && row <= 20) {
        *background = 0x0a1b0e;
        if (row >= 11) *foreground = 0x8fc499;
        if (row == 14 || row == 17) *foreground = 0x71ff57;
        if (row == 18 || row == 19) *foreground = 0xb6e9bf;
    }
    if (!show_checks && game_count && column < 37 &&
        row >= 5 + 2 * (selected % PAGE_SIZE) &&
        row <= 6 + 2 * (selected % PAGE_SIZE)) {
        *background = column == 0 ? 0x71ff57 : 0x20532b;
        *foreground = 0xd6ffe2;
    }
    if ((show_checks && row == 16) ||
        (!show_checks && game_count && row == 9 && column >= 42)) {
        *foreground = diagnostics.ready ? 0x71ff57 : 0x7ebfff;
    }
    if (row == 21) *background = 0x08160a;
    if (row == 22 && column == 32) *foreground = 0x71ff57;
    if (row == 23 && column >= 2 && column < 5) *foreground = 0x71ff57;
    if (row == 23 && column >= (show_checks ? 31u : 19u) &&
        column < (show_checks ? 33u : 21u)) *foreground = 0xa0ff89;
    if (row == 23 && column == (show_checks ? 19u : 38u)) *foreground = 0x7ebfff;
    if (matrix_cell(row, column)) {
        unsigned head = (matrix_phase + column * 3u) % SCREEN_ROWS;
        *foreground = row == head ? 0x71ff57 : 0x24532b;
    }
}

static void draw_screen(void) {
    memset(screen, ' ', sizeof(screen));
    put_matrix();
    put_text(0, 2, "[>]  PSXCORE");
    put_text(0, 49, "TESTER // PSX");
    put_text(1, 7, "PLAYSTATION / TEST TERMINAL");
    put_text(1, 49, backend == BACKEND_ATA ? "INTERNAL DRIVE" : "USB DRIVE");
    put_text(3, 2, " GAME INDEX  ");
    put_text(3, 18, "  SYSTEM CHECKS  ");
    unsigned page = selected / PAGE_SIZE;
    if (show_checks) {
        if (game_count) {
            put_clipped(5, 2, 64, games[selected].name);
            put_label(7, "Game image", games[selected].issue ?
                      games[selected].issue : "structure OK");
        } else put_text(5, 2, "No game selected");
        put_label(9, "POPS", diagnostics.pops);
        put_label(10, "IOP", diagnostics.iop);
        put_label(11, "Bootstrap", diagnostics.bootstrap);
        put_label(13, "Card 0", diagnostics.card0);
        put_label(14, "Card 1", diagnostics.card1);
        put_text(15, 2, code_validation ? "Code Validation: ON  (R2 toggles)" :
                 "Code Validation: OFF (R2 toggles)");
        put_text(16, 2, diagnostics.ready ? "READY TO LAUNCH" :
                 "NEEDS ATTENTION - review the checks above");
        put_text(17, 2, "(helps with nonboot/blackscreen)");
        if (game_count) put_clipped(18, 2, 64, games[selected].path);
    } else {
        for (unsigned row = 0; row < PAGE_SIZE; ++row) {
            unsigned index = page * PAGE_SIZE + row;
            if (index >= game_count) break;
            put_text(row * 2 + 5, 2, index == selected ? "$" : " ");
            put_clipped(row * 2 + 5, 4, 32, games[index].name);
        }
        if (!game_count) {
            put_text(7, 2, "Your library is empty");
            put_text(9, 2, "Add VCD files to mass0:/POPS");
            put_text(11, 2, "SELECT  Scan for games");
            put_text(5, 42, "Choose a game to begin");
            put_text(9, 42, "NO GAME SELECTED");
        } else {
            Game *game = &games[selected];
            put_title(5, 42, 24, 3, game->name);
            put_text(9, 42, diagnostics.ready ? "READY TO PLAY   [ X ]" : "NEEDS ATTENTION  [ TRI ]");
            char detail[32];
            snprintf(detail, sizeof(detail), "%llu MiB  /  %u tracks",
                     (unsigned long long)(game->bytes / 1048576), game->tracks);
            put_clipped(12, 42, 24, detail);
        }
        put_text(11, 42, backend == BACKEND_ATA ? "STORAGE  Internal drive" : "STORAGE  USB drive");
        put_text(14, 42, "MEMORY CARDS");
        put_text(15, 42, card_mode == CARDS_SHARED ? "Shared between games" : "Separate for each game");
        put_text(17, 42, "OPTIONAL FIXES");
        put_text(18, 42, code_validation ? "Code Validation [R2] ON" :
                 "Code Validation [R2] OFF");
        put_text(19, 42, "(helps with");
        put_text(20, 42, "nonboot/blackscreen)");
        char count[64];
        snprintf(count, sizeof(count), "%u games  //  Page %u of %u", game_count,
                 game_count ? page + 1 : 0,
                 game_count ? (game_count + PAGE_SIZE - 1) / PAGE_SIZE : 0);
        put_clipped(19, 2, 35, count);
    }
    put_text(21, 0, ">");
    put_clipped(21, 2, 64, status_text);
    put_text(22, 2, "UP/DOWN Select  L1/R1 Page  X Launch  SELECT Rescan  R2 Validate");
    put_text(23, 2, show_checks ? "TRI Library      O Back      SQ Card mode" :
             "TRI Checks       SQ Card mode       O Manage cards");
    if (screen_invalid) {
        scr_setbgcolor(0x040805);
        scr_clear();
    }
    for (unsigned row = 0; row < SCREEN_ROWS; ++row) {
        for (unsigned column = 0; column < SCREEN_COLUMNS; ++column) {
            unsigned char character = (unsigned char)screen[row][column];
            uint32_t color, background;
            cell_style(row, column, &color, &background);
            if (screen_invalid || previous_screen[row][column] != (char)character ||
                previous_colors[row][column] != color ||
                previous_backgrounds[row][column] != background) {
                scr_setbgcolor(background);
                scr_putchar((column + 6) * 8, (row + 2) * 8, color, character);
            }
            previous_screen[row][column] = (char)character;
            previous_colors[row][column] = color;
            previous_backgrounds[row][column] = background;
        }
    }
    /* Storage and card dialogs share the terminal palette. */
    scr_setbgcolor(0x040805);
    screen_invalid = 0;
}

static void launch_selected(void) {
    if (!game_count) return;
    refresh_diagnostics();
    if (!diagnostics.ready) {
        snprintf(status_text, sizeof(status_text), "Launch blocked; inspect the checks");
        return;
    }
    snprintf(status_text, sizeof(status_text), "Loading PSXCore bootstrap...");
    draw_screen();
    if (SifLoadFileInit() < 0) {
        snprintf(status_text, sizeof(status_text), "ELF loader unavailable");
        return;
    }
    t_ExecData executable = {0};
    int result = SifLoadElf(BOOTSTRAP_PATH, &executable);
    SifLoadFileExit();
    if (result < 0 || !executable.epc) {
        snprintf(status_text, sizeof(status_text), "Bootstrap load failed (%d)", result);
        return;
    }
    char cards[2][NP_FILE_PATH_BYTES];
    card_paths(&games[selected], card_mode, cards);
    char *arguments[] = {BOOTSTRAP_PATH, "--file", games[selected].path,
                         "--backend", backend == BACKEND_ATA ? "ata" : "usb",
                         "--card0", cards[0], "--card1", cards[1],
                         code_validation ? "--code-validation" : "--no-code-validation"
                         };
    padPortClose(0, 0);
    padEnd();
    FlushCache(0);
    FlushCache(2);
    _ExecPS2((void *)(uintptr_t)executable.epc,
             (void *)(uintptr_t)executable.gp,
             sizeof(arguments) / sizeof(arguments[0]), arguments);
    snprintf(status_text, sizeof(status_text), "Bootstrap returned unexpectedly");
}

int main(void) {
    init_scr();
    scr_setbgcolor(0x040805);
    scr_setfontcolor(0xb6e9bf);
    scr_setCursor(0);
    snprintf(status_text, sizeof(status_text), "Initializing PS2 services...");
    if (!init_common()) goto failed;
    backend = choose_backend();
    scr_clear();
    scr_printf("Loading %s storage...\n", backend == BACKEND_ATA ? "ATA" : "USB");
    if (!init_storage()) goto failed;
    scan_library();
    refresh_diagnostics();
    draw_screen();
    unsigned animation_frames = 0;
    for (;;) {
        unsigned buttons = pressed_buttons();
        int redraw = 0;
        if (buttons & PAD_UP && game_count) {
            selected = selected ? selected - 1 : game_count - 1;
            redraw = 1;
        } else if (buttons & PAD_DOWN && game_count) {
            selected = (selected + 1) % game_count;
            redraw = 1;
        } else if (buttons & PAD_L1 && game_count) {
            selected = selected >= PAGE_SIZE ? selected - PAGE_SIZE : 0;
            redraw = 1;
        } else if (buttons & PAD_R1 && game_count) {
            selected += PAGE_SIZE;
            if (selected >= game_count) selected = game_count - 1;
            redraw = 1;
        } else if (buttons & PAD_R2) {
            code_validation = !code_validation;
            snprintf(status_text, sizeof(status_text), "Code Validation %s; applies to the next launch",
                     code_validation ? "ON" : "OFF");
            redraw = 1;
        } else if (buttons & PAD_TRIANGLE) {
            show_checks = !show_checks;
            redraw = 1;
        } else if (buttons & PAD_SELECT) {
            scan_library();
            redraw = 1;
        } else if (buttons & PAD_SQUARE) {
            card_mode = card_mode == CARDS_SHARED ? CARDS_PRIVATE : CARDS_SHARED;
            snprintf(status_text, sizeof(status_text), "%s cards selected",
                     card_mode == CARDS_SHARED ? "Shared" : "Per-game");
            redraw = 1;
        } else if (buttons & PAD_CIRCLE) {
            if (show_checks) show_checks = 0;
            else {
                card_manager();
                screen_invalid = 1;
            }
            redraw = 1;
        } else if (buttons & PAD_CROSS) {
            launch_selected();
            redraw = 1;
        }
        if (redraw) {
            refresh_diagnostics();
            draw_screen();
        }
        /* Animate only decorative cells; never rescan or touch saves on a tick. */
        if (++animation_frames >= 16) {
            animation_frames = 0;
            ++matrix_phase;
            draw_screen();
        }
        usleep(16000);
    }
failed:
    scr_clear();
    scr_printf("PSXCore Tester stopped\n\n%s\n", status_text);
    sleep(10);
    return 1;
}
