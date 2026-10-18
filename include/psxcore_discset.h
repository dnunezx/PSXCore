/* Original binary disc-set extension; existing file v6 request stays intact. */
#ifndef PSXCORE_DISCSET_H
#define PSXCORE_DISCSET_H
#include "psxcore_file.h"
#define NP_DISC_SET_MAGIC UINT32_C(0x5344504e)
#define NP_DISC_SET_LIMIT 4u
#define NP_FILE_CTL_DISC_SET UINT32_C(0x50584303)
typedef struct {
    char path[NP_FILE_PATH_BYTES];
    uint64_t image_bytes;
    unsigned char header_sha256[32],toc_sha256[32];
    uint32_t sectors,tracks;
} NpFileDiscEntry;
typedef struct {
    uint32_t magic;
    uint16_t version,size;
    uint32_t count,initial;
    unsigned char disc_volume[16],save_binding[32];
    NpFileDiscEntry entries[NP_DISC_SET_LIMIT];
} NpFileDiscSet;
_Static_assert(sizeof(NpFileDiscEntry)==336,"disc entry ABI");
_Static_assert(sizeof(NpFileDiscSet)==1408,"disc-set v1 ABI");
/* Binding covers the existing shared card pair, save endpoint and volume.
 * Registering a set cannot replace any of these save destinations. */
void np_file_disc_save_binding(const char (*cards)[NP_FILE_PATH_BYTES],uint32_t backend,
    uint32_t device,const unsigned char volume[16],unsigned char out[32]);
const char *np_file_disc_set_check(const NpFileDiscSet *set,size_t bytes,
    const char *initial_path,const unsigned char volume[16],const unsigned char save_binding[32]);
/* Route slot 0 is always the initially launched image; slot i selects
 * entries[(initial+i)%count]. Only exact /disc/disc0 through disc3 exist. */
int np_file_disc_slot(const char *name,unsigned *slot);
const NpFileDiscEntry *np_file_disc_entry(const NpFileDiscSet *set,unsigned slot);
const char *np_file_disc_set_route(const NpFileDiscSet *set,unsigned unit,const char *name,
    const char cards[2][NP_FILE_PATH_BYTES],char *out,size_t capacity);
/* Check 2048-byte measured header plus native TOC geometry; no file reads.
 * The frontend also owns full-image identity validation when forming a set. */
const char *np_file_disc_entry_check(const NpFileDiscEntry *entry,const unsigned char *header,size_t bytes);
#endif
