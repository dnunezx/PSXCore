/* Transactional boot-context and normal-interrupt scheduler binding. */
#ifndef PSXCORE_CD_SWAP_BINDINGS_H
#define PSXCORE_CD_SWAP_BINDINGS_H
#include "psxcore_boot.h"
#include "psxcore_patch.h"
#include "cdswap_runtime.h"
#include "cdreopen.h"
#include <string.h>
static const char *np_runtime_cd_swap_context_plan(const void *image,size_t bytes,const NpElf *elf,
    uintptr_t boot,uintptr_t tail,uintptr_t code_end,uintptr_t end,
    uintptr_t record,uintptr_t record_end,uintptr_t state,uintptr_t helper,NpPatchSet *set) {
    static const unsigned char hash[32]={
        0x59,0xdf,0x33,0x89,0xc4,0xdf,0x88,0xa5,0x72,0xda,0xa7,0x20,0xb0,0x55,0x07,0xc5,
        0x2c,0x34,0xed,0xdf,0xa0,0x03,0x1a,0x6f,0xbe,0xec,0x55,0xe0,0xc2,0xd0,0xfc,0xb1};
    static const uint32_t windows[][2]={{0x200198,0x2001a0},{0x200e98,0x200fe8},
        {0x201008,0x20101c},{0x22e710,0x22e730},{0x22e840,0x22e850},
        {0x22e870,0x22e880},{0x230330,0x230520}};
    unsigned char actual[32];
    if(!image || bytes!=3166988 || !elf || !set || !elf->count || elf->count>8 ||
       set->count>NP_PATCH_LIMIT-2)return "cd-swap-context-input";
    if(boot<0xeb000 || boot>=code_end || tail<0xeb000 || tail>=code_end ||
       boot==tail || ((boot|tail)&3) || code_end<=0xeb000 || code_end>=end ||
       end>0xec000 || ((code_end|end)&15) || record<code_end || (record&15) ||
       record_end<record || record_end-record!=768 || record_end>end ||
       state<record_end || (state&15) || state>end || end-state<96 ||
       helper<0xe9000 || helper>=0xea000 || (helper&63))return "cd-swap-context-arena";
    np_boot_sha256(image,bytes,actual);
    if(memcmp(actual,hash,32))return "cd-swap-context-fingerprint";
    /* Retain all native scheduler/RPC/kernel call windows in this binding.
     * Any previously planned change to one of them rejects composition. */
    for(unsigned i=0;i<set->count;++i)for(unsigned j=0;j<sizeof(windows)/sizeof(windows[0]);++j) {
        const NpPatch *p=&set->items[i];
        if(p->address<windows[j][1] && windows[j][0]<(uint64_t)p->address+p->length)
            return "cd-swap-context-overlap";
    }
    const uint32_t addresses[2]={0x200198,0x200fe0};
    const uint32_t expected[2][2]={{0x0c08d7aa,0},{0x03e00008,0x27bd0020}};
    const uint32_t replacement[2][2]={{0x08000000u|((uint32_t)boot>>2),0},
        {0x08000000u|((uint32_t)tail>>2),0x27bd0020}};
    /* Require unique file-backed mappings even though the complete source
     * hash already matches. Planning never writes the source image. */
    for(unsigned i=0;i<2;++i) {
        unsigned hits=0;
        for(unsigned j=0;j<elf->count;++j) {
            const NpSegment *s=&elf->segments[j];
            if(addresses[i]>=s->address && (uint64_t)addresses[i]+8<=(uint64_t)s->address+s->filesz) {
                uint64_t offset=(uint64_t)s->offset+addresses[i]-s->address;
                if(offset+8>bytes || memcmp((const unsigned char *)image+(size_t)offset,expected[i],8))
                    return "cd-swap-context-mapping";
                ++hits;
            }
        }
        if(hits!=1)return "cd-swap-context-segment";
    }
    NpPatchSet candidate=*set;
    const char *error=np_patch_add(&candidate,addresses[0],expected[0],replacement[0],8);
    if(!error)error=np_patch_add(&candidate,addresses[1],expected[1],replacement[1],8);
    if(!error)*set=candidate;
    return error;
}
#endif
