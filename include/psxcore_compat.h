/* Original PSXCore compatibility policy. No runtime or game payloads. */
#ifndef PSXCORE_COMPAT_H
#define PSXCORE_COMPAT_H
#include <stdint.h>
#define NP_RUNTIME_KNOWN 1u
#define NP_ENV_PS2 1u
#define NP_ENV_PCSX2 2u
#define NP_COMPAT_AUTO 0u
#define NP_COMPAT_DEFAULT 1u
#define NP_COMPAT_CUSTOM 2u
#define NP_FIX_LICENSE 1u
#define NP_FIX_READBACK 2u
#define NP_FIX_ALL (NP_FIX_LICENSE | NP_FIX_READBACK)
#define NP_PROFILE_REVISION 1u
typedef struct {
    uint32_t mode, enable, disable, diagnostics;
} NpCompatOptions;
typedef struct {
    uint32_t fixes, available, profile_revision;
    const char *profile;
} NpCompatResult;
/* serial/hash must describe the caller-validated native disc, not its filename.
 * Results are experimental. Available means implemented, not hardware-tested. */
const char *np_compat_resolve(uint32_t runtime, uint32_t environment,
    const char *serial, const unsigned char disc_sha256[32],
    const NpCompatOptions *options, NpCompatResult *out);
#endif
