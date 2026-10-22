#include "gamepatches.h"
#include <string.h>
#ifndef NP_RAYMAN_LOADER_RETIRE
#define NP_RAYMAN_LOADER_RETIRE 1
#endif
#if NP_RAYMAN_LOADER_RETIRE != 0 && NP_RAYMAN_LOADER_RETIRE != 1
#error "Rayman loader retirement requires a boolean"
#endif
#if NP_RAYMAN_LOADER_RETIRE
extern const NpGamePatch np_game_patch_rayman2;
#endif
/* Add game modules here; main.c contains no title matching or installers. */
static const NpGamePatch *const patches[]={
#if NP_RAYMAN_LOADER_RETIRE
    &np_game_patch_rayman2,
#endif
    NULL
};
const char *np_game_patch_expect(const NpGamePatchContext *c,
    uint32_t address, const void *expected, size_t length) {
    if(!c || !c->image || !c->elf || !expected || !length ||
       !c->elf->count || c->elf->count>8 ||
       (uint64_t)address+length>UINT32_MAX)return "game-patch-input";
    size_t offset=0;unsigned found=0;
    for(unsigned i=0;i<c->elf->count;++i) {
        const NpSegment *s=&c->elf->segments[i];
        if(address>=s->address && (uint64_t)address+length<=(uint64_t)s->address+s->filesz) {
            const uint64_t at=(uint64_t)s->offset+address-s->address;
            if(at>c->image_size || length>c->image_size-at)return "game-patch-bounds";
            offset=(size_t)at;++found;
        }
    }
    if(found!=1)return "game-patch-segment";
    return memcmp((const unsigned char *)c->image+offset,expected,length)?"game-patch-expected":NULL;
}
const char *np_game_patches_plan(const NpGamePatchContext *c,
    NpPatchSet *set,unsigned *applied) {
    if(applied)*applied=0;
    if(!c || !c->request || !c->image || !c->elf || !set || !applied ||
       set->count>NP_PATCH_LIMIT || !memchr(c->request->serial,0,sizeof(c->request->serial)))
        return "game-patch-input";
    const unsigned before=set->count;unsigned count=0;
    for(unsigned i=0;patches[i];++i) {
        const NpGamePatch *p=patches[i];
        if(c->request->runtime!=p->runtime || (c->fixes&p->excluded_fixes) ||
           strcmp(c->request->serial,p->serial))continue;
        const char *error=p->plan(c,set);
        if(error) {set->count=before;return error;}
        ++count;
    }
    *applied=count;return NULL;
}
