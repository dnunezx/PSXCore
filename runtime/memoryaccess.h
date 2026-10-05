/* Original POPS generated-memory adaptation. No host-emulator dependency. */
#ifndef NP_MEMORYACCESS_H
#define NP_MEMORYACCESS_H
#include <stdint.h>

/* Entries correspond to EE memory opcodes 0x20..0x2f. Zero means unsupported.
 * Rewrite only the pinned generator's three-word scalar-access reservation.
 * Return 1 on replacement, 0 on unchanged native paths, -1 on a bad plan.
 * SP-based and merge accesses retain native emission. */
int np_memory_emit_rewrite(uint32_t instruction, uint32_t ps1_base,
                           uint32_t out[3], const uint32_t helpers[16]);
/* The pinned grouped emitter has eight stack slots for allocated registers.
 * Validate every descriptor/reservation before changing any generated word.
 * Return the number rewritten, 0 for native-only paths, or a negative
 * diagnostic stage on rejection. */
int np_memory_group_rewrite(uint32_t *end, unsigned count, unsigned ps1_base,
                           const uint32_t records[][3], const uint32_t registers[],
                           const uint32_t helpers[16]);
#endif
