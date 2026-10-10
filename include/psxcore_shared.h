/* Shared PS1 card-pair identity, independent of disc filenames. */
#ifndef PSXCORE_SHARED_H
#define PSXCORE_SHARED_H
#include <stddef.h>
#include <stdint.h>
#define NP_SHARED_MAGIC UINT32_C(0x5343504e)
#define NP_SHARED_VERSION 1u
#define NP_SHARED_MEMBERS 32u
#define NP_SHARED_NATIVE 1u
#define NP_SHARED_FILE 2u
typedef struct {
    uint32_t magic;
    uint16_t version,size;
    uint32_t count,storage;
    char key[33],name[63];
    unsigned char volume[16],members[NP_SHARED_MEMBERS][32],sha256[32];
} NpSharedSet;
_Static_assert(sizeof(NpSharedSet)==1184,"shared card set layout");
/* Canonical zero-padded strings, no path separators; native keys <=20 chars. */
int np_shared_key_valid(const char *key,uint32_t storage);
const char *np_shared_init(NpSharedSet *set,uint32_t storage,const char *key,
    const char *name,const unsigned char volume[16]);
const char *np_shared_check(const void *bytes,size_t size);
const char *np_shared_match(const NpSharedSet *set,uint32_t storage,const char *key,
    const unsigned char volume[16],const unsigned char disc[32]);
/* Idempotent enrollment/removal; preserves set identity and cards. */
const char *np_shared_member(NpSharedSet *set,const unsigned char disc[32],int enroll);
#endif
