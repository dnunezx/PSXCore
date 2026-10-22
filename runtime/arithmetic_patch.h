#ifndef NP_ARITHMETIC_PATCH_H
#define NP_ARITHMETIC_PATCH_H
#include "psxcore_patch.h"

/* Full runtime identity is verified by the caller. No overlapping native
 * IDCT hook: fast-path installation leaves this entry to the shared plan. */
static inline const char *np_arithmetic_plan(NpPatchSet *set,uintptr_t begin,
    uintptr_t compiler,uintptr_t gte,uintptr_t mdec,uintptr_t end) {
    if(!set || set->count>NP_PATCH_LIMIT-2)return "arithmetic-capacity";
    if(begin!=0xece00 || compiler!=begin || gte<=compiler || mdec<=gte ||
       end<=mdec || end>0xf0000 || ((begin|compiler|gte|mdec|end)&3))
        return "arithmetic-resident";
    static const uint32_t dispatch[2]={0x3202003f,0x2442ffff};
    static const uint32_t idct[4]={0x27bdfeb0,0xffb00100,0x03a0582d,0xffb10108};
    const uint32_t dispatch_patch[2]={0x08000000u|((uint32_t)compiler>>2),0};
    const uint32_t idct_patch[4]={0x08000000u|((uint32_t)mdec>>2),0,idct[2],idct[3]};
    const char *error=np_patch_add(set,0x2067d4,dispatch,dispatch_patch,sizeof(dispatch));
    return error?error:np_patch_add(set,0x20d3b8,idct,idct_patch,sizeof(idct));
}
#endif
