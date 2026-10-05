#include "psxcore_patch.h"
#include <string.h>

const char *np_patch_add(NpPatchSet *set, uint32_t address,
    const void *expected, const void *replacement, size_t length) {
    if (!set || set->count >= NP_PATCH_LIMIT || !expected || !replacement ||
        !length || length > NP_PATCH_BYTES || (uint64_t)address + length > UINT32_MAX)
        return "patch-input";
    NpPatch *p = &set->items[set->count++];
    memset(p, 0, sizeof(*p));
    p->address = address; p->length = (uint32_t)length;
    memcpy(p->expected, expected, length); memcpy(p->replacement, replacement, length);
    return NULL;
}
const char *np_patch_apply(void *image, size_t size, const NpElf *elf, const NpPatchSet *set) {
    size_t offsets[NP_PATCH_LIMIT];
    if (!image || !elf || !set || set->count > NP_PATCH_LIMIT || !elf->count || elf->count > 8)
        return "patch-input";
    for (unsigned i = 0; i < set->count; ++i) {
        const NpPatch *p = &set->items[i];
        unsigned matches = 0;
        uint64_t end = (uint64_t)p->address + p->length;
        if (!p->length || p->length > NP_PATCH_BYTES || end > UINT32_MAX) return "patch-range";
        for (unsigned j = 0; j < elf->count; ++j) {
            const NpSegment *s = &elf->segments[j];
            if (p->address >= s->address && end <= (uint64_t)s->address + s->filesz) {
                uint64_t offset = (uint64_t)s->offset + p->address - s->address;
                if (offset + p->length > size) return "patch-file-bounds";
                offsets[i] = (size_t)offset; ++matches;
            }
        }
        if (matches != 1) return "patch-segment";
        for (unsigned k = 0; k < i; ++k) {
            const NpPatch *q = &set->items[k];
            if ((p->address < (uint64_t)q->address + q->length && q->address < end) ||
                (offsets[i] < offsets[k] + q->length && offsets[k] < offsets[i] + p->length))
                return "patch-overlap";
        }
        if (memcmp((unsigned char *)image + offsets[i], p->expected, p->length)) return "patch-expected";
    }
    for (unsigned i = 0; i < set->count; ++i)
        memcpy((unsigned char *)image + offsets[i], set->items[i].replacement, set->items[i].length);
    return NULL;
}
