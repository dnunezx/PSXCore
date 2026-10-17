#include "psxcore_runtime.h"
#include "psxcore_compat.h"

const char *np_runtime_correct(uint32_t runtime, NpPatchSet *set) {
    if (runtime != NP_RUNTIME_KNOWN) return "core-runtime";
    if (!set || set->count > NP_PATCH_LIMIT - 6) return "patch-input";
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
    error = np_patch_add(set, 0x0020ac4c, dma_expected, dma_corrected, sizeof(dma_expected));
    if (error) return error;
    /* CD-ROM Play ignores SetMode bit 0. POPS otherwise advances the audio
     * track without submitting any CDDA samples when that bit is clear.
     * Keep the native mute, data-track and failed-sector checks intact. */
    static const unsigned char cdda_expected[28] = {
        0x00,0x00,0x02,0x92, 0x01,0x00,0x42,0x30,
        0xff,0x00,0x42,0x30, 0x09,0x00,0x40,0x10,
        0x01,0x00,0x02,0x24, 0x08,0x00,0x42,0x52,
        0x00,0xc2,0x70,0x26
    };
    static const unsigned char cdda_corrected[28] = {
        0x00,0x00,0x02,0x92, 0x01,0x00,0x42,0x30,
        0xff,0x00,0x42,0x30, 0x00,0x00,0x00,0x00,
        0x01,0x00,0x02,0x24, 0x08,0x00,0x42,0x52,
        0x00,0xc2,0x70,0x26
    };
    error = np_patch_add(set, 0x0020eac8, cdda_expected, cdda_corrected, sizeof(cdda_expected));
    if (error) return error;
    /* Enabling the SPU is not a power-on reset. POPS clears its entire SPU
     * state here, losing voice rates and other registers programmed while
     * disabled. Silent voices then never reach their end flags, blocking
     * games that allocate voices only after their envelopes become idle.
     * Keep the native cold-reset initializer, enable predicates, control
     * shadow and transfer/IRQ processing; omit only the enable-time reset. */
    static const unsigned char spu_expected[32] = {
        0x07,0x00,0x40,0x14, 0xaa,0x01,0x33,0x96,
        0x00,0x80,0x62,0x32, 0x05,0x00,0x40,0x10,
        0x50,0x03,0x04,0x26, 0x80,0x4f,0x08,0x0c,
        0x00,0x00,0x00,0x00, 0xaa,0x01,0x33,0xa6
    };
    static const unsigned char spu_corrected[32] = {
        0x07,0x00,0x40,0x14, 0xaa,0x01,0x33,0x96,
        0x00,0x80,0x62,0x32, 0x05,0x00,0x40,0x10,
        0x50,0x03,0x04,0x26, 0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00, 0xaa,0x01,0x33,0xa6
    };
    error = np_patch_add(set, 0x00213c0c, spu_expected, spu_corrected, sizeof(spu_expected));
    if (error) return error;
    /* GTE data register 23 (RES1) is ordinary 32-bit read/write storage.
     * POPS classifies its writes as discarded, so MTC2/LWC2 omit the store
     * while MFC2/SWC2 read stale data. Select the native word-store path;
     * retain the surrounding register classes and special FIFO handlers. */
    static const unsigned char gte_expected[32] = {
        2,0,0,0, 2,0,0,0, 0,0,0,0, 0,0,0,0,
        0,0,0,0, 3,0,0,0, 3,0,0,0, 0,0,0,0
    };
    static const unsigned char gte_corrected[32] = {
        2,0,0,0, 2,0,0,0, 0,0,0,0, 0,0,0,0,
        0,0,0,0, 0,0,0,0, 3,0,0,0, 0,0,0,0
    };
    error = np_patch_add(set, 0x0024a938, gte_expected, gte_corrected, sizeof(gte_expected));
    if (error) return error;
    /* GP1(10h) selects GPU information using the command's low bits. At this
     * point v1 contains only its opcode (10h), so masking v1 always selects
     * zero. Use the full command retained in s1; keep queue publication,
     * read barriers, the query latch and every native response unchanged. */
    static const unsigned char gpu_info_expected[32] = {
        0x10,0x00,0x02,0x24, 0x04,0x00,0x82,0x14,
        0x00,0x70,0x12,0x3c, 0x0f,0x00,0x63,0x30,
        0x30,0x00,0x02,0x3c, 0xdc,0xef,0x43,0xac,
        0xf0,0x10,0x43,0x26, 0x7c,0x22,0x62,0x8c
    };
    static const unsigned char gpu_info_corrected[32] = {
        0x10,0x00,0x02,0x24, 0x04,0x00,0x82,0x14,
        0x00,0x70,0x12,0x3c, 0x0f,0x00,0x23,0x32,
        0x30,0x00,0x02,0x3c, 0xdc,0xef,0x43,0xac,
        0xf0,0x10,0x43,0x26, 0x7c,0x22,0x62,0x8c
    };
    return np_patch_add(set, 0x0021a5b0, gpu_info_expected, gpu_info_corrected, sizeof(gpu_info_expected));
}
