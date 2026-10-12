#include "psxcore_compat.h"
#include <string.h>

typedef struct {
    uint32_t runtime, environments, fixes;
    const char *serial;
    unsigned char sha256[32];
    const char *id, *game, *revision;
} NpCompatProfile;
typedef struct {
    uint32_t fix, environments, address, length;
    unsigned char expected[NP_PATCH_BYTES], replacement[NP_PATCH_BYTES];
} NpCatalogPatch;
#include "compat_profiles.generated.h"

static uint32_t available(uint32_t environment) {
    uint32_t mask = NP_FIX_LICENSE;
    for (unsigned i = 0; i < sizeof(s_catalogPatches)/sizeof(s_catalogPatches[0]); ++i)
        if (s_catalogPatches[i].environments & (1u << environment))
            mask |= s_catalogPatches[i].fix;
    return mask;
}

const char *np_compat_resolve(uint32_t runtime, uint32_t environment,
    const char *serial, const unsigned char disc_sha256[32],
    const NpCompatOptions *options, NpCompatResult *out) {
    NpCompatResult result = {0, 0, NP_PROFILE_REVISION, "baseline", NULL, NULL};
    if (out) memset(out, 0, sizeof(*out));
    if (!serial || !disc_sha256 || !options || !out) return "compat-input";
    if (runtime != NP_RUNTIME_KNOWN) return "compat-runtime";
    if (environment != NP_ENV_PS2) return "compat-environment";
    if (options->mode > NP_COMPAT_CUSTOM || options->diagnostics > 1) return "compat-options";
    if ((options->enable | options->disable) & ~NP_FIX_ALL) return "compat-unknown-fix";
    if (options->enable & options->disable) return "compat-conflicting-overrides";
    if (options->mode != NP_COMPAT_CUSTOM && (options->enable | options->disable))
        return "compat-overrides-require-custom";
    result.available = available(environment);
    if (options->enable & ~result.available) return "compat-fix-unavailable";
    for (unsigned i = 0; i < sizeof(s_profiles)/sizeof(s_profiles[0]); ++i) {
        const NpCompatProfile *p = &s_profiles[i];
        if (p->runtime != runtime || !(p->environments & (1u << environment)) ||
            strcmp(serial, p->serial) || memcmp(disc_sha256, p->sha256, 32)) continue;
        result.game_name = p->game;
        result.disc_revision = p->revision;
        if (options->mode != NP_COMPAT_DEFAULT) {
            result.profile = p->id;
            result.fixes = p->fixes;
        }
        break;
    }
    if (options->mode == NP_COMPAT_CUSTOM)
        result.fixes = (result.fixes | options->enable) & ~options->disable;
    *out = result;
    return NULL;
}

const char *np_compat_add_patches(uint32_t runtime, uint32_t environment,
    uint32_t fixes, NpPatchSet *set) {
    if (!set || set->count > NP_PATCH_LIMIT) return "compat-patch-input";
    if (runtime != NP_RUNTIME_KNOWN) return "compat-runtime";
    if (environment != NP_ENV_PS2) return "compat-environment";
    if (fixes & ~available(environment)) return "compat-fix-unavailable";
    NpPatchSet next = *set;
    for (unsigned i = 0; i < sizeof(s_catalogPatches)/sizeof(s_catalogPatches[0]); ++i) {
        const NpCatalogPatch *p = &s_catalogPatches[i];
        if (!(fixes & p->fix) || !(p->environments & (1u << environment))) continue;
        const char *error = np_patch_add(&next, p->address, p->expected, p->replacement, p->length);
        if (error) return error;
    }
    *set = next;
    return NULL;
}
