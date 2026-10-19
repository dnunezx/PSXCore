/* Fingerprinted native-menu presentation and button mapping only. */
#ifndef NP_IGR_MENU_H
#define NP_IGR_MENU_H
#include "psxcore_patch.h"
#include <string.h>
static const char *np_runtime_igr_menu_plan(const void *image,size_t bytes,
    const NpElf *elf,uintptr_t draw,uintptr_t loader_end,NpPatchSet *set) {
    const uint32_t addresses[3]={0x226768,0x2264f4,0x22651c};
    const uint32_t expected[3][2]={{0x27bdffc0,0x0000202d},{0x30620020,0},{0x30620040,0}};
    const unsigned lengths[3]={8,4,4};
    if(!image || !elf || !set || !elf->count || elf->count>8 ||
       set->count>NP_PATCH_LIMIT-3)return "igr-menu-input";
    if(draw<0x84000 || draw>=loader_end || loader_end>0xe0000 || (draw&3))return "igr-menu-layout";
    for(unsigned i=0;i<3;++i) {
        unsigned hits=0;
        for(unsigned j=0;j<elf->count;++j) {
            const NpSegment *s=&elf->segments[j];
            if(addresses[i]>=s->address && (uint64_t)addresses[i]+lengths[i]<=(uint64_t)s->address+s->filesz) {
                uint64_t off=(uint64_t)s->offset+addresses[i]-s->address;
                if(off+lengths[i]>bytes || memcmp((const unsigned char *)image+(size_t)off,expected[i],lengths[i]))return "igr-menu-expected";
                for(unsigned k=0;k<elf->count;++k)if(k!=j && off<(uint64_t)elf->segments[k].offset+elf->segments[k].filesz &&
                    elf->segments[k].offset<off+lengths[i])return "igr-menu-file-alias";
                ++hits;
            }
        }
        if(hits!=1)return "igr-menu-segment";
        for(unsigned j=0;j<set->count;++j) {
            const NpPatch *p=&set->items[j];
            if(addresses[i]<(uint64_t)p->address+p->length && p->address<(uint64_t)addresses[i]+lengths[i])return "igr-menu-overlap";
        }
    }
    NpPatchSet candidate=*set;
    const uint32_t replacement[3][2]={{0x08000000u|((uint32_t)draw>>2),0},{0x30620040,0},{0x30620020,0}};
    for(unsigned i=0;i<3;++i) {
        const char *error=np_patch_add(&candidate,addresses[i],expected[i],replacement[i],lengths[i]);
        if(error)return error;
    }
    *set=candidate;return NULL;
}
#endif
