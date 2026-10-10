/* Checked, diagnostic-only POPS observation. No input or lifecycle changes. */
#ifndef PSXCORE_IGR_OBSERVE_H
#define PSXCORE_IGR_OBSERVE_H
#include "psxcore_patch.h"
#include <string.h>
#define NP_IGR_OBSERVE_MAGIC UINT32_C(0x4f47504e)
typedef struct {
    uint32_t magic,version;
    uint32_t pad_generation,pad_calls,source,destination,copy_bytes;
    uint32_t frame_before,frame_after,source_bytes,state,request,packet,source_checked;
    uint32_t worker_generation,wait_calls,wake_returns,phase;
    int32_t wait_result;
    uint32_t card_flags[2];
    uint32_t alarm_generation,alarm_calls,alarm_sema;
    int32_t alarm_result;
    uint32_t reserved[3];
} NpIgrObservation;
_Static_assert(sizeof(NpIgrObservation)==112,"IGR observation layout");
/* Each pad/worker/alarm record has an independent single writer. Read its
 * generation before/after; accept equal even values only. A pad sample also
 * requires source_checked and equal before/after native frame fields.
 * Raw state/request/packet fields are observations, not a decoded valid pad.
 * Worker phase: 0 unseen, 1 entering wait, 2 returned, 3 failed return.
 * Neither phase 1 nor a clear dirty bit alone proves safe shutdown. */
static const char *np_runtime_igr_observe_plan(const void *image,size_t bytes,
    const NpElf *elf,uintptr_t begin,uintptr_t end,const uintptr_t hooks[2],NpPatchSet *set) {
    static const uint32_t context[][2]={
        {0x2377f4,0x0c0902ef},{0x2377f8,0x0200282d},
        {0x20c818,0x0c08ba14},{0x20c81c,0xaf82886c},
        {0x20c028,0x0c08dde6},{0x20c02c,0x0200302d},
        {0x2377e0,0x0c08ddba},{0x2377f0,0x8e060060},
        {0x237724,0x8e020058},{0x237728,0x8e0300d8},
        {0x237734,0x000211c0},{0x237860,0x90820070},
        {0x20c808,0x8f848868},{0x20c810,0x2402ffff},
    };
    if(!image || !elf || !hooks || !set || !elf->count || elf->count>8 ||
       set->count>NP_PATCH_LIMIT-2)return "igr-observe-input";
    if(begin<0xf8000 || end<=begin || end>0xff000 || (begin&15) || (end&3))
        return "igr-observe-resident";
    for(unsigned i=0;i<2;++i)
        if(hooks[i]<begin || hooks[i]>=end || (hooks[i]&3))return "igr-observe-hook";
    if(hooks[0]==hooks[1])return "igr-observe-hook";
    for(unsigned i=0;i<sizeof(context)/sizeof(context[0]);++i) {
        unsigned matches=0;
        for(unsigned j=0;j<elf->count;++j) {
            const NpSegment *s=&elf->segments[j];
            if(context[i][0]>=s->address && (uint64_t)context[i][0]+4<=(uint64_t)s->address+s->filesz) {
                uint64_t off=(uint64_t)s->offset+context[i][0]-s->address;
                uint32_t word;
                if(off+4>bytes)return "igr-observe-file-bounds";
                memcpy(&word,(const unsigned char *)image+(size_t)off,4);
                if(word!=context[i][1])return "igr-observe-expected";
                ++matches;
            }
        }
        if(matches!=1)return "igr-observe-segment";
        for(unsigned j=0;j<set->count;++j) {
            const NpPatch *p=&set->items[j];
            if(context[i][0]<(uint64_t)p->address+p->length &&
               p->address<(uint64_t)context[i][0]+4)return "igr-observe-overlap";
        }
    }
    const uint32_t sites[2]={0x2377f4,0x20c818};
    for(unsigned i=0;i<2;++i)for(unsigned j=0;j<set->count;++j) {
        const NpPatch *p=&set->items[j];
        /* Also protect the original delay slot whose semantics were checked. */
        if(sites[i]<(uint64_t)p->address+p->length && p->address<(uint64_t)sites[i]+8)
            return "igr-observe-overlap";
    }
    NpPatchSet candidate=*set;
    for(unsigned i=0;i<2;++i) {
        uint32_t expected[2]={context[i*2][1],context[i*2+1][1]};
        uint32_t replacement[2]={0x0c000000u|((uint32_t)hooks[i]>>2),expected[1]};
        const char *error=np_patch_add(&candidate,sites[i],expected,replacement,8);
        if(error)return error;
    }
    *set=candidate;
    return NULL;
}
#endif
