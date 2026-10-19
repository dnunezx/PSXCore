/* Original bounded, transactional staged-image patch engine. */
#ifndef PSXCORE_PATCH_H
#define PSXCORE_PATCH_H
#include "psxcore_elf.h"
#define NP_PATCH_BYTES 32u
/* Includes file storage, native IGR, disc swapping and optional diagnostics. */
#define NP_PATCH_LIMIT 100u
typedef struct {
    uint32_t address, length;
    unsigned char expected[NP_PATCH_BYTES], replacement[NP_PATCH_BYTES];
} NpPatch;
typedef struct { unsigned count; NpPatch items[NP_PATCH_LIMIT]; } NpPatchSet;
const char *np_patch_add(NpPatchSet *set, uint32_t address,
    const void *expected, const void *replacement, size_t length);
/* Checks all bounds, expected bytes and overlap before changing any byte.
 * Caller verifies runtime fingerprint first; this is not an ELF/hash validator. */
const char *np_patch_apply(void *image, size_t size, const NpElf *elf, const NpPatchSet *set);
#endif
