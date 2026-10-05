/* Original PSXCore code, Danny Nunez (dnunezx), 2026. No POPS execution. */
#include "psxcore_vcd.h"
#include <debug.h>
#include <kernel.h>
#include <iopcontrol.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <sifrpc.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <hdd-ioctl.h>
#include <io_common.h>

#define MODULE(name) extern unsigned char name##_irx[]; extern unsigned int size_##name##_irx
MODULE(iomanX); MODULE(fileXio); MODULE(ps2dev9); MODULE(bdm);
MODULE(ata_bd); MODULE(ps2hdd_bdm); MODULE(ps2fs);
MODULE(psxcore_ro);
#define LOG(...) do { printf("PSXCORE " __VA_ARGS__); scr_printf(__VA_ARGS__); } while (0)
#define LOAD(name, args, length) do { \
    int module_result = -1; \
    int module_id = SifExecModuleBuffer(name##_irx, size_##name##_irx, length, args, &module_result); \
    printf("PSXCORE module=%s id=%d result=%d\n", #name, module_id, module_result); \
    if (module_id < 0 || module_result < 0) return -1; \
} while (0)

static int init_storage(void) {
    unsigned tries;
    static const char hdd_args[] = "-o\0" "4\0" "-n\0" "20";
    static const char pfs_args[] = "-m\0" "3\0" "-o\0" "8\0" "-n\0" "40";
    SifInitRpc(0);
    for (tries = 0; tries < 300 && !SifIopReset("", 0); ++tries) usleep(10000);
    if (tries == 300) return -1;
    for (tries = 0; tries < 300 && !SifIopSync(); ++tries) usleep(10000);
    if (tries == 300) return -1;
    SifInitRpc(0);
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();
    LOAD(iomanX, NULL, 0); LOAD(fileXio, NULL, 0);
    LOAD(ps2dev9, NULL, 0); LOAD(bdm, NULL, 0);
    LOAD(psxcore_ro, NULL, 0); LOAD(ata_bd, NULL, 0);
    LOAD(ps2hdd_bdm, hdd_args, sizeof(hdd_args));
    LOAD(ps2fs, pfs_args, sizeof(pfs_args));
    fileXioInit();
    return 0;
}

static void prerequisite(const char *name, unsigned expected_size, int elf) {
    char path[128]; unsigned char h[52]; iox_stat_t st;
    int fd, count;
    snprintf(path, sizeof(path), "pfs1:/POPS/%s", name);
    if (fileXioGetStat(path, &st) < 0) { LOG("runtime=%s status=missing\n", name); return; }
    fd = fileXioOpen(path, FIO_O_RDONLY, 0);
    if (fd < 0) { LOG("runtime=%s status=unreadable\n", name); return; }
    count = fileXioRead(fd, h, sizeof(h)); fileXioClose(fd);
    if (st.hisize || st.size != expected_size || count != sizeof(h) ||
        (elf && (memcmp(h, "\177ELF", 4) || h[4] != 1 || h[5] != 1 || h[18] != 8 || h[19])))
        LOG("runtime=%s status=invalid\n", name);
    else LOG("runtime=%s status=present-unverified\n", name);
}

static int inventory(void) {
    unsigned valid = 0, invalid = 0, ignored = 0;
    int dir, status; iox_dirent_t entry;
    unsigned char header[NP_VCD_HEADER_BYTES];
    dir = fileXioDopen("pfs2:/");
    if (dir < 0) { LOG("error=library-open code=%d\n", dir); return -1; }
    while ((status = fileXioDread(dir, &entry)) > 0) {
        char path[320]; const char *reason; int fd, got; NpVcdInfo info;
        if (!FIO_S_ISREG(entry.stat.mode) || !np_vcd_filename(entry.name)) { ++ignored; continue; }
        snprintf(path, sizeof(path), "pfs2:/%s", entry.name);
        fd = fileXioOpen(path, FIO_O_RDONLY, 0);
        got = fd < 0 ? -1 : fileXioRead(fd, header, sizeof(header));
        if (fd >= 0) fileXioClose(fd);
        reason = fd < 0 ? "open-failed" : got < 0 ? "read-failed" :
            np_vcd_check(header, (size_t)got,
            ((uint64_t)entry.stat.hisize << 32) | entry.stat.size, &info);
        if (reason) { ++invalid; LOG("reject=%s reason=%s\n", entry.name, reason); }
        else { ++valid; LOG("candidate=%s sectors=%lu tracks=%u\n", entry.name,
                           (unsigned long)info.sectors, info.tracks); }
    }
    fileXioDclose(dir);
    if (status < 0) { LOG("error=library-read code=%d\n", status); return -1; }
    LOG("inventory candidates=%u rejected=%u ignored=%u\n", valid, invalid, ignored);
    return 0;
}

int main(int argc, char **argv) {
    int common = 0, library = 0, outcome = 1;
    (void)argc; (void)argv;
    init_scr();
    LOG("PSXCore milestone 1 - read-only APA/PFS probe\n");
    if (init_storage()) { LOG("error=storage-init\n"); goto done; }
    for (unsigned attempt = 0; attempt < 20; ++attempt) {
        int fd = fileXioDopen("hdd0:");
        if (fd >= 0) { fileXioDclose(fd); break; }
        usleep(100000);
    }
    common = fileXioMount("pfs1:", "hdd0:__common", FIO_MT_RDONLY) == 0;
    if (common) {
        prerequisite("POPS.ELF", 3166988, 1);
        prerequisite("IOPRP252.IMG", 265233, 0);
    } else LOG("error=common-partition-unavailable\n");
    library = fileXioMount("pfs2:", "hdd0:__.POPS", FIO_MT_RDONLY) == 0;
    if (!library) { LOG("error=pops-partition-unavailable\n"); goto done; }
    outcome = inventory() == 0 ? 0 : 1;
done:
    if (library && fileXioUmount("pfs2:") < 0) outcome = 1;
    if (common && fileXioUmount("pfs1:") < 0) outcome = 1;
    LOG("complete result=%d mode=read-only execution=disabled\n", outcome);
    SleepThread();
    return outcome;
}
