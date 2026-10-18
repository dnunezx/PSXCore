/* Original transactional input + normal-interrupt scheduler binding. */
#ifndef PSXCORE_CD_SWAP_PATCH_H
#define PSXCORE_CD_SWAP_PATCH_H
#include "cdswap_bindings.h"
#include "cdswap.h"
static const char *np_runtime_cd_swap_plan(const void *image,size_t bytes,const NpElf *elf,
    uintptr_t boot,uintptr_t tail,uintptr_t code_end,uintptr_t end,
    uintptr_t record,uintptr_t record_end,uintptr_t state,uintptr_t helper,
    uintptr_t input,uintptr_t begin,uintptr_t control_end,uintptr_t config,uintptr_t tocs,NpPatchSet *set) {
    if(!set || !elf || !elf->count || elf->count>8 || set->count>NP_PATCH_LIMIT-3)return "cd-swap-input";
    if(begin!=0xe8000 || control_end<=begin || control_end>0xe9000 || config<begin ||
       (config&63) || config>control_end || control_end-config<sizeof(NpCdSwap) || tocs!=0xea000 ||
       input<0xeb000 || input>=code_end || input==boot || input==tail || (input&3))return "cd-swap-arena";
    for(unsigned i=0;i<elf->count;++i){
        const NpSegment *s=&elf->segments[i];
        if(s->address<0xec000 && (uint64_t)s->address+s->memsz>0xe8000)return "cd-swap-pops-overlap";
    }
    const uint32_t windows[][2]={{0x20c010,0x20c0d8},{0x2289e0,0x228ad4},{0x237798,0x237814}};
    for(unsigned i=0;i<set->count;++i)for(unsigned j=0;j<3;++j){
        const NpPatch *p=&set->items[i];
        if(p->address<windows[j][1] && windows[j][0]<(uint64_t)p->address+p->length)return "cd-swap-overlap";
    }
    NpPatchSet candidate=*set;
    const char *error=np_runtime_cd_swap_context_plan(image,bytes,elf,boot,tail,code_end,end,record,record_end,state,helper,&candidate);
    if(error)return error;
    const uint32_t expected[2]={0x0c08a278,0x26040002};
    unsigned hits=0;
    for(unsigned i=0;i<elf->count;++i){
        const NpSegment *s=&elf->segments[i];
        if(0x20c07c>=s->address && 0x20c084u<=(uint64_t)s->address+s->filesz){
            uint64_t offset=(uint64_t)s->offset+0x20c07c-s->address;
            if(offset+8>bytes || memcmp((const unsigned char *)image+(size_t)offset,expected,8))return "cd-swap-mapping";
            ++hits;
        }
    }
    if(hits!=1)return "cd-swap-segment";
    const uint32_t replacement[2]={0x0c000000u|((uint32_t)input>>2),expected[1]};
    error=np_patch_add(&candidate,0x20c07c,expected,replacement,8);
    if(!error)*set=candidate;
    return error;
}
#endif
