/* Read-only storage identity retained by the bootstrap across IOP resets. */
#ifndef PSXCORE_VOLUME_H
#define PSXCORE_VOLUME_H
#include <stdint.h>
#define NP_VOLUME_IOCTL 0x50585601u
#define NP_VOLUME_MAGIC 0x50585649u
typedef struct {
    uint32_t magic;
    uint16_t version,size;
    uint64_t offset,sectors;
    uint32_t sector_size,reserved;
    unsigned char boot_sha256[32];
} NpVolumeIdentity;
_Static_assert(sizeof(NpVolumeIdentity)==64,"volume identity wire ABI");
int np_volume_identity_build(NpVolumeIdentity *out,unsigned char sector[512],
    uint64_t offset,uint64_t sectors,uint32_t sector_size);
int np_volume_identity_valid(const NpVolumeIdentity *id);
#endif
