/* Diagnostic call boundaries for the checksum-pinned POPS revision. */
#ifndef PSXCORE_CD_OBSERVE_H
#define PSXCORE_CD_OBSERVE_H
#include "psxcore_patch.h"
#include <string.h>
typedef struct {
    uint32_t generation,enters,returns;
    int32_t result;
    uint32_t arguments[4],saturated,reserved;
} NpCdCallObservation;
typedef struct {
    uint32_t magic,version,rows,stride;
    NpCdCallObservation calls[8];
} NpCdObservation;
_Static_assert(sizeof(NpCdCallObservation)==40,"CD call layout");
_Static_assert(sizeof(NpCdObservation)==336,"CD observation layout");
/* Independent records: read generation before/after and require equal even
 * values. The prefetch call return is submission only; its native callback
 * is a separate row. Version 2 rows 3/4 count native response-object retirement
 * after both object fields are cleared, without calling the IRQ helper again.
 * Object reuse does not acknowledge PS1 consumption of its response bytes.
 * Registers are raw observations, not necessarily valid arguments/results.
 * Equal counts cannot establish no new reads, DMA drain or save completion.
 * Same-site nesting is unsupported; no record is an ownership certificate. */
static const char *np_runtime_cd_observe_plan(const void *image,size_t bytes,
    const NpElf *elf,uintptr_t begin,uintptr_t end,const uintptr_t hooks[8],NpPatchSet *set) {
    static const uint32_t context[8][3]={
        {0x2101b4,0x0c08c0cc,0xafa00000},{0x210068,0x0c08c0cc,0xafa00000},
        {0x20ff14,0x0c08ba14,0x00000000},{0x20e7a4,0x03e00008,0x27bd0010},
        {0x20e70c,0x03e00008,0x27bd0010},{0x20e624,0x00a0f809,0xac400000},
        {0x20fb4c,0x0808398e,0x27bd0010},{0x21023c,0x0c084024,0x00000000}
    };
    static const uint32_t retirement[2][6]={
        {0x20e790,0xaf908888,0xdfbf0008,0xae000014,0xae000000,0xdfb00000},
        {0x20e6f8,0xaf908888,0xdfbf0008,0xae000014,0xae000000,0xdfb00000}
    };
    if(!image || !elf || !hooks || !set || !elf->count || elf->count>8 ||
       set->count>NP_PATCH_LIMIT-8)return "cd-observe-input";
    if(begin<0xf8000 || end<=begin || end>0xff000 || (begin&15) || (end&15))
        return "cd-observe-resident";
    for(unsigned i=0;i<2;++i) {
        unsigned matches=0;
        for(unsigned j=0;j<elf->count;++j) {
            const NpSegment *s=&elf->segments[j];
            if(retirement[i][0]>=s->address && (uint64_t)retirement[i][0]+20<=(uint64_t)s->address+s->filesz) {
                uint64_t off=(uint64_t)s->offset+retirement[i][0]-s->address;
                if(off+20>bytes)return "cd-observe-file-bounds";
                if(memcmp((const unsigned char *)image+(size_t)off,&retirement[i][1],20))
                    return "cd-observe-retirement";
                ++matches;
            }
        }
        if(matches!=1)return "cd-observe-segment";
        for(unsigned j=0;j<set->count;++j) {
            const NpPatch *p=&set->items[j];
            if(retirement[i][0]<(uint64_t)p->address+p->length &&
               p->address<(uint64_t)retirement[i][0]+20)return "cd-observe-overlap";
        }
    }
    for(unsigned i=0;i<8;++i) {
        if(hooks[i]<begin || hooks[i]>=end || (hooks[i]&3))return "cd-observe-hook";
        for(unsigned j=0;j<i;++j)if(hooks[i]==hooks[j])return "cd-observe-hook";
        unsigned matches=0;
        for(unsigned j=0;j<elf->count;++j) {
            const NpSegment *s=&elf->segments[j];
            if(context[i][0]>=s->address && (uint64_t)context[i][0]+8<=(uint64_t)s->address+s->filesz) {
                uint64_t off=(uint64_t)s->offset+context[i][0]-s->address;
                if(off+8>bytes)return "cd-observe-file-bounds";
                if(memcmp((const unsigned char *)image+(size_t)off,&context[i][1],8))
                    return "cd-observe-expected";
                ++matches;
            }
        }
        if(matches!=1)return "cd-observe-segment";
        for(unsigned j=0;j<set->count;++j) {
            const NpPatch *p=&set->items[j];
            if(context[i][0]<(uint64_t)p->address+p->length &&
               p->address<(uint64_t)context[i][0]+8)return "cd-observe-overlap";
        }
    }
    NpPatchSet candidate=*set;
    for(unsigned i=0;i<8;++i) {
        uint32_t replacement[2]={ (i==3 || i==4 || i==6?0x08000000u:0x0c000000u)|((uint32_t)hooks[i]>>2),context[i][2] };
        const char *error=np_patch_add(&candidate,context[i][0],&context[i][1],replacement,8);
        if(error)return error;
    }
    *set=candidate;
    return NULL;
}
#endif
