/* Original direct-file launch contract and storage policy, 2026. */
#ifndef PSXCORE_FILE_H
#define PSXCORE_FILE_H
#include "psxcore_launch.h"
#include "psxcore_vcd.h"
#define NP_FILE_MAGIC UINT32_C(0x334c504e)
#define NP_FILE_VERSION 3u
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
