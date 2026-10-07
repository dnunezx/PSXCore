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
/* Scalar finalizer with the pinned JR-ra delay-load prefix. Only a load into
 * PS1 v0 can use the return helper; other reservations retain normal emission.
 * A preceding LUI/immediate pair must define the same allocated destination;
 * newly allocated load destinations cannot supply the old PS1 value.
 * The native path also checks its preceding checksum prologue and the original
 * PS1 constant-v0/JR-ra/load-v0 instructions. out has 18 preceding words. */
int np_memory_emit_finalize_rewrite(uint32_t instruction, uint32_t ps1_base,
    uint32_t ps1_target, uint32_t *out, const uint32_t helpers[16],
    const uint32_t return_helpers[8]);
/* Nontrapping ALU writes cancel a pending PS1 v0 load, including instructions
 * which also read its old value. Loads and conditional/link writes are excluded. */
int np_memory_return_load_cancelled(uint32_t instruction);
/* The pinned grouped emitter has eight stack slots for allocated registers.
 * Validate every descriptor/reservation before changing any generated word.
 * Return the number rewritten, 0 for native-only paths, or a negative
 * diagnostic stage on rejection. */
int np_memory_group_rewrite(uint32_t *end, unsigned count, unsigned ps1_base,
                           const uint32_t records[][3], const uint32_t registers[],
                           const uint32_t helpers[16]);
#endif
