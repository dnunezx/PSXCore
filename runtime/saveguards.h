/* Internal, bounded plan for required guards on the checksum-pinned runtime. */
#ifndef PSXCORE_RUNTIME_SAVEGUARDS_H
#define PSXCORE_RUNTIME_SAVEGUARDS_H
#include "psxcore_patch.h"
#include <string.h>
static const char *np_runtime_save_plan(const void *image,size_t bytes,const NpElf *elf,
    const uintptr_t hooks[9],NpPatchSet *set) {
    static const uint32_t sites[9]={0x20c8c4,0x20c8f8,0x20c908,0x20c928,0x20c938,
        0x20c884,0x20d0f8,0x20d138,0x20d148};
    static const uint32_t targets[9]={0x231358,0x231c08,0x2315e0,0x2332a8,0x2334b8,
        0x22e860,0x22e820,0x22e620,0x22e640};
    static const uint32_t delays[9]={0x240601b6,0,0x0200202d,0x03a0202d,0x0000282d,
        0x0000982d,0xafa00054,0xaf828868,0xaf828864};
    if(!image || !elf || !hooks || !set || !elf->count || elf->count>8 ||
       set->count>NP_PATCH_LIMIT-9)return "save-plan-input";
    for(unsigned i=0;i<9;++i) {
        if(hooks[i]<0xf0000 || hooks[i]>=0xf7000 || (hooks[i]&3))return "save-plan-hook";
        unsigned matches=0;
        for(unsigned j=0;j<elf->count;++j) {
            const NpSegment *s=&elf->segments[j];
            if(sites[i]>=s->address && (uint64_t)sites[i]+8<=(uint64_t)s->address+s->filesz) {
                uint64_t offset=(uint64_t)s->offset+sites[i]-s->address;
                uint32_t words[2];
                if(offset+8>bytes)return "save-plan-file-bounds";
                memcpy(words,(const unsigned char *)image+(size_t)offset,8);
                if(words[0]!=(0x0c000000u|(targets[i]>>2)) || words[1]!=delays[i])
                    return "save-plan-expected";
                ++matches;
            }
        }
        if(matches!=1)return "save-plan-segment";
    }
    for(unsigned i=0;i<9;++i) {
        uint32_t expected=0x0c000000u|(targets[i]>>2);
        uint32_t replacement=0x0c000000u|((uint32_t)hooks[i]>>2);
        const char *error=np_patch_add(set,sites[i],&expected,&replacement,4);
        if(error)return error;
    }
    return NULL;
}
#endif
