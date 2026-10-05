/* Original PSXCore code, Danny Nunez (dnunezx), 2026. */
#include "psxcore_vcd.h"
#include <string.h>

static uint32_t le32(const unsigned char *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static int bcd(unsigned value) {
    return (value & 15) < 10 && (value >> 4) < 10 ?
           (int)((value >> 4) * 10 + (value & 15)) : -1;
}
int np_vcd_filename(const char *name) {
    size_t n = name ? strlen(name) : 0;
    return n > 4 && name[n-4] == '.' &&
        (name[n-3] == 'V' || name[n-3] == 'v') &&
        (name[n-2] == 'C' || name[n-2] == 'c') &&
        (name[n-1] == 'D' || name[n-1] == 'd');
}
const char *np_vcd_check(const unsigned char *h, size_t bytes,
                         uint64_t file_bytes, NpVcdInfo *info) {
    uint32_t sectors;
    int tracks, hi_minutes, lo_minutes, seconds, frames;
    if (info) memset(info, 0, sizeof(*info));
    if (!h || bytes < NP_VCD_HEADER_BYTES) return "short-header";
    if (h[0] != 0x41 || h[2] != 0xa0 || h[7] != 1 || h[8] != 0x20 ||
        h[0xc] != 0xa1 || h[0x16] != 0xa2) return "unsupported-toc";
    if (memcmp(h + 0x400, "kHn", 3)) return "missing-vcd-marker";
    if (h[0x403] < 0x20 || h[0x403] > 0x23) return "unsupported-vcd-version";
    tracks = bcd(h[0x11]);
    if (tracks < 1 || tracks > 99) return "invalid-track-count";
    sectors = le32(h + 0x408);
    if (!sectors || sectors != le32(h + 0x40c)) return "invalid-sector-count";
    if (file_bytes != NP_VCD_DATA_OFFSET + (uint64_t)sectors * NP_VCD_SECTOR_BYTES)
        return "payload-size-mismatch";
    hi_minutes = bcd(h[0x1a]); lo_minutes = bcd(h[0x1b]);
    seconds = bcd(h[0x1c]); frames = bcd(h[0x1d]);
    if (hi_minutes < 0 || lo_minutes < 0 || seconds < 0 || seconds >= 60 ||
        frames < 0 || frames >= 75) return "invalid-leadout-time";
    if ((uint64_t)(hi_minutes * 100 + lo_minutes) * 4500 + seconds * 75 + frames
        != (uint64_t)sectors + 150) return "leadout-size-mismatch";
    if (info) { info->sectors = sectors; info->tracks = (unsigned)tracks; }
    return NULL;
}
