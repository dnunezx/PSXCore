/* Optional frontend-neutral IGR contract and normalized input policy.
 * These helpers do not install POPS hooks, flush saves or enable runtime return. */
#ifndef PSXCORE_IGR_H
#define PSXCORE_IGR_H
#include <stddef.h>
#include <stdint.h>

#define NP_IGR_MAGIC UINT32_C(0x5247504e)
#define NP_IGR_VERSION 1u
#define NP_IGR_PATH_BYTES 256u
#define NP_IGR_HOLD_MS 1000u
#define NP_IGR_REPORT_GAP_MS 100u
/* Active-high normalized PS controller bits: Select, Start, L1/L2/R1/R2.
 * A runtime adapter must independently verify and decode the raw report. */
#define NP_IGR_COMBO UINT16_C(0x0f09)
#define NP_IGR_INPUT_RESERVED 1u
#define NP_IGR_INPUT_REQUEST 2u

typedef struct {
    uint32_t magic;
    uint16_t version,size;
    uint32_t enabled,reserved;
    char return_path[NP_IGR_PATH_BYTES];
} NpIgrConfig;
_Static_assert(sizeof(NpIgrConfig)==272,"IGR config layout");

/* Integers serialize little-endian. Disabled configs have an all-zero path;
 * enabled configs have a canonical zero-padded mc0:/ or mc1:/ file path.
 * Validation proves syntax only, not file existence, loadability or support. */
void np_igr_config_init(NpIgrConfig *config);
const char *np_igr_config_check(const void *bytes,size_t size);
int np_igr_path_valid(const char *path,size_t capacity);

/* Caller-owned policy state. One controller, one session, serialized updates.
 * A fresh report with all shortcut buttons released must precede arming. */
typedef struct {
    uint32_t enabled,armed,holding,requested;
    uint32_t have_report,sequence,report_ms,hold_ms;
} NpIgrInput;
const char *np_igr_input_init(NpIgrInput *state,const NpIgrConfig *config);
/* now_ms is monotonic milliseconds modulo 2^32. sequence changes only when a
 * completed new report arrives; repeated polls must retain its sequence.
 * Call during missing reports too (valid=0 for invalid/disconnected input).
 * Gaps >100ms disarm. Duplicate reports never advance the hold or rearm.
 * RESERVED asks the adapter to hide the full shortcut from guest input,
 * including a stale valid sample that cannot advance the hold;
 * REQUEST is emitted once, and only requests a save-aware coordinator exit.
 * Neither flag grants permission to reset or bypass save completion. */
uint32_t np_igr_input_update(NpIgrInput *state,uint16_t buttons,
    uint32_t sequence,uint32_t now_ms,int valid);
#endif
