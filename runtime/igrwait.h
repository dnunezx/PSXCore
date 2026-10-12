/* Dormant primitive contract. No IGR pause/return capability is provided. */
#ifndef PSXCORE_IGR_WAIT_H
#define PSXCORE_IGR_WAIT_H
#include "psxcore_patch.h"
#include <string.h>
/* After this check and the full native hash gate, a future owner may call the
 * primitive with 1..65535 native alarm units. All failure returns abort waiting;
 * uncertain completion retains resources and quarantines further calls. This
 * check does not establish live scheduling, timeout units or mutation exclusion.
 * Calls must be serialized by one owner; this primitive is not reentrant. */
extern int np_igr_wait_once(uint32_t native_alarm_units);
extern char np_igr_wait_begin[],np_igr_wait_end[];
#if NP_IGR_PROBE
/* Diagnostic phases: 0 idle, 1 attempted, 2 created, 3 alarm setup returned,
 * 4 entering wait, 5 wait returned, 6 delete returned, 7 probe returned.
 * Main and callback have separate single-writer generations. Require matching
 * even generations before/after reading each group. A callback publication
 * records its last semaphore access, not proof that its epilogue has returned.
 * Versions 5/6 reuse the checked native user-mode alarm callback and its stack.
 * Version 6 reserves an additional 640 bytes for native kernel context saving.
 * Callback publication fields remain zero; callback_last_access=1 denotes
 * this native callback selection, not an observed callback acknowledgement.
 * A bounded main stack and generation-bracketed worker and
 * native RPC/file completion reads immediately around the native wait call.
 * Tail counts do not prove device durability or a complete save-drain protocol.
 * Valid worker reads are snapshots, not full job-completion acknowledgement.
 * CP0 Count values use native units. Worker deltas are raw observations only.
 * One main-thread owner; no input reservation, pause or save-drain capability. */
typedef struct {
    uint32_t magic,version,generation,phase,attempts,native_delay;
    int32_t semaphore,alarm,create_result,alarm_result,wait_result,delete_result,result;
    uint32_t quarantined,count_before,count_after;
    uint32_t worker_waits_before,worker_wakes_before,worker_waits_after,worker_wakes_after;
    uint32_t callback_generation,callback_calls;
    int32_t signal_result;
    uint32_t callback_semaphore,count_wait_enter,count_wait_exit,count_callback,caller_sp;
    uint32_t worker_generation_before,worker_valid_before;
    uint32_t worker_generation_after,worker_valid_after;
    uint32_t callback_incoming_sp,callback_private_sp,main_private_sp,callback_last_access;
    uint32_t rpc_before,file_before,rpc_after,file_after,async_valid_before,async_valid_after;
} NpIgrProbeObservation;
_Static_assert(sizeof(NpIgrProbeObservation)==168,"IGR yield probe layout");
extern NpIgrProbeObservation np_igr_probe_observation;
extern char np_igr_probe_stack[],np_igr_probe_stack_end[];
static const char *np_runtime_igr_probe_stack_check(uintptr_t begin,uintptr_t end) {
    if(begin<0xf7900 || end>0xf8000 || end<=begin || end-begin!=768 ||
       (begin&15) || (end&15))return "igr-probe-stack";
    return NULL;
}
extern char np_igr_probe_rpc_complete[],np_igr_probe_file_complete[];
static const char *np_runtime_igr_completion_plan(const void *image,size_t bytes,
    const NpElf *elf,uintptr_t begin,uintptr_t end,const uintptr_t hooks[2],NpPatchSet *set) {
    static const uint32_t context[][2]={
        {0x22fe84,0x0c08bf58},{0x22fe88,0x8e040000},{0x22fe8c,0xae000000},
        {0x22fe90,0xdfbf0020},{0x22fe94,0xdfb10010},{0x22fe98,0xdfb00000},
        {0x22fe9c,0x03e00008},{0x22fea0,0x27bd0030},
        {0x230f48,0xac620000},{0x230f50,0xdfbf0020},{0x230f5c,0xdfbf0020},
        {0x230f60,0x03e00008},{0x230f64,0x27bd0030}};
    if(!image || !elf || !hooks || !set || !elf->count || elf->count>8 ||
       set->count>NP_PATCH_LIMIT-2)return "igr-completion-input";
    if(begin<0xf8000 || end<=begin || end>0xff000 || (begin&15) || (end&3))
        return "igr-completion-resident";
    if(hooks[0]==hooks[1])return "igr-completion-hook";
    for(unsigned i=0;i<2;++i)if(hooks[i]<begin || hooks[i]>=end || (hooks[i]&3))
        return "igr-completion-hook";
    for(unsigned i=0;i<sizeof(context)/sizeof(context[0]);++i) {
        unsigned matches=0;
        for(unsigned j=0;j<elf->count;++j) {
            const NpSegment *s=&elf->segments[j];
            if(context[i][0]>=s->address && (uint64_t)context[i][0]+4<=(uint64_t)s->address+s->filesz) {
                uint64_t off=(uint64_t)s->offset+context[i][0]-s->address;
                uint32_t word;
                if(off+4>bytes)return "igr-completion-file-bounds";
                memcpy(&word,(const unsigned char *)image+(size_t)off,4);
                if(word!=context[i][1])return "igr-completion-expected";
                ++matches;
            }
        }
        if(matches!=1)return "igr-completion-segment";
        for(unsigned j=0;j<set->count;++j) {
            const NpPatch *p=&set->items[j];
            if(context[i][0]<(uint64_t)p->address+p->length &&
               p->address<(uint64_t)context[i][0]+4)return "igr-completion-overlap";
        }
    }
    NpPatchSet candidate=*set;
    const uint32_t sites[2]={0x22fe9c,0x230f60};
    for(unsigned i=0;i<2;++i) {
        const uint32_t expected[2]={0x03e00008,0x27bd0030};
        const uint32_t replacement[2]={0x08000000u|((uint32_t)hooks[i]>>2),0x27bd0030};
        const char *error=np_patch_add(&candidate,sites[i],expected,replacement,8);
        if(error)return error;
    }
    *set=candidate;
    return NULL;
}
#endif
static const char *np_runtime_igr_wait_check(const void *image,size_t bytes,
    const NpElf *elf,uintptr_t begin,uintptr_t end,const NpPatchSet *set) {
    static const uint32_t stubs[5][2]={{0x22e820,0x24030040},{0x22e830,0x24030041},
        {0x22e5a0,0x240300fc},{0x22e860,0x24030044},{0x22e850,0x2403ffbd}};
    static const uint32_t callback[9]={0x27bdfff0,0xffbf0000,0x0c08ba14,
        0x00c0202d,0x0000000f,0x42000038,0xdfbf0000,0x03e00008,0x27bd0010};
    if(!image || !elf || !set || !elf->count || elf->count>8 || set->count>NP_PATCH_LIMIT)
        return "igr-wait-input";
    if(begin<0xf8000 || end<=begin || end>0xff000 || (begin&15) || (end&3))
        return "igr-wait-resident";
    for(unsigned n=0;n<29;++n) {
        uint32_t address,expected;
        if(n<20) {
            unsigned word=n%4;
            address=stubs[n/4][0]+word*4;
            expected=word==0?stubs[n/4][1]:word==1?0x0000000c:word==2?0x03e00008:0;
        } else {address=0x238468+(n-20)*4;expected=callback[n-20];}
        unsigned matches=0;
        for(unsigned j=0;j<elf->count;++j) {
            const NpSegment *s=&elf->segments[j];
            if(address>=s->address && (uint64_t)address+4<=(uint64_t)s->address+s->filesz) {
                uint64_t offset=(uint64_t)s->offset+address-s->address;
                uint32_t word;
                if(offset+4>bytes)return "igr-wait-file-bounds";
                memcpy(&word,(const unsigned char *)image+(size_t)offset,4);
                if(word!=expected)return "igr-wait-expected";
                ++matches;
            }
        }
        if(matches!=1)return "igr-wait-segment";
        for(unsigned j=0;j<set->count;++j) {
            const NpPatch *p=&set->items[j];
            if(address<(uint64_t)p->address+p->length && p->address<(uint64_t)address+4)
                return "igr-wait-overlap";
        }
    }
    return NULL;
}
#endif
