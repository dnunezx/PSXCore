/* Original diagnostic-only binding. No caller may infer a lease from records. */
#ifndef PSXCORE_CD_CONTEXT_H
#define PSXCORE_CD_CONTEXT_H
#include "cdgate_patch.h"
extern char np_cd_context[],np_cd_context_end[];
void np_cd_context_boot(void),np_cd_context_write(void),np_cd_context_tail(void);
static const char *np_runtime_cd_context_plan(const void *image,size_t bytes,const NpElf *elf,
    uintptr_t begin,uintptr_t code_end,uintptr_t end,uintptr_t state,
    uintptr_t boot,uintptr_t write,uintptr_t tail,uintptr_t record,uintptr_t record_end,NpPatchSet *set) {
    if(!set || boot<begin || boot>=code_end || (boot&3) || boot==write || boot==tail ||
       record<state+sizeof(NpCdGate) || (record&15) || record_end<record ||
       record_end-record!=768 || record_end>end)return "cd-context-arena";
    NpPatchSet candidate=*set;
    const char *error=np_runtime_cd_gate_plan(image,bytes,elf,begin,code_end,end,state,write,tail,&candidate);
    if(error)return error;
    if(candidate.count>=NP_PATCH_LIMIT)return "cd-context-capacity";
    error=np_cd_gate_expect(image,bytes,elf,&candidate,0x200198,0x0c08d7aa);
    if(!error)error=np_cd_gate_expect(image,bytes,elf,&candidate,0x20019c,0);
    const uint32_t expected[2]={0x0c08d7aa,0},replacement[2]={0x08000000u|((uint32_t)boot>>2),0};
    if(!error)error=np_patch_add(&candidate,0x200198,expected,replacement,8);
    if(!error)*set=candidate;
    return error;
}
#endif
