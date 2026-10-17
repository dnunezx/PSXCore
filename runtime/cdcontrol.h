/* Original dormant command admission/effects binding. Not a media-change API. */
#ifndef PSXCORE_CD_CONTROL_H
#define PSXCORE_CD_CONTROL_H
#include "cdresponse.h"

enum {NP_CD_CONTROL_CLOSING=1u};
enum {
    NP_CD_CONTROL_PRODUCERS=1u, NP_CD_CONTROL_SECTOR_DMA=2u,
    NP_CD_CONTROL_AUDIO_TAIL=4u, NP_CD_CONTROL_VERIFY_SAVE=8u,
    NP_CD_CONTROL_INIT_DRAIN=15u
};
typedef struct {
    NpCdProtocolPlan reply;
    uint32_t command,parameters,value,native_status,admission,applied;
} NpCdControlPlan;
typedef struct {uint32_t generation,token,acknowledged,reserved;} NpCdControlPermit;
_Static_assert(sizeof(NpCdControlPlan)==88,"control proposal layout");

/* Prepare is pure and never consumes native parameters. Closing admission also
 * rejects media commands while logical phase is still READY, before ejection.
 * Status and already-specified unavailable replies progress; unresolved controls
 * defer for bounded owner service. The owner must retire obsolete Stop
 * completion before an intervening command; this layer does not own a session
 * queue or automatically cancel it. Counts for owned Getstat/Stop/Init/Setmode are checked before reading
 * value, so a missing Setmode parameter cannot reuse native FIFO bytes.
 * Disabled, and READY with admission open, delegate non-owned native commands.
 * These are decisions only; every producer still needs a guarded binding. */
int np_cd_control_prepare(const NpCdProtocol *state,uint32_t command,
    uint32_t parameters,uint32_t value,uint32_t native_status,
    uint32_t admission,NpCdControlPlan *plan);
/* Applies an owned Init/Stop/Setmode exactly once, BEFORE protocol acceptance.
 * Requires a checked revision/ABI, exclusive serialized scheduler/CD owner,
 * outside dispatch/cleanup, with all required response nodes RESERVED. No live
 * caller or permit producer is installed. A permit is an owner receipt from
 * actual guarded bindings, NEVER a sampled zero, user setting or elapsed delay.
 * Init requires sector/cache/DMA, downstream audio-transfer and verification/
 * save retirement too; it deliberately executes the destructive native reset.
 * Stop/Setmode only require exclusive producers and do not certify IO drainage.
 * All failures preserve state, proposal, owned responses and native memory.
 * Success marks applied; reply publication/acceptance remains the caller's job.
 * Native timing is 16934 ticks for ACK and 50802 for Stop/Init completion;
 * these are pinned POPS timings, not a physical-drive timing claim. */
int np_cd_control_apply(const NpCdProtocol *state,NpCdControlPlan *plan,
    const NpCdControlPermit *permit,NpCdResponse *responses);
/* Commit only after effects (when needed) and matching snapshot publication.
 * All planned replies must still be SCHEDULED, with the same owner tags and
 * bytes. Init also retains its separately RESERVED completion node. Calling
 * the protocol core directly would bypass this command-owner check. */
int np_cd_control_accept(NpCdProtocol *state,const NpCdControlPlan *plan,
    const NpCdResponse *responses);
#endif
