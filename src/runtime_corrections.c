#include "psxcore_runtime.h"
#include "psxcore_compat.h"

const char *np_runtime_correct(uint32_t runtime, NpPatchSet *set) {
    if (runtime != NP_RUNTIME_KNOWN) return "core-runtime";
    if (!set || set->count > NP_PATCH_LIMIT - 2) return "patch-input";
    /* POPS texture-window handler: select GS repeat for an unmasked axis.
     * The masked-axis branch still selects region repeat. This shared default
     * fixes scrolling textures; it is not a title-specific sky override. */
    static const unsigned char expected[4] = {1,0,15,36};
    static const unsigned char corrected[4] = {0,0,15,36};
    const char *error = np_patch_add(set, 0x0021d6a4, expected, corrected, sizeof(expected));
    if (error) return error;
    /* PS1 RAM address zero is a valid linked GPU DMA start. Keep the preceding
     * 0xFFFFFF terminator check and the taken path's delay-slot store; replace
     * only the branch that otherwise treats zero as an empty transfer. */
    static const unsigned char dma_expected[32] = {
        0x20,0x00,0x02,0x8e, 0x24,0x20,0x43,0x00,
        0x04,0x00,0x83,0x50, 0x00,0x00,0xb0,0xdf,
        0x06,0x00,0x80,0x54, 0x20,0x01,0x00,0xae,
        0x00,0x00,0xb0,0xdf, 0x02,0x00,0x04,0x24
    };
    static const unsigned char dma_corrected[32] = {
        0x20,0x00,0x02,0x8e, 0x24,0x20,0x43,0x00,
        0x04,0x00,0x83,0x50, 0x00,0x00,0xb0,0xdf,
        0x06,0x00,0x00,0x10, 0x20,0x01,0x00,0xae,
        0x00,0x00,0xb0,0xdf, 0x02,0x00,0x04,0x24
    };
    return np_patch_add(set, 0x0020ac4c, dma_expected, dma_corrected, sizeof(dma_expected));
}
