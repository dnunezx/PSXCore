#ifndef PSXCORE_IGR_QUIT_H
#define PSXCORE_IGR_QUIT_H
#include "psxcore_patch.h"
#include <string.h>
/* Diagnostic only. A confirmed native menu quit is recorded and deferred.
 * Raw flags/alarm are not a coherent cross-worker snapshot or drain proof.
 * Read matching even generations; single native input owner, no nesting. */
typedef struct {
    uint32_t magic,version,generation,requests,caller_sp,card_flags[2];
    int32_t alarm_id;
} NpIgrQuitObservation;
_Static_assert(sizeof(NpIgrQuitObservation)==32,"IGR quit observation layout");
static const char *np_runtime_igr_quit_plan(const void *image,size_t bytes,
    const NpElf *elf,uintptr_t begin,uintptr_t end,uintptr_t hook,NpPatchSet *set) {
    static const uint32_t context[][2]={
        {0x20c07c,0x0c08a278},{0x20c080,0x26040002},
        {0x20c084,0x10400013},{0x20c088,0x8fa30000},
        {0x20c08c,0x0c0831bc},{0x20c090,0x00000000},
        {0x20c094,0x0c085114},{0x20c098,0x00000000},
        {0x20c0a4,0x0c08b44c},{0x20c0a8,0x24a59708},
        {0x20c0c4,0x1000fffa},{0x20c0c8,0x00000000},{0x20c0d0,0x8fa30000},
        {0x20c0d4,0x26730001},{0x20c0d8,0x2a620002},
        {0x20c0dc,0x24630090},{0x20c0e0,0x1440fef3},
        {0x20c0e4,0xafa30000}
    };
    if(!image || !elf || !set || !elf->count || elf->count>8 ||
       set->count>=NP_PATCH_LIMIT)return "igr-quit-input";
    if(begin<0xf8000 || end<=begin || end>0xff000 || (begin&15) || (end&3) ||
       hook<begin || hook>=end || (hook&3))return "igr-quit-resident";
    for(unsigned i=0;i<sizeof(context)/sizeof(context[0]);++i) {
        unsigned matches=0;
        for(unsigned j=0;j<elf->count;++j) {
            const NpSegment *s=&elf->segments[j];
            if(context[i][0]>=s->address && (uint64_t)context[i][0]+4<=(uint64_t)s->address+s->filesz) {
                uint64_t offset=(uint64_t)s->offset+context[i][0]-s->address;
                uint32_t word;
                if(offset+4>bytes)return "igr-quit-file-bounds";
                for(unsigned k=0;k<elf->count;++k) {
                    const NpSegment *other=&elf->segments[k];
                    if(k!=j && offset<(uint64_t)other->offset+other->filesz &&
                       other->offset<offset+4)return "igr-quit-file-alias";
                }
                memcpy(&word,(const unsigned char *)image+(size_t)offset,4);
                if(word!=context[i][1])return "igr-quit-expected";
                ++matches;
            }
        }
        if(matches!=1)return "igr-quit-segment";
        for(unsigned j=0;j<set->count;++j) {
            const NpPatch *p=&set->items[j];
            if(context[i][0]<(uint64_t)p->address+p->length &&
               p->address<(uint64_t)context[i][0]+4)return "igr-quit-overlap";
        }
    }
    const uint32_t expected[2]={0x10400013,0x8fa30000};
    const uint32_t replacement[2]={0x08000000u|((uint32_t)hook>>2),expected[1]};
    NpPatchSet candidate=*set;
    const char *error=np_patch_add(&candidate,0x20c084,expected,replacement,8);
    if(error)return error;
    *set=candidate;
    return NULL;
}
#endif
