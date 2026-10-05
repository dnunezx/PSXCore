/* Original Stage 2C ELF contract. Not a Sony runtime loader. */
#ifndef PSXCORE_ELF_H
#define PSXCORE_ELF_H
#include <stddef.h>
#include <stdint.h>
#define NP_ELF_MAX_SEGMENTS 8
#define NP_ELF_MAX_BYTES 0x200000u
#define NP_ELF_LOW 0x00100000u
#define NP_ELF_HIGH 0x01000000u
#define NP_STAGE_BASE 0x01c00000u
#define NP_PLAN_BASE 0x01e00000u
#define NP_STACK_TOP 0x01f00000u
typedef struct { uint32_t offset, address, filesz, memsz; } NpSegment;
typedef struct {
    uint32_t entry, count;
    NpSegment segments[NP_ELF_MAX_SEGMENTS];
} NpElf;
const char *np_elf_check(const void *bytes, size_t size, NpElf *out);
#endif
