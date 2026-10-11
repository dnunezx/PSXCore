/* Original native-partition launch contract. Version 1 dry-run ABI is separate. */
#ifndef PSXCORE_LAUNCH_H
#define PSXCORE_LAUNCH_H
#include <stddef.h>
#include <stdint.h>
#include "psxcore_compat.h"
#define NP_LAUNCH_MAGIC UINT32_C(0x324c504e)
#define NP_LAUNCH_VERSION 3u
#define NP_SONY_SAVE_PARTITION "hdd0:__common"
typedef struct {
    uint32_t magic;
    uint16_t version, size;
    uint32_t runtime, environment, reserved[2];
    NpCompatOptions options;
    char disc_partition[40], save_partition[40], serial[16];
    unsigned char disc_sha256[32], header_sha1[20];
    unsigned char padding[4];
} NpLaunchRequest;
_Static_assert(sizeof(NpLaunchRequest) == 192, "native launch layout changed");
/* Sony's shared pair lives in hdd0:__common:/ps1emu/card0 and card1.
 * Frontend manages existing cards. No private save ownership or migration.
 * Full-disc hashes are optional precomputed compatibility hints, never needed
 * for card selection. Native requests supply SHA-1 of the first 256 KiB for
 * Sony's bounded boot check. File requests project their 2 KiB header instead.
 * Integer fields use little-endian encoding on PS2. */
void np_launch_init(NpLaunchRequest *request);
const char *np_launch_check(const void *bytes, size_t length, NpCompatResult *resolved);
const char *np_launch_argument(const NpLaunchRequest *request, char *out, size_t capacity);
#endif
