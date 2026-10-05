/* Explicit prepared-disc association, independent of library filenames. */
#ifndef PSXCORE_DISCMAP_H
#define PSXCORE_DISCMAP_H
#include <stddef.h>
#include <stdint.h>
#include "psxcore_launch.h"
#define NP_DISC_MAP_LIMIT 32u
#define NP_DISC_MAP_BYTES 4096u
typedef struct {
    unsigned char sha256[32];
    char partition[40],save_key[33],serial[16];
    uint32_t save_mode;
} NpDiscMapEntry;
typedef struct { unsigned count; NpDiscMapEntry entries[NP_DISC_MAP_LIMIT]; } NpDiscMap;
typedef struct { uint64_t bytes; unsigned char sha256[32],header_sha1[20]; } NpDiscIdentity;
/* Text: PSXCORE-DISCMAP 1, then five space-separated fields per line:
 * <64 hex digits> <hdd0:partition> <save_key> <isolated|shared> <serial|->.
 * LF/CRLF accepted. Final newline required. Duplicate hashes/partitions rejected.
 * Parsing/finding clear their output on failure; no storage operations occur. */
const char *np_disc_map_parse(const void *bytes,size_t size,NpDiscMap *out);
const char *np_disc_map_find(const NpDiscMap *map,const unsigned char sha256[32],NpDiscMapEntry *out);
/* Both identities must be freshly measured from complete, structurally checked
 * selected/prepared images. This compares them; it does not open or hash files. */
const char *np_disc_map_match(const NpDiscMapEntry *entry,const NpDiscIdentity *selected,
    const NpDiscIdentity *prepared);
const char *np_disc_map_argument(const NpDiscMapEntry *entry,char *out,size_t capacity);
/* Builds the v2 request from measured image data and explicit catalog ownership.
 * Storage/card/runtime readiness must be checked by the consuming frontend. */
const char *np_disc_map_launch(const NpDiscMapEntry *entry,const NpDiscIdentity *identity,
    uint32_t environment,const NpCompatOptions *options,NpLaunchRequest *out);
#endif
