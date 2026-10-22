/* Startup registration for title-specific native runtime adapters.
 * This extends compatibility selection and reuses NpPatchSet; no frame polling. */
#ifndef NP_GAME_PATCHES_H
#define NP_GAME_PATCHES_H
#include "psxcore_launch.h"
#include "psxcore_patch.h"
typedef struct {
    const NpLaunchRequest *request;
    uint32_t fixes;
    const void *image;
    size_t image_size;
    const NpElf *elf;
} NpGamePatchContext;
typedef struct {
    const char *id, *serial;
    uint32_t runtime, excluded_fixes;
    /* Planning runs once at startup. Append checked writes only; do not
     * edit existing entries or game/runtime memory in a planner. */
    const char *(*plan)(const NpGamePatchContext *, NpPatchSet *);
} NpGamePatch;
const char *np_game_patches_plan(const NpGamePatchContext *context,
    NpPatchSet *set, unsigned *applied);
/* Verify the complete native instruction sequence, including delay slots.
 * The existing patch engine still checks writes and overlap before applying. */
const char *np_game_patch_expect(const NpGamePatchContext *context,
    uint32_t address, const void *expected, size_t length);
#endif
