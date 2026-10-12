/* Pinned scheduler cleanup boundary. Diagnostic only; no lease or swap API. */
#ifndef PSXCORE_CD_RENDEZVOUS_H
#define PSXCORE_CD_RENDEZVOUS_H
#include "psxcore_patch.h"
#include <string.h>
typedef struct {
    uint32_t magic,version,bytes,reserved;
    uint32_t generation,visits,overflow,nested;
    uint32_t dispatching,cd_queued,cd_write,cd_read,cd_active,cd_submissions;
    uint32_t mixed_queued,mixed_produce,mixed_consume;
    uint32_t read_event,play_event,request_lba,dma_busy,dma_event,dma_chcr;
    uint32_t active_response,cd_registers,verification_jobs,verification_count;
    uint32_t card0_flags,card1_flags,cache_count,scheduler_pending,reserved_tail;
} NpCdRendezvousObservation;
_Static_assert(sizeof(NpCdRendezvousObservation)==128,"CD rendezvous layout");
extern NpCdRendezvousObservation np_cd_rendezvous_observation;
extern char np_cd_rendezvous_begin[],np_cd_rendezvous_end[];
extern void np_cd_rendezvous_tail(void);
/* Require equal even generations around reads, visits>0, overflow=nested=0.
 * Those checks only establish a stable record, not cross-worker exclusion.
 * Raw scratchpad fields are copied by guest code; no host scratchpad mapping
 * is needed. No native CD/DMA/save fields are written or cancelled. */
static const char *np_runtime_cd_rendezvous_plan(const void *image,size_t bytes,
    const NpElf *elf,uintptr_t begin,uintptr_t end,uintptr_t hook,NpPatchSet *set) {
    static const uint32_t context[][2]={
        {0x200f5c,0xaf80882c},{0x200f64,0xaf808830},
        {0x200fa4,0x00c0202d},{0x200fa8,0x1080000b},{0x200fac,0xdfb00000},
        {0x200fb0,0x8c820014},{0x200fb4,0x00000000},{0x200fb8,0x10400003},
        {0x200fbc,0x8c900000},{0x200fc0,0x0c080476},{0x200fc4,0x00000000},
        {0x200fc8,0x0200202d},{0x200fcc,0x5480fffa},{0x200fd0,0x8c820014},
        {0x200fd4,0xdfb00000},{0x200fd8,0xdfb10008},{0x200fdc,0xdfbf0010},
        {0x200fe0,0x03e00008},{0x200fe4,0x27bd0020}};
    if(!image || !elf || !set || !elf->count || elf->count>8 ||
       set->count>=NP_PATCH_LIMIT)return "cd-rendezvous-input";
    if(begin<0xf8000 || end<=begin || end-begin<=sizeof(NpCdRendezvousObservation) ||
       end>0xff000 || (begin&15) || (end&15) || hook<begin ||
       hook>=end-sizeof(NpCdRendezvousObservation) || (hook&3))return "cd-rendezvous-resident";
    for(unsigned i=0;i<sizeof(context)/sizeof(context[0]);++i) {
        unsigned matches=0;
        for(unsigned j=0;j<elf->count;++j) {
            const NpSegment *s=&elf->segments[j];
            if(context[i][0]>=s->address && (uint64_t)context[i][0]+4<=(uint64_t)s->address+s->filesz) {
                uint64_t off=(uint64_t)s->offset+context[i][0]-s->address;
                uint32_t word;
                if(off+4>bytes)return "cd-rendezvous-file-bounds";
                memcpy(&word,(const unsigned char *)image+(size_t)off,4);
                if(word!=context[i][1])return "cd-rendezvous-expected";
                ++matches;
            }
        }
        if(matches!=1)return "cd-rendezvous-segment";
        for(unsigned j=0;j<set->count;++j) {
            const NpPatch *p=&set->items[j];
            if(context[i][0]<(uint64_t)p->address+p->length &&
               p->address<(uint64_t)context[i][0]+4)return "cd-rendezvous-overlap";
        }
    }
    const uint32_t expected[2]={0x03e00008,0x27bd0020};
    const uint32_t replacement[2]={0x08000000u|((uint32_t)hook>>2),expected[1]};
    NpPatchSet candidate=*set;
    const char *error=np_patch_add(&candidate,0x200fe0,expected,replacement,8);
    if(!error)*set=candidate;
    return error;
}
#endif
