/* Original diagnostic bootstrap contract. Separate from Stage 2C. */
#ifndef PSXCORE_BOOT_H
#define PSXCORE_BOOT_H
#include "psxcore_elf.h"
#define NP_BOOT_STAGE 0x01800000u
#define NP_BOOT_CAPACITY 0x00400000u
#define NP_BOOT_IOP 0x01c00000u
#define NP_BOOT_IOP_CAPACITY 0x00080000u
#define NP_BOOT_PLAN 0x01e00000u
#define NP_BOOT_STACK 0x01f00000u
#define NP_BOOT_RAM_END 0x00865940u
#define NP_BOOT_SPAD 0x70000000u
#define NP_BOOT_SPAD_BYTES 0x4000u
typedef struct {
    NpElf elf;
    uint32_t argc, argv[3];
    char strings[2][128];
    uint32_t mfifo_pre_kernel, scratch_pre_kernel;
} NpBootPlan;
const char *np_boot_elf_check(const void *bytes, size_t size, NpElf *out);
void np_boot_sha256(const void *bytes, size_t size, unsigned char digest[32]);
#endif
