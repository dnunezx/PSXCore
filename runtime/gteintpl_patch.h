#ifndef NP_GTE_INTPL_PATCH_H
#define NP_GTE_INTPL_PATCH_H
#include "psxcore_patch.h"

/* The caller verifies the complete POPS revision before planning adapters.
 * The patch engine checks every expected byte, segment and overlap before
 * committing any runtime writes. Resident layout remains explicitly bounded. */
static inline const char *np_gte_intpl_plan(NpPatchSet *set,
    uintptr_t begin,uintptr_t compile,uintptr_t entry,uintptr_t clamp,uintptr_t end) {
    if(!set || set->count>NP_PATCH_LIMIT-2)return "gte-intpl-capacity";
    if(begin<0xf7e10 || begin!=compile || compile>=entry || entry>=clamp ||
       clamp>=end || end>0xf7fc0 || ((begin|compile|entry|clamp|end)&3))
        return "gte-intpl-resident";
    static const uint32_t emitter[4]={0x3c020021,0x10000065,0x24449770,0};
    static const uint32_t saturation[8]={0x24027fff,0x0100702a,0x0048782a,0x000e400b,
        0x004f400b,0x01cf7025,0x000e7600,0x002e0825};
    uint32_t emit_patch[4]={0x08000000u|((uint32_t)compile>>2),0,emitter[2],emitter[3]};
    uint32_t clamp_patch[8];
    for(unsigned i=0;i<8;++i)clamp_patch[i]=saturation[i];
    clamp_patch[0]=0x08000000u|((uint32_t)clamp>>2);
    clamp_patch[1]=saturation[0];
    const char *error=np_patch_add(set,0x206ac0,emitter,emit_patch,sizeof(emitter));
    return error?error:np_patch_add(set,0x209868,saturation,clamp_patch,sizeof(saturation));
}
#endif
