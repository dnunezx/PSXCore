#ifndef NP_POLYGON_PACKET_PATCH_H
#define NP_POLYGON_PACKET_PATCH_H
#include "psxcore_patch.h"
#include <string.h>

/* Caller verifies the complete POPS revision. Check the local entry/finalizer
 * contracts and resident placement before adding the transactional redirect. */
static inline const char *np_polygon_packet_plan(const void *image,size_t size,
    const NpElf *elf,NpPatchSet *set,uintptr_t begin,uintptr_t table,uintptr_t end) {
    if(!image || !elf || !elf->count || elf->count>NP_ELF_MAX_SEGMENTS ||
       !set || set->count>=NP_PATCH_LIMIT)return "polygon-packet-input";
    if(begin!=0xffd00 || table<=begin || table>0x100000-164 ||
       (table&15) || end!=table+164 ||
       end>0x100000)return "polygon-packet-resident";
    static const uint32_t entry[8]={0x00158a02,0x2689330c,0x32230001,0x8d2b0000,
        0x32050001,0x8d28000c,0x24040100,0x24060004};
    static const uint32_t finalizer[8]={0x11000009,0xad600098,0,0x25430004,
        0xad400000,0x0069102b,0xae030000,0};
    static const uint32_t epilogue[8]={0xdfb00050,0xdfb10058,0x7bb20040,0xdfb30060,
        0xdfb40068,0xdfb50070,0xdfb60078,0xdfb70080};
    const struct { uint32_t address;const uint32_t *words; } pins[]={
        {0x21f630,entry},{0x21f7d4,finalizer},{0x21f840,epilogue}};
    for(unsigned i=0;i<elf->count;++i) {
        const NpSegment *s=&elf->segments[i];
        if(s->address<0x100000 && (uint64_t)s->address+s->memsz>begin)
            return "polygon-packet-overlap";
    }
    for(unsigned p=0;p<sizeof(pins)/sizeof(pins[0]);++p) {
        unsigned found=0;
        for(unsigned i=0;i<elf->count;++i) {
            const NpSegment *s=&elf->segments[i];
            if(s->address<=pins[p].address &&
               (uint64_t)pins[p].address+32<=(uint64_t)s->address+s->filesz) {
                const uint64_t offset=(uint64_t)s->offset+pins[p].address-s->address;
                if(offset>size || size-offset<32)return "polygon-packet-bounds";
                if(memcmp((const unsigned char *)image+offset,pins[p].words,32))
                    return "polygon-packet-expected";
                ++found;
            }
        }
        if(found!=1)return "polygon-packet-segment";
    }
    const uint32_t replacement[2]={0x08000000u|((uint32_t)begin>>2),0};
    return np_patch_add(set,0x21f630,entry,replacement,sizeof(replacement));
}
#endif
