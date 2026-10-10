#ifndef NP_CODE_GUARD_H
#define NP_CODE_GUARD_H
#include <stdint.h>
#define NP_CODE_GUARD_RECORD_WORDS 12u
#define NP_CODE_GUARD_MAX_RECORDS 258u
#define NP_CODE_GUARD_MAX_WORDS (9u + 8u * NP_CODE_GUARD_MAX_RECORDS)
#define NP_CODE_GUARD_EXIT_EMITTER 0x2029c8u
/* Decoded opcode records contribute; synthetic exits do not read PS1 code. */
unsigned np_code_guard_plan(uint32_t psx_pc, const uint32_t *records,
    const uint32_t *originals, unsigned count, uint32_t *out, unsigned capacity,
    uint32_t address, uint32_t validator);
#endif
