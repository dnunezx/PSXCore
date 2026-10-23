/* Original PSXCore compatibility policy. No runtime or game payloads. */
#ifndef PSXCORE_COMPAT_H
#define PSXCORE_COMPAT_H
#include <stdint.h>
#include "psxcore_patch.h"
#include "psxcore_profile_catalog.h"
#define NP_RUNTIME_KNOWN 1u
#define NP_ENV_PS2 1u
#define NP_COMPAT_AUTO 0u
#define NP_COMPAT_DEFAULT 1u
#define NP_COMPAT_CUSTOM 2u
#define NP_FIX_LICENSE 1u
/* Code Validation: original exact decoded-instruction checker.
 * Selected per game by a profile or CUSTOM enable/disable overrides. */
#define NP_FIX_CODE_VALIDATION 8u
#define NP_CODE_VALIDATION_LABEL "Code Validation"
/* Retired environment value 2 and fix bit 2 remain reserved. */
/* Optional output-rate feedback; may lower pitch. Disabled unless selected. */
#define NP_FIX_ADAPTIVE_AUDIO 16u
#define NP_ADAPTIVE_AUDIO_LABEL "Adaptive Audio"
/* Experimental output compatibility; explicit CUSTOM selection only. */
#define NP_FIX_HDTV 32u
#define NP_HDTV_LABEL "HDTV Fix (480i / 576i)"
#define NP_FIX_ALL (NP_FIX_HDTV | NP_FIX_ADAPTIVE_AUDIO | NP_FIX_CODE_VALIDATION | NP_FIX_LICENSE | NP_FIX_CATALOG_MASK)
#define NP_PROFILE_REVISION 5u
typedef struct {
    uint32_t mode, enable, disable, diagnostics;
} NpCompatOptions;
typedef struct {
    uint32_t fixes, available, profile_revision;
    const char *profile;
    const char *game_name, *disc_revision;
} NpCompatResult;
/* serial/hash must describe the caller-validated native disc, not its filename.
 * Results are experimental. Available means implemented, not hardware-tested. */
const char *np_compat_resolve(uint32_t runtime, uint32_t environment,
    const char *serial, const unsigned char disc_sha256[32],
    const NpCompatOptions *options, NpCompatResult *out);
/* Queue reviewed guest-runtime writes transactionally. Caller verifies the
 * runtime fingerprint first. Built-in adapters are installed by the bootstrap. */
const char *np_compat_add_patches(uint32_t runtime, uint32_t environment,
    uint32_t fixes, NpPatchSet *set);
#endif
