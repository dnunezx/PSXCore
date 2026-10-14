#include "codeguard.h"
#include <stddef.h>
#if defined(__mips__)
#define RESIDENT __attribute__((section("._psxcore_code_guard")))
#else
#define RESIDENT
#endif
static inline int decoded(const uint32_t *records, unsigned i) {
    const uint32_t *record = records + i * NP_CODE_GUARD_RECORD_WORDS;
    return (record[0] & 1) && record[11] != NP_CODE_GUARD_EXIT_EMITTER;
}
RESIDENT unsigned np_code_guard_plan(uint32_t psx_pc, const uint32_t *records,
    const uint32_t *originals, unsigned count, uint32_t *out, unsigned capacity,
    uint32_t address, uint32_t validator) {
    if (!records || !originals || !out ||
        (((uintptr_t)records | (uintptr_t)originals | (uintptr_t)out) & 3) ||
        !count || count > NP_CODE_GUARD_MAX_RECORDS ||
        (psx_pc & 3) || (psx_pc & 0x1fffffffu) >= 0x800000u ||
        (address & 3) || address < 0x100000u || address >= 0x2000000u ||
        (validator & 3) || validator < 0xff000u || validator >= 0x100000u)
        return 0;
    /* Emit exact comparisons in the block itself. Aligned 64-bit pairs avoid
     * SIMD reduction; short blocks avoid the shared validator's stack,
     * metadata traversal and loop branches. */
    unsigned code = 2, source_hi = ~0u, comparisons = 0, units = 0;
    for (unsigned i = 0; i < count;) {
        if (!decoded(records,i)) { ++i; continue; }
        const uint32_t source = 0x40000000u | ((psx_pc + i * 4) & 0x1fffff);
        const unsigned hi = (source + 0x8000u) >> 16;
        if (hi != source_hi) { ++code; source_hi = hi; }
        const int pair = !(source & 7) && i + 2 <= count && decoded(records,i+1);
        code += 4;
        ++units;
        comparisons += pair ? 2 : 1;
        i += pair ? 2 : 1;
    }
    /* Each intermediate branch delay slot prepares the next comparison. */
    if (units) code -= units - 1;
    code += 4; /* Success jump over snapshots; local mismatch exit. */
    const unsigned head = (((address + code * 4 + 15u) & ~15u) - address) / 4 +
        ((psx_pc & 15) / 4);
    const unsigned words = head + count;
    if (!comparisons || words > capacity || words > (0x2000000u - address) / 4)
        return 0;
    const uint32_t snapshot = address + head * 4, body = address + words * 4;
    const uint32_t failure = address + (code - 2) * 4;
    out[0] = 0x3c050000u | (snapshot >> 16);
    out[1] = 0x34a50000u | (snapshot & 0xffff);
    unsigned at = 2, delay = ~0u;
    source_hi = ~0u;
    for (unsigned i = 0; i < count;) {
        if (!decoded(records,i)) { ++i; continue; }
        const uint32_t source = 0x40000000u | ((psx_pc + i * 4) & 0x1fffff);
        const unsigned hi = (source + 0x8000u) >> 16;
        if (hi != source_hi) {
            const uint32_t load = 0x3c040000u | hi;
            if (delay != ~0u) { out[delay] = load; delay = ~0u; }
            else out[at++] = load;
            source_hi = hi;
        }
        const int pair = !(source & 7) && i + 2 <= count && decoded(records,i+1);
        const uint32_t load = (pair ? 0xdc820000u : 0x8c820000u) | (source & 0xffff);
        if (delay != ~0u) { out[delay] = load; delay = ~0u; }
        else out[at++] = load;
        out[at++] = (pair ? 0xdca30000u : 0x8ca30000u) | (i * 4);
        out[at] = 0x14430000u |
            ((failure - (address + at * 4) - 4) / 4);
        ++at;
        delay = at;
        out[at++] = 0;
        i += pair ? 2 : 1;
    }
    out[at++] = 0x08000000u | (body >> 2);
    out[at++] = 0;
    out[at++] = 0x08000000u | (validator >> 2);
    out[at++] = 0;
    while (at < head) out[at++] = 0;
    for (unsigned i = 0; i < count; ++i)
        out[at++] = decoded(records,i) ? originals[i] : 0;
    return words;
}
#if defined(__mips__)
extern void np_code_guard_validate(void);
extern uint32_t np_code_guard_originals[NP_CODE_GUARD_MAX_RECORDS];
RESIDENT uint32_t *np_code_guard_emit(uint32_t psx_pc, uint32_t *out,
    const uint32_t *last) {
    const uintptr_t first = 0x503b20u, end = (uintptr_t)last;
    if (end < first || end - first >= NP_CODE_GUARD_MAX_RECORDS * 48u ||
        (end - first) % 48u || ((uintptr_t)out & 3)) return NULL;
    const uint32_t base = *(volatile uint32_t *)0x502738;
    const uint32_t limit = *(volatile uint32_t *)0x50273c;
    const uintptr_t cursor = (uintptr_t)out;
    if (base < 0x100000u || limit > 0x1000000u || base >= limit ||
        cursor < base || cursor >= limit) return NULL;
    const unsigned count = (end - first) / 48u + 1;
    const unsigned words = np_code_guard_plan(psx_pc, (const uint32_t *)first,
        np_code_guard_originals, count, out, (limit - cursor) / 4u, (uint32_t)cursor,
        (uint32_t)(uintptr_t)np_code_guard_validate);
    return words ? out + words : NULL;
}
#endif
