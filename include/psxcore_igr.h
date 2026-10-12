/* Optional frontend-neutral IGR contract, input and cooperative pause policies.
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
 * REQUEST is emitted once and requests a runtime exit. The caller owns the
 * exit mechanism; this policy does not save game progress. */
uint32_t np_igr_input_update(NpIgrInput *state,uint16_t buttons,
    uint32_t sequence,uint32_t now_ms,int valid);

/* Caller-owned cooperative pause policy, serialized by the runtime adapter.
 * This is not an inter-thread mailbox: the adapter must synchronize requests,
 * cancellation and polling. No POPS hook or kernel wait is installed here.
 * A HOLD result applies only while that adapter owns the post-event boundary,
 * excludes all new guest mutations and yields without dispatching guest events.
 * It never authorizes save flush, teardown or ELF return. */
#define NP_IGR_PAUSE_RUN 0u
#define NP_IGR_PAUSE_HOLD 1u
#define NP_IGR_PAUSE_IGNORE 2u
#define NP_IGR_PAUSE_MAX_MS 60000u
typedef struct {
    uint32_t sequence,ticket,pending,held;
    uint32_t started_ms,timeout_ms;
} NpIgrPause;
void np_igr_pause_init(NpIgrPause *state);
/* Initialize once per session. Tickets belong only to that state/session.
 * One outstanding request. Returns a nonzero ticket, or zero on rejection.
 * Ticket exhaustion rejects requests rather than reusing an old identity.
 * Timeout covers both acquisition and holding, in modulo-2^32 milliseconds. */
uint32_t np_igr_pause_request(NpIgrPause *state,uint32_t now_ms,uint32_t timeout_ms);
/* Cancellation requires the current ticket; stale tickets cannot resume a
 * newer pause. If held, a new request remains rejected until the owner polls
 * this cancellation. Cancel does not itself acknowledge guest resume. */
int np_igr_pause_cancel(NpIgrPause *state,uint32_t ticket);
/* Call only from the owning guest thread, including each cooperative wait turn.
 * boundary_verified must mean committed CPU context, completed guest dispatch,
 * and exclusion of every guest mutation source. card_command_idle must come
 * from independently verified protocol tracking, never flags/counter alone.
 * Only exact 1 is accepted. Missing proof, timeout or cancellation returns RUN;
 * loss of proof while held retires the request. A stale/invalid ticket returns
 * IGNORE and leaves the current request intact. IGNORE must never resume a held
 * thread. RUN is an instruction to stop holding, not a resume acknowledgement. */
uint32_t np_igr_pause_poll(NpIgrPause *state,uint32_t ticket,uint32_t now_ms,
    int boundary_verified,int card_command_idle);
#endif
