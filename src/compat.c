#include "psxcore_compat.h"
#include <string.h>

const char *np_compat_resolve(uint32_t runtime, uint32_t environment,
    const char *serial, const unsigned char disc_sha256[32],
    const NpCompatOptions *options, NpCompatResult *out) {
    /* Exact converted Spyro fixture from the recorded gameplay checkpoint.
     * Its fingerprint also distinguishes region, revision and conversion. */
    static const unsigned char spyro[32] = {
        0x81,0x68,0x51,0xb1,0x9e,0xa6,0x64,0x9f,0xc6,0xab,0x75,0xe7,0xeb,0x2a,0x79,0xe3,
        0x39,0x77,0x80,0x79,0xe8,0x25,0x9c,0x44,0x92,0x76,0x9d,0x6f,0x9b,0xfb,0x71,0xca
    };
    NpCompatResult result = {0, NP_FIX_LICENSE, NP_PROFILE_REVISION, "baseline"};
    if (out) memset(out, 0, sizeof(*out));
    if (!serial || !disc_sha256 || !options || !out) return "compat-input";
    if (runtime != NP_RUNTIME_KNOWN) return "compat-runtime";
    if (environment != NP_ENV_PS2 && environment != NP_ENV_PCSX2) return "compat-environment";
    if (options->mode > NP_COMPAT_CUSTOM || options->diagnostics > 1) return "compat-options";
    if ((options->enable | options->disable) & ~NP_FIX_ALL) return "compat-unknown-fix";
    if (options->enable & options->disable) return "compat-conflicting-overrides";
    if (options->mode != NP_COMPAT_CUSTOM && (options->enable | options->disable))
        return "compat-overrides-require-custom";
    if (environment == NP_ENV_PCSX2) result.available |= NP_FIX_READBACK;
    if (options->enable & ~result.available) return "compat-fix-unavailable";
    /* Emulator evidence must never automatically select a console workaround. */
    if (options->mode != NP_COMPAT_DEFAULT && environment == NP_ENV_PCSX2 &&
        !strcmp(serial, "SCUS_942.28") && !memcmp(disc_sha256, spyro, sizeof(spyro))) {
        result.profile = "spyro-us-pcsx2-experimental";
        result.fixes = NP_FIX_LICENSE | NP_FIX_READBACK;
    }
    if (options->mode == NP_COMPAT_CUSTOM)
        result.fixes = (result.fixes | options->enable) & ~options->disable;
    *out = result;
    return NULL;
}
