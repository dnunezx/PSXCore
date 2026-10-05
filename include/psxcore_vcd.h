/* Original PSXCore code, Danny Nunez (dnunezx), 2026. */
#ifndef PSXCORE_VCD_H
#define PSXCORE_VCD_H
#include <stddef.h>
#include <stdint.h>
#define NP_VCD_HEADER_BYTES 0x410u
#define NP_VCD_DATA_OFFSET 0x100000u
#define NP_VCD_SECTOR_BYTES 2352u
typedef struct { uint32_t sectors; unsigned tracks; } NpVcdInfo;
/* Structural inventory only; does not assert complete disc integrity/playability. */
const char *np_vcd_check(const unsigned char *header, size_t bytes,
                         uint64_t file_bytes, NpVcdInfo *info);
int np_vcd_filename(const char *name);
#endif
