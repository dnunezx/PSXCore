/* Original direct-file launch contract and storage policy, 2026. */
#ifndef PSXCORE_FILE_H
#define PSXCORE_FILE_H
#include "psxcore_launch.h"
#include "psxcore_vcd.h"
#define NP_FILE_MAGIC UINT32_C(0x334c504e)
#define NP_FILE_VERSION 3u
#define NP_FILE_VERSION_4 4u
#define NP_FILE_VERSION_5 5u
#define NP_FILE_VOLUME_MARKER "/POPS/PSXCORE.VOLUME"
#define NP_FILE_VOLUME_MAGIC UINT32_C(0x5658504e)
#define NP_FILE_BACKEND_ATA 1u
#define NP_FILE_BACKEND_USB 2u
#define NP_FILE_BACKEND_MX4SIO 3u
#define NP_FILE_BACKEND_MMCE 4u
#define NP_FILE_CAP_READ 1u
#define NP_FILE_CAP_SEEK 2u
#define NP_FILE_CAP_WRITE 4u
#define NP_FILE_CAP_RENAME 8u
/* Flush callback completion, not acknowledged device durability. */
#define NP_FILE_CAP_FLUSH 16u
#define NP_FILE_CAP_DURABLE 32u
#define NP_FILE_CAP_ALL 63u
#define NP_FILE_DISC_CAPS (NP_FILE_CAP_READ|NP_FILE_CAP_SEEK)
#define NP_FILE_SAVE_CAPS (NP_FILE_DISC_CAPS|NP_FILE_CAP_WRITE|NP_FILE_CAP_RENAME|NP_FILE_CAP_FLUSH)
#define NP_FILE_PATH_BYTES 256u
#define NP_FILE_CARD_BYTES 131072u
#define NP_FILE_CTL_FLUSH 0x50584301u
#define NP_FILE_CTL_STATUS 0x50584302u
typedef struct {
    uint32_t magic;
    uint16_t version, size;
    uint32_t runtime, environment, save_mode, reserved;
    NpCompatOptions options;
    uint64_t image_bytes;
    char driver[4];
    uint32_t device;
    char disc_path[NP_FILE_PATH_BYTES];
    char runtime_root[128];
    char save_key[33], serial[16];
    unsigned char padding[7];
    unsigned char disc_sha256[32], header_sha1[20];
    uint32_t reserved_tail;
} NpFileLaunchRequest;
_Static_assert(sizeof(NpFileLaunchRequest)==552, "file request ABI");
/* Device number is a discovery constraint, not persistent media identity.
 * Mount enumeration is never serialized. Content is reverified after reset.
 * MMCE has its own file namespace; it is not a BDM block device. */
typedef struct {
    uint32_t backend, device, required, reserved;
} NpFileEndpoint;
typedef struct {
    uint32_t magic;
    uint16_t version, size;
    uint32_t runtime, environment, save_mode, reserved;
    NpCompatOptions options;
    uint64_t image_bytes;
    char disc_path[NP_FILE_PATH_BYTES];
    char runtime_root[128];
    char save_key[33], serial[16];
    unsigned char padding[7];
    unsigned char disc_sha256[32], header_sha1[20];
    uint32_t reserved_tail;
    NpFileEndpoint disc, saves;
} NpFileLaunchRequestV4;
_Static_assert(sizeof(NpFileEndpoint)==16, "file endpoint ABI");
_Static_assert(sizeof(NpFileLaunchRequestV4)==576, "file v4 ABI");
/* Application volume identity. Clone copies must be given distinct IDs.
 * Zero IDs preserve v3/v4 single-volume discovery; split stores require IDs. */
typedef struct {
    NpFileLaunchRequestV4 launch;
    unsigned char disc_volume[16],save_volume[16];
} NpFileLaunchRequestV5;
typedef struct {
    uint32_t magic,version;
    unsigned char id[16],sha256[32],reserved[8];
} NpFileVolumeMarker;
_Static_assert(sizeof(NpFileLaunchRequestV5)==608, "file v5 ABI");
_Static_assert(sizeof(NpFileVolumeMarker)==64, "volume marker ABI");
int np_file_volume_id_parse(const char *hex,unsigned char out[16]);
void np_file_volume_id_text(const unsigned char id[16],char out[33]);
int np_file_volume_id_present(const unsigned char id[16]);
const char *np_file_volume_marker_check(const void *bytes,size_t size,unsigned char id[16]);
void np_file_volume_marker_init(NpFileVolumeMarker *marker,const unsigned char id[16]);
void np_file_launch_init_v5(NpFileLaunchRequestV5 *request);
const char *np_file_launch_check_v5(const void *bytes,size_t size,NpCompatResult *resolved);
const char *np_file_launch_decode_v5(const void *bytes,size_t size,NpFileLaunchRequestV5 *out,NpCompatResult *resolved);
void np_file_launch_init_v4(NpFileLaunchRequestV4 *request);
/* Decode v3 ATA requests or v4 into a checked v4 contract. No I/O or writes.
 * Output is cleared on rejection; input and output must not overlap. */
const char *np_file_launch_decode(const void *bytes,size_t size,
    NpFileLaunchRequestV4 *out,NpCompatResult *resolved);
const char *np_file_launch_check_v4(const void *bytes,size_t size,NpCompatResult *resolved);
const char *np_file_launch_native_v4(const NpFileLaunchRequestV4 *file,NpLaunchRequest *native);
/* A known contract backend may still be unavailable in a particular runtime. */
const char *np_file_backend_driver(uint32_t backend);
const char *np_file_endpoint_check(const NpFileEndpoint *endpoint,int saves);
void np_file_launch_init(NpFileLaunchRequest *request);
const char *np_file_launch_check(const void *bytes,size_t size,NpCompatResult *resolved);
/* Projection for existing compatibility/save guards, never an actual APA target. */
const char *np_file_launch_native(const NpFileLaunchRequest *file,NpLaunchRequest *native);
int np_file_path_valid(const char *path,size_t capacity,int directory);
/* pfs0:/disc/disc0 -> selected read-only VCD; pfs1:/ps1emu/cardN[.bak]
 * -> /POPS/SAVES/<key>/cardN[.bak]. Other paths/units are rejected. */
const char *np_file_route(unsigned unit,const char *name,const char *disc,
    const char *key,char *out,size_t capacity);
int np_file_card_header_valid(const unsigned char *bytes,size_t size);
void np_file_card_frame(unsigned frame,unsigned char bytes[128]);
/* Bounds/alignment used by the native IOPCD file reader. */
const char *np_file_read_span(uint64_t size,uint32_t lba,uint32_t count,
    uint32_t *offset,uint32_t *length,uint32_t *alignment);
#endif
