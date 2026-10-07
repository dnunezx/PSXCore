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
        /* POPS's SWC2 path supplies its value in EE a3. Capture the value
         * before the signed offset argument replaces that register. */
        first = (rt << 21) | 0x0000282du;
        second = jump;
        third = offset;
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

#if defined(__mips__)
__attribute__((section("._psxcore_return_delay")))
#endif
int np_memory_emit_finalize_rewrite(uint32_t instruction, uint32_t ps1_base,
    uint32_t ps1_target, uint32_t *out, const uint32_t helpers[16],
    const uint32_t return_helpers[8]) {
    if (!out) return -1;
    uint32_t plan[3] = {out[0],out[1],out[2]};
    int result = np_memory_emit_rewrite(instruction,ps1_base,plan,helpers);
    if (result != 1) return result;
    const unsigned op = instruction >> 26, rt = (instruction >> 16) & 31;
    if (op < 0x28 && ps1_target == 2 && rt >= 8 && (rt <= 25 || rt == 30)) {
#if defined(__mips__)
        if ((uintptr_t)out < 0x100000 + 72 || (uintptr_t)out > 0x2000000 - 12)
            return -1;
#endif
        /* Prove the allocated destination contains the old constant return
         * value. A load's fresh allocation need not initialize its old value.
         * Pinned generator: publish JR ra's captured target, then construct
         * a scalar address from the CPU register bank. The callback beginning
         * directly at the load does not have this prefix and stays native. */
        const uint32_t immediate = out[-6] >> 26;
        if ((out[-7] & 0xffff0000u) == (0x3c000000u | (rt << 16)) &&
            (immediate == 9 || immediate == 13) &&
            (out[-6] & 0x03ff0000u) == ((rt << 21) | (rt << 16)) &&
            out[-5] == 0xaf9e0000 &&
            out[-4] == (0x8f880000u | (0x10 + ps1_base * 4)) &&
            out[-3] == 0x3c054000 && out[-2] == 0x00282024 && out[-1] == 0x00a42021) {
#if defined(__mips__)
            /* The allocator may reuse another PS1 register's native slot.
             * Prove these constants belong to PS1 v0 using the pinned block
             * checksum prologue and original source, rather than assuming
             * every native LUI/immediate pair defines the load's old value. */
            if ((out[-18] & 0xffff0000u) != 0x3c020000 ||
                (out[-17] & 0xffff0000u) != 0x3c030000 ||
                (out[-16] & 0xffff0000u) != 0x8c420000 ||
                (out[-15] & 0xffff0000u) != 0x8c630000 ||
                (out[-14] & 0xffff0000u) != 0x3c040000 || out[-13] != 0x00431021 ||
                (out[-12] & 0xffff0000u) != 0x34830000 || out[-11] != 0x10430003 ||
                out[-10] || out[-9] != 0x0808207c || out[-8] != 0x24020006) goto publish;
            uint32_t address = (out[-18] << 16) + (int16_t)out[-16];
            uint32_t second = (out[-17] << 16) + (int16_t)out[-15];
            if (address < 0x40000000 || address > 0x407ffff0 || (address & 3) ||
                second != address + 4) goto publish;
            const volatile uint32_t *source = (const volatile uint32_t *)(uintptr_t)address;
            if (source[0] != (0x3c020000u | (out[-7] & 0xffff)) ||
                source[1] != ((immediate << 26) | 0x00420000u | (out[-6] & 0xffff)) ||
                source[2] != 0x03e00008 ||
                source[3] != ((op << 26) | (ps1_base << 21) | 0x00020000u |
                              (instruction & 0xffff))) goto publish;
#endif
            if (!return_helpers) return -1;
            const uint32_t helper = return_helpers[op - 0x20];
            if (!helper || helper < 0xf7000 || helper >= 0xf7800 || (helper & 3)) return -1;
            plan[0] = 0x0c000000u | (helper >> 2);
        }
    }
#if defined(__mips__)
publish:
#endif
    out[0] = plan[0]; out[1] = plan[1]; out[2] = plan[2];
    return 1;
}

#if defined(__mips__)
__attribute__((section("._psxcore_return_delay")))
#endif
int np_memory_return_load_cancelled(uint32_t instruction) {
    unsigned op = instruction >> 26;
    if (op >= 9 && op <= 15) return ((instruction >> 16) & 31) == 2;
    if (op || ((instruction >> 11) & 31) != 2) return 0;
    unsigned fn = instruction & 63;
    /* Shifts and nontrapping integer ALU operations. */
    if (fn < 32) return (0xddu >> fn) & 1;
    return (0xcfau >> (fn - 32)) & 1;
}
