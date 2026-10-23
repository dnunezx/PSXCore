/* Optional output-only adapter for the checksum-verified POPS revision. */
#ifndef PSXCORE_HDTV_H
#define PSXCORE_HDTV_H
#include "psxcore_compat.h"
/* Caller must verify the original runtime fingerprint before planning.
 * OFF leaves the plan untouched. ON verifies the complete control-path
 * anchors and queues four call redirects plus the output-height correction;
 * np_patch_apply remains the final
 * transactional instruction/bounds/overlap check before any write. */
const char *np_hdtv_plan(uint32_t runtime, uint32_t fixes,
    const void *image, size_t size, const NpElf *elf, NpPatchSet *set,
    uintptr_t begin, uintptr_t display, uintptr_t crt, uintptr_t height, uintptr_t end);
#endif
