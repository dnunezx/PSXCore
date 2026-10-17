/* Original direct-file launch contract and storage policy, 2026. */
#ifndef PSXCORE_FILE_H
#define PSXCORE_FILE_H
#include "psxcore_launch.h"
#include "psxcore_vcd.h"
#define NP_FILE_MAGIC UINT32_C(0x334c504e)
#define NP_FILE_VERSION 6u
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
#define NP_FILE_HEADER_HASH_BYTES 2048u
#define NP_FILE_CARD0 "/POPS/ps1emu/card0"
#define NP_FILE_CARD1 "/POPS/ps1emu/card1"
#define NP_FILE_CTL_FLUSH 0x50584301u
#define NP_FILE_CTL_STATUS 0x50584302u
/* The runtime uses the selected existing pair; frontends may provision cards. */
typedef struct {
    uint32_t backend, device, required, reserved;
} NpFileEndpoint;
typedef struct {
    uint32_t magic;
    uint16_t version, size;
    uint32_t runtime, environment;
    NpCompatOptions options;
    uint64_t image_bytes;
    NpFileEndpoint disc, saves;
    char disc_path[NP_FILE_PATH_BYTES],runtime_root[128];
    char cards[2][NP_FILE_PATH_BYTES],serial[16];
    /* Bounded header/session check; optional precomputed hash for profiles. */
    unsigned char header_sha1[20],disc_sha256[32];
    unsigned char disc_volume[16],save_volume[16];
    uint32_t reserved;
} NpFileLaunchRequest;
_Static_assert(sizeof(NpFileLaunchRequest)==1072, "file v6 ABI");
_Static_assert(sizeof(NpFileEndpoint)==16, "file endpoint ABI");
/* Volume IDs bind removable storage, never games to cards. Split stores need
 * distinct IDs. A single store may use a zero ID. */
typedef struct {
    uint32_t magic,version;
    unsigned char id[16],sha256[32],reserved[8];
} NpFileVolumeMarker;
_Static_assert(sizeof(NpFileVolumeMarker)==64, "volume marker ABI");
int np_file_volume_id_parse(const char *hex,unsigned char out[16]);
void np_file_volume_id_text(const unsigned char id[16],char out[33]);
int np_file_volume_id_present(const unsigned char id[16]);
const char *np_file_volume_marker_check(const void *bytes,size_t size,unsigned char id[16]);
void np_file_volume_marker_init(NpFileVolumeMarker *marker,const unsigned char id[16]);
/* A known contract backend may still be unavailable in a particular runtime. */
const char *np_file_backend_driver(uint32_t backend);
const char *np_file_endpoint_check(const NpFileEndpoint *endpoint,int saves);
void np_file_launch_init(NpFileLaunchRequest *request);
const char *np_file_launch_check(const void *bytes,size_t size,NpCompatResult *resolved);
/* Projection for compatibility guards; Sony uses hdd0:__common for saves. */
const char *np_file_launch_native(const NpFileLaunchRequest *file,NpLaunchRequest *native);
int np_file_path_valid(const char *path,size_t capacity,int directory);
/* Fixed POPS names map to the frontend-selected shared cards; .bak writes
 * remain Sony's save transaction. The frontend owns card creation/deletion. */
int np_file_card_path_valid(const char *path);
int np_file_card_pair_valid(const char cards[2][NP_FILE_PATH_BYTES]);
const char *np_file_route(unsigned unit,const char *name,const char *disc,
    const char cards[2][NP_FILE_PATH_BYTES],char *out,size_t capacity);
int np_file_card_header_valid(const unsigned char *bytes,size_t size);
int np_file_card_format_frame(unsigned frame,unsigned char bytes[128]);
/* Bounds/alignment used by the native IOPCD file reader. */
const char *np_file_read_span(uint64_t size,uint32_t lba,uint32_t count,
    uint32_t *offset,uint32_t *length,uint32_t *alignment);
#endif
