#include "psxcore_runtime.h"
#include "psxcore_compat.h"

const char *np_runtime_correct(uint32_t runtime, NpPatchSet *set) {
    if (runtime != NP_RUNTIME_KNOWN) return "core-runtime";
    /* POPS texture-window handler: select GS repeat for an unmasked axis.
     * The masked-axis branch still selects region repeat. This shared default
     * fixes scrolling textures; it is not a title-specific sky override. */
    static const unsigned char expected[4] = {1,0,15,36};
    static const unsigned char corrected[4] = {0,0,15,36};
    return np_patch_add(set, 0x0021d6a4, expected, corrected, sizeof(expected));
}
