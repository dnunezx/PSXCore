/* Rayman 2 USA executable-loader invalidation adapter. */
#include "../gamepatches.h"
extern void np_loader_retire_compile(void),np_loader_retire(void);
extern char np_loader_retire_begin[],np_loader_retire_end[];
static const char *plan(const NpGamePatchContext *c,NpPatchSet *set) {
    const uintptr_t begin=(uintptr_t)np_loader_retire_begin;
    const uintptr_t end=(uintptr_t)np_loader_retire_end;
    const uintptr_t compile=(uintptr_t)np_loader_retire_compile;
    const uintptr_t retire=(uintptr_t)np_loader_retire;
    if(begin!=0xef000 || end<=begin || end>0xf0000 ||
       compile<begin || compile>=end || retire<begin || retire>=end ||
       (compile&3) || (retire&3))return "rayman2-helper-layout";
    const uint32_t sites[2]={0x20273c,0x202288};
    const uint32_t expected[2][2]={{0x0c08215a,0x0100282d},
        {0x2442e000,0x0043102b}};
    for(unsigned i=0;i<2;++i) {
        const char *error=np_game_patch_expect(c,sites[i],expected[i],8);
        if(error)return error;
    }
    const uint32_t replacement[2]={0x0c000000u|((uint32_t)compile>>2),0x2442dff0};
    for(unsigned i=0;i<2;++i) {
        const char *error=np_patch_add(set,sites[i],expected[i],replacement+i,4);
        if(error)return error;
    }
    return NULL;
}
const NpGamePatch np_game_patch_rayman2={
    "rayman2-us-loader","SLUS_012.35",NP_RUNTIME_KNOWN,NP_FIX_CODE_VALIDATION,plan
};
