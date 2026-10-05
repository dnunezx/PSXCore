/* Synthetic images only; never requires firmware or a game. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "psxcore_launch.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void digest(const char *hex, unsigned char out[32]) {
    for (unsigned i = 0; i < 32; ++i) {
        unsigned byte;
        assert(sscanf(hex + i * 2, "%2x", &byte) == 1);
        out[i] = (unsigned char)byte;
    }
}
int main(void) {
    unsigned char doom[32], spyro[32], unknown[32];
    digest("2b51e078e6aa19df24aefde63dec7a978b91104f1c293c05939bc7eec6a8a0f4", doom);
    digest("816851b19ea6649fc6ab75e7eb2a79e339778079e8259c4492769d6f9bfb71ca", spyro);
    memcpy(unknown, doom, 32); unknown[31] ^= 1;
    NpCompatOptions o = {NP_COMPAT_AUTO, 0, 0, 1};
    NpCompatResult r;
    const uint32_t all = NP_FIX_LICENSE | NP_FIX_READBACK;
    assert(!np_compat_resolve(1,2,"SLUS_000.77",doom,&o,&r));
    assert(r.fixes == all && r.available == all && r.profile_revision == 3);
    assert(!strcmp(r.profile,"doom-us-pcsx2-experimental"));
    assert(!strcmp(r.game_name,"Doom") && strstr(r.disc_revision,"verified"));
    assert(!np_compat_resolve(1,2,"SCUS_942.28",spyro,&o,&r));
    assert(r.fixes == (NP_FIX_LICENSE | NP_FIX_READBACK));
    assert(!np_compat_resolve(1,2,"SLUS_000.77",unknown,&o,&r) && !r.fixes && !r.game_name);
    assert(!np_compat_resolve(1,2,"SLES_000.77",doom,&o,&r) && !r.fixes);
    assert(!np_compat_resolve(1,2,"",doom,&o,&r) && !r.fixes);
    assert(!np_compat_resolve(1,1,"SLUS_000.77",doom,&o,&r) && !r.fixes);
    assert(r.available == NP_FIX_LICENSE);
    o.mode = NP_COMPAT_DEFAULT;
    assert(!np_compat_resolve(1,2,"SLUS_000.77",doom,&o,&r) && !r.fixes && r.game_name);
    o.mode = NP_COMPAT_CUSTOM; o.disable = NP_FIX_READBACK;
    assert(!np_compat_resolve(1,2,"SLUS_000.77",doom,&o,&r));
    assert(r.fixes == NP_FIX_LICENSE);
    o.enable = NP_FIX_READBACK;
    assert(np_compat_resolve(1,2,"SLUS_000.77",doom,&o,&r) && !r.fixes);
    o.disable = 0;
    assert(np_compat_resolve(1,1,"SLUS_000.77",doom,&o,&r));
    o.enable = 0x80000000u;
    assert(np_compat_resolve(1,2,"SLUS_000.77",doom,&o,&r));
    o.mode = NP_COMPAT_AUTO; o.enable = NP_FIX_LICENSE;
    assert(np_compat_resolve(1,2,"SLUS_000.77",doom,&o,&r));
    o.enable = 0;
    assert(np_compat_resolve(2,2,"SLUS_000.77",doom,&o,&r));
    assert(np_compat_resolve(1,99,"SLUS_000.77",doom,&o,&r));

    NpLaunchRequest launch;
    np_launch_init(&launch); launch.environment = NP_ENV_PCSX2;
    strcpy(launch.disc_partition,"hdd0:PP.PSXC.DOOMDISC");
    strcpy(launch.save_partition,"hdd0:PP.PSXC.doom"); strcpy(launch.save_key,"doom");
    strcpy(launch.serial,"SLUS_000.77"); memcpy(launch.disc_sha256,doom,32);
    launch.header_sha1[0] = 1;
    assert(sizeof(launch) == 224 && !np_launch_check(&launch,sizeof(launch),&r));
    assert(r.fixes == all);
    launch.disc_sha256[31] ^= 1;
    assert(!np_launch_check(&launch,sizeof(launch),&r) && !r.fixes);

    NpPatchSet set = {0};
    /* Texture wrapping no longer comes from the optional profile catalog. */
    assert(!np_compat_add_patches(1,2,all,&set) && !set.count);
    assert(!np_compat_add_patches(1,2,0,&set) && !set.count);
    assert(np_compat_add_patches(1,2,4,&set) && !set.count);
    o.mode = NP_COMPAT_CUSTOM; o.enable = 0; o.disable = 4;
    assert(np_compat_resolve(1,2,"SLUS_000.77",doom,&o,&r));
    assert(np_compat_add_patches(2,2,all,&set) && !set.count);
    assert(np_compat_add_patches(1,99,all,&set) && !set.count);
    puts("Exact profile selection, launch contract and retired core-fix overrides passed");
    return 0;
}
