/* Original streaming SHA-256 interface for bounded file reads. */
#ifndef PSXCORE_HASH_H
#define PSXCORE_HASH_H
#include <stddef.h>
#include <stdint.h>
typedef struct {
    uint32_t state[8];
    uint64_t bytes;
    size_t used;
    unsigned char buffer[64];
} NpSha256;
void np_sha256_init(NpSha256 *hash);
/* Caller supplies initialized state and valid storage; NULL is allowed at size 0. */
void np_sha256_update(NpSha256 *hash, const void *bytes, size_t size);
/* Consumes the state; initialize again before reusing it. */
void np_sha256_final(NpSha256 *hash, unsigned char digest[32]);
typedef struct {
    uint32_t state[5];
    uint64_t bytes;
    size_t used;
    unsigned char buffer[64];
} NpSha1;
/* SHA-1 is used only for the existing native first-256-KiB header job;
 * full image identity and mapping always use SHA-256. Same state contract. */
void np_sha1_init(NpSha1 *hash);
void np_sha1_update(NpSha1 *hash,const void *bytes,size_t size);
void np_sha1_final(NpSha1 *hash,unsigned char digest[20]);
#endif
