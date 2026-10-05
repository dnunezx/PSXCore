#include "memoryaccess.h"

#if defined(__mips__)
__attribute__((section("._psxcore_memory")))
#endif
int np_memory_emit_rewrite(uint32_t instruction, uint32_t ps1_base,
                           uint32_t out[3], const uint32_t helpers[16]) {
    const unsigned op = instruction >> 26;
    if (ps1_base == 29 || op < 0x20 || op > 0x2f || (op & 3) == 2)
        return 0;
    if (!helpers) return -1;
    const uint32_t helper = helpers[op - 0x20];
    if (!helper) return 0;
    if (!out || ps1_base > 31 || ((instruction >> 21) & 31) != 4 ||
        helper < 0xf0000 || helper >= 0xf7000 || (helper & 3) ||
        out[0] != instruction || out[1] || out[2]) return -1;
    const uint32_t jump = 0x0c000000u | (helper >> 2);
    const uint32_t offset = 0x24070000u | (instruction & 0xffff);
    const uint32_t rt = (instruction >> 16) & 31;
    uint32_t first, second, third;
    if (op & 8) {
        first = offset;
        second = jump;
        third = (rt << 21) | 0x0000282du; /* native store-value delay slot */
    } else {
        first = jump;
        second = offset;                 /* native signed-offset delay slot */
        third = 0x0040002du | (rt << 11); /* native load-result move */
    }
    out[0] = first;
    out[1] = second;
    out[2] = third;
    return 1;
}

#if defined(__mips__)
__attribute__((section("._psxcore_memory")))
#endif
int np_memory_group_rewrite(uint32_t *end, unsigned count, unsigned ps1_base,
                           const uint32_t records[][3], const uint32_t registers[],
                           const uint32_t helpers[16]) {
    if (count > 8 || ps1_base > 31) return -1;
    if (!count || ps1_base == 29) return 0;
    if (!end || !records || !registers || !helpers || ((uintptr_t)end & 3)) return -2;
    unsigned words = 0;
    for (unsigned i = 0; i < count; ++i) {
        const unsigned op = records[i][0], rt = registers[i];
        /* Native allocated registers survive the existing I/O handlers. */
        if (op < 0x20 || op > 0x2e) return -3;
        if (records[i][1] > 31) return -4;
        /* POPS also pins the PS1 return register in EE register 30. */
        if (rt && (rt < 8 || (rt > 25 && rt != 30))) return -5;
        if ((op & 3) != 2 && !helpers[op - 0x20]) return -6;
        words += (op & 3) == 2 ? 1 : 3;
    }
#if defined(__mips__)
    /* Generated code is EE RAM; reject wraparound/outside-RAM cursors before
     * looking backwards. The native descriptor and stack lists are trusted
     * inputs from the checksum-pinned emitter, bounded above to eight entries. */
    if ((uintptr_t)end < 0x100000 + words * 4 || (uintptr_t)end > 0x2000000) return -7;
#endif
    uint32_t *const start = end - words;
    uint32_t plan[8][3];
    unsigned at = 0, changed = 0;
    for (unsigned i = 0; i < count; ++i) {
        const unsigned op = records[i][0], rt = registers[i];
        const uint32_t instruction = (op << 26) | (4 << 21) | (rt << 16) |
                                     (records[i][2] & 0xffff);
        if (start[at] != instruction) return -8;
        if ((op & 3) == 2) { ++at; continue; }
        plan[i][0] = start[at];
        plan[i][1] = start[at + 1];
        plan[i][2] = start[at + 2];
        if (np_memory_emit_rewrite(instruction, ps1_base, plan[i], helpers) != 1) return -9;
        ++changed;
        at += 3;
    }
    at = 0;
    for (unsigned i = 0; i < count; ++i) {
        if ((records[i][0] & 3) == 2) { ++at; continue; }
        /* Generated words are four-byte aligned, including reservations at
         * an odd word offset. Keep publication as aligned word stores. */
        volatile uint32_t *output = start + at;
        output[0] = plan[i][0];
        output[1] = plan[i][1];
        output[2] = plan[i][2];
        at += 3;
    }
    return (int)changed;
}
