/* Original native-partition launch contract. Version 1 dry-run ABI is separate. */
#ifndef PSXCORE_LAUNCH_H
#define PSXCORE_LAUNCH_H
#include <stddef.h>
#include <stdint.h>
#include "psxcore_compat.h"
#define NP_LAUNCH_MAGIC UINT32_C(0x324c504e)
#define NP_LAUNCH_VERSION 2u
#define NP_SAVE_ISOLATED 0u
#define NP_SAVE_SHARED 1u
typedef struct {
    uint32_t magic;
    uint16_t version, size;
    uint32_t runtime, environment, save_mode, reserved;
    NpCompatOptions options;
    char disc_partition[40], save_partition[40], save_key[33], serial[16];
    unsigned char disc_sha256[32], header_sha1[20];
    unsigned char padding[3];
} NpLaunchRequest;
_Static_assert(sizeof(NpLaunchRequest) == 224, "native launch layout changed");
/* Initializes the fixed header; caller fills selected partitions/identity/hash.
 * Save partition must be hdd0:PP.PSXC.<save_key>, key = 1..20 ASCII alphanumerics,
 * '_' or '-'. The frontend persists ownership and explicitly shared identities.
 * Disc partition must contain the validated native /disc/disc0 image.
 * Integer fields use little-endian encoding on PS2. */
void np_launch_init(NpLaunchRequest *request);
const char *np_launch_check(const void *bytes, size_t length, NpCompatResult *resolved);
const char *np_launch_argument(const NpLaunchRequest *request, char *out, size_t capacity);
#endif
