/* Shared corrections to the checksum-verified POPS runtime, independent of games. */
#ifndef PSXCORE_RUNTIME_H
#define PSXCORE_RUNTIME_H
#include "psxcore_patch.h"
#define NP_RUNTIME_CORRECTION_REVISION 22u
/* Queue core corrections after runtime identity/layout verification, before
 * np_patch_apply. No game identity, environment or compatibility options apply.
 * Supplied files remain unchanged; application is guarded and transactional. */
const char *np_runtime_correct(uint32_t runtime, NpPatchSet *set);
#endif
