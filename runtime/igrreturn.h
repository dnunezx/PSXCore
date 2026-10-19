#ifndef PSXCORE_IGR_RETURN_H
#define PSXCORE_IGR_RETURN_H
#include "psxcore_patch.h"
#include <string.h>
/* POPStarter's existing native menu/quit -> launch adapter -> resident loader
 * route. The bootstrap's full POPS fingerprint is a separate prerequisite.
 * No new shortcut, forced save, drain or cooperative pause is installed. */
static const char *np_runtime_igr_return_plan(const void *image,size_t bytes,
    const NpElf *elf,uintptr_t entry,uintptr_t end,NpPatchSet *set) {
    static const uint32_t context[][2]={
        {0x20c07c,0x0c08a278},{0x20c080,0x26040002},
        {0x20c084,0x10400013},{0x20c088,0x8fa30000},
        {0x20c08c,0x0c0831bc},{0x20c090,0},
        {0x20c094,0x0c085114},{0x20c098,0},
        {0x20c0a4,0x0c08b44c},{0x20c0a8,0x24a59708},
        {0x20c0c4,0x1000fffa},{0x20c0c8,0},
        {0x2362b8,0x27bdffe0},{0x2362bc,0x00a0302d},
        {0x2362d4,0x0c08d82e},{0x2362d8,0x24841140},
        {0x2362dc,0x0c08d874},{0x2362e0,0},
        {0x2362fc,0x0808bb10},{0x236300,0x27bd0020},
        {0x2361d0,0x0808d5a0},{0x2361d4,0},
        {0x235680,0x27bdfff0},{0x235688,0x0c08bb20},
        {0x23569c,0x0c08d5b0},{0x2356ac,0x0c08bb24}
    };
    if(!image || !elf || !set || !elf->count || elf->count>8 ||
       set->count>NP_PATCH_LIMIT-2)return "igr-return-input";
    if(entry!=0x84000 || end<=entry || end>0xe0000 || (end&3))return "igr-return-layout";
    for(unsigned i=0;i<sizeof(context)/sizeof(*context);++i) {
        unsigned matches=0;
        for(unsigned j=0;j<elf->count;++j) {
            const NpSegment *s=&elf->segments[j];
            /* Reserve the whole loader plus its 0xe0000..0xe8000 stack. */
            if(s->address<0xe8000 && entry<(uint64_t)s->address+s->memsz)return "igr-return-pops-overlap";
            if(context[i][0]>=s->address && (uint64_t)context[i][0]+4<=(uint64_t)s->address+s->filesz) {
                uint64_t off=(uint64_t)s->offset+context[i][0]-s->address;
                if(off+4>bytes)return "igr-return-file-bounds";
                for(unsigned k=0;k<elf->count;++k) {
                    const NpSegment *other=&elf->segments[k];
                    if(k!=j && off<(uint64_t)other->offset+other->filesz &&
                       other->offset<off+4)return "igr-return-file-alias";
                }
                uint32_t word;memcpy(&word,(const unsigned char *)image+(size_t)off,4);
                if(word!=context[i][1])return "igr-return-expected";
                ++matches;
            }
        }
        if(matches!=1)return "igr-return-segment";
        for(unsigned j=0;j<set->count;++j) {
            const NpPatch *p=&set->items[j];
            if(context[i][0]<(uint64_t)p->address+p->length &&
               p->address<(uint64_t)context[i][0]+4)return "igr-return-overlap";
        }
    }
    NpPatchSet candidate=*set;
    const uint32_t expected[2]={0x0c08b44c,0x24a59708};
    const uint32_t replacement[2]={0x0c08d8ae,expected[1]};
    const char *error=np_patch_add(&candidate,0x20c0a4,expected,replacement,8);
    const uint32_t old_adapter[2]={0x0808bb10,0x27bd0020};
    const uint32_t new_adapter[2]={0x08000000u|((uint32_t)entry>>2),old_adapter[1]};
    if(!error)error=np_patch_add(&candidate,0x2362fc,old_adapter,new_adapter,8);
    if(error)return error;
    *set=candidate;return NULL;
}
#endif
