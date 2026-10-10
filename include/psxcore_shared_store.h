/* EE fileXio operations on an explicitly mounted card-pair directory.
 * The caller owns mounts and verifies their persistent storage identity.
 * No partition allocation, save conversion, automatic repair or card deletion. */
#ifndef PSXCORE_SHARED_STORE_H
#define PSXCORE_SHARED_STORE_H
#include "psxcore_shared.h"
const char *np_shared_store_read(const char *base,NpSharedSet *set);
/* Base must already exist and have no card/ownership/recovery files.
 * NULL source creates blank cards; otherwise copy and verify both source cards.
 * Failure preserves staging files and requires explicit recovery. */
const char *np_shared_store_create(const char *base,const NpSharedSet *set,const char *source);
/* Compare current metadata with expected before replacing; cards are untouched. */
const char *np_shared_store_update(const char *base,const NpSharedSet *expected,const NpSharedSet *next);
#endif
