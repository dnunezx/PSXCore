/* Original transactional plan for isolated gate fixtures. No runtime caller. */
#ifndef PSXCORE_CD_GATE_PATCH_H
#define PSXCORE_CD_GATE_PATCH_H
#include "cdgate.h"
#include "psxcore_patch.h"
#include <string.h>
static const char *np_cd_gate_expect(const void *image,size_t bytes,const NpElf *elf,
    const NpPatchSet *set,uint32_t address,uint32_t expected) {
    unsigned hits=0;
    for(unsigned i=0;i<elf->count;++i) {
        const NpSegment *s=&elf->segments[i];
        if(address>=s->address && (uint64_t)address+4<=(uint64_t)s->address+s->filesz) {
            uint64_t off=(uint64_t)s->offset+address-s->address;uint32_t word;
            if(off+4>bytes)return "cd-gate-file-bounds";
            memcpy(&word,(const unsigned char *)image+(size_t)off,4);
            if(word!=expected)return "cd-gate-expected";
            ++hits;
        }
    }
    if(hits!=1)return "cd-gate-segment";
    for(unsigned i=0;i<set->count;++i) {
        const NpPatch *p=&set->items[i];
        if(address<(uint64_t)p->address+p->length && p->address<(uint64_t)address+4)
            return "cd-gate-overlap";
    }
    return 0;
}
/* Caller first verifies the complete POPS fingerprint. Check all MMIO bank
 * dispatch entries, cycle cost, parameter saturation/flush and return cleanup
 * words used by this binding. Both redirects commit together or neither does.
 * State/hook bounds do not establish arena lifetime or permit a live launch. */
static const char *np_runtime_cd_gate_plan(const void *image,size_t bytes,const NpElf *elf,
    uintptr_t begin,uintptr_t code_end,uintptr_t end,uintptr_t state,
    uintptr_t write_hook,uintptr_t tail_hook,NpPatchSet *set) {
    static const uint32_t context[][2]={
        {0x20f9c0,0x27bdfff0},{0x20f9c4,0x8f8680a0},{0x20f9c8,0xffbf0000},
        {0x20f9cc,0x3084000f},{0x20f9d0,0x8cc202c4},{0x20f9d4,0x2442fff1},
        {0x20f9d8,0xacc202c4},{0x20f9dc,0x93838878},{0x20f9e0,0x30630003},
        {0x20f9e4,0x00031900},{0x20f9e8,0x00832025},{0x20f9ec,0x2c820034},
        {0x20fa30,0x2ca2001f},{0x20fa68,0x00a0f809},{0x20fa78,0xac40c218},
        {0x20fa88,0x8c830018},{0x20fa90,0xa065001c},{0x20fa98,0x2c620004},
        {0x20faac,0xac820018},{0x20fb20,0x30a20040},{0x20fb2c,0xac40c218},
        {0x20fb48,0xa0440000},{0x20fb4c,0x0808398e},{0x20fb50,0x27bd0010},
        {0x200f5c,0xaf80882c},{0x200f64,0xaf808830},
        {0x200fa4,0x00c0202d},{0x200fa8,0x1080000b},{0x200fac,0xdfb00000},
        {0x200fb0,0x8c820014},{0x200fb4,0},{0x200fb8,0x10400003},
        {0x200fbc,0x8c900000},{0x200fc0,0x0c080476},{0x200fc4,0},
        {0x200fc8,0x0200202d},{0x200fcc,0x5480fffa},{0x200fd0,0x8c820014},
        {0x200fd4,0xdfb00000},{0x200fd8,0xdfb10008},{0x200fdc,0xdfbf0010},
        {0x200fe0,0x03e00008},{0x200fe4,0x27bd0020}};
    if(!image || !elf || !set || !elf->count || elf->count>8 || set->count>NP_PATCH_LIMIT-2)
        return "cd-gate-input";
    if(begin!=0xeb000 || code_end<=begin || end<=code_end || end>0xec000 ||
       (code_end&15) || (end&15) || state<code_end || state>end ||
       end-state<sizeof(NpCdGate) || (state&15) || write_hook<begin || write_hook>=code_end ||
       tail_hook<begin || tail_hook>=code_end || (write_hook&3) || (tail_hook&3) || write_hook==tail_hook)
        return "cd-gate-arena";
    for(unsigned i=0;i<sizeof(context)/sizeof(context[0]);++i) {
        const char *error=np_cd_gate_expect(image,bytes,elf,set,context[i][0],context[i][1]);
        if(error)return error;
    }
    for(unsigned i=0;i<52;++i) {
        uint32_t handler=0x20fbc0;
        if(!(i&15))handler=0x20fa10;
        else if(i==1)handler=0x20fa30;else if(i==2)handler=0x20fa80;
        else if(i==3)handler=0x20fab0;else if(i==0x12)handler=0x20fb10;
        else if(i==0x13)handler=0x20fb20;else if(i==0x22)handler=0x20fb58;
        else if(i==0x23)handler=0x20fb68;else if(i==0x31)handler=0x20fb78;
        else if(i==0x32)handler=0x20fb88;else if(i==0x33)handler=0x20fb98;
        const char *error=np_cd_gate_expect(image,bytes,elf,set,0x4f9f20u+4u*i,handler);
        if(error)return error;
    }
    const uint32_t expected[2][2]={{0x27bdfff0,0x8f8680a0},{0x03e00008,0x27bd0020}};
    const uint32_t replacement[2][2]={{0x08000000u|((uint32_t)write_hook>>2),0},
        {0x08000000u|((uint32_t)tail_hook>>2),0x27bd0020}};
    NpPatchSet candidate=*set;
    const char *error=np_patch_add(&candidate,0x20f9c0,expected[0],replacement[0],8);
    if(!error)error=np_patch_add(&candidate,0x200fe0,expected[1],replacement[1],8);
    if(!error)*set=candidate;
    return error;
}
#endif
