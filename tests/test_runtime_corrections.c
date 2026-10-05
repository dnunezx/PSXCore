/* Synthetic runtime segments; no Sony/game payload. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "psxcore_runtime.h"
#include "psxcore_compat.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    NpElf elf = {0}; elf.count = 1;
    elf.segments[0] = (NpSegment){0,0x21d6a4,8,8};
    NpPatchSet set = {0}, original;
    unsigned char image[8], before[8], unknown[32] = {1};
    const unsigned char expected[8] = {1,0,15,36,5,0,0,0};
    /* Unidentified games and every mode/platform still receive core wrapping. */
    for (uint32_t env = NP_ENV_PS2; env <= NP_ENV_PCSX2; ++env) {
        for (uint32_t mode = NP_COMPAT_AUTO; mode <= NP_COMPAT_CUSTOM; ++mode) {
            NpCompatOptions options = {mode,0,0,1}; NpCompatResult result;
            assert(!np_compat_resolve(1,env,"",unknown,&options,&result) && !result.fixes);
            memset(&set,0,sizeof(set)); memcpy(image,expected,8);
            assert(!np_runtime_correct(1,&set) && set.count == 1);
            assert(!np_compat_add_patches(1,env,result.fixes,&set) && set.count == 1);
            assert(!np_patch_apply(image,8,&elf,&set));
            assert(!image[0] && !memcmp(image+1,expected+1,7));
        }
    }
    /* A mismatching core instruction rejects the whole plan, including an
     * earlier otherwise valid write. No partial correction is permitted. */
    memset(&set,0,sizeof(set)); memcpy(image,expected,8); image[0] = 2;
    const unsigned char change[4] = {7,0,0,0};
    assert(!np_patch_add(&set,0x21d6a8,expected+4,change,4));
    assert(!np_runtime_correct(1,&set)); memcpy(before,image,8);
    assert(np_patch_apply(image,8,&elf,&set) && !memcmp(image,before,8));
    /* Duplicate/overlapping corrections and missing segments are rejected. */
    memcpy(image,expected,8); memcpy(before,image,8); set.count = 0;
    assert(!np_runtime_correct(1,&set) && !np_runtime_correct(1,&set));
    assert(np_patch_apply(image,8,&elf,&set) && !memcmp(image,before,8));
    set.count = 1; elf.segments[0].filesz = 3;
    assert(np_patch_apply(image,8,&elf,&set) && !memcmp(image,before,8));
    set.count = 0; original = set;
    assert(np_runtime_correct(2,&set) && !memcmp(&set,&original,sizeof(set)));
    assert(np_runtime_correct(1,NULL));
    set.count = NP_PATCH_LIMIT; original = set;
    assert(np_runtime_correct(1,&set) && !memcmp(&set,&original,sizeof(set)));
    puts("Shared runtime correction: all modes/platforms and atomic rejection passed");
    return 0;
}
