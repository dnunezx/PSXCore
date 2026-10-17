/* Original bounded parking of native read/play scheduler work. */
#ifndef PSXCORE_CD_PARK_H
#define PSXCORE_CD_PARK_H
#include <stdint.h>
typedef struct {
    uint32_t event,callback,argument,remaining,due;
} NpCdParkEvent;
typedef struct {
    uint32_t generation,held,count,started,budget,expired;
    uint32_t handles[2],control[8];
    NpCdParkEvent events[2];
} NpCdPark;
_Static_assert(sizeof(NpCdPark)==104,"CD parked work layout");

/* Dormant native binding; no caller, patches, command gate or media IO.
 * Caller MUST establish checked revision/instruction windows, POPS gp,
 * exclusive scheduler/CD main-thread ownership after dispatch AND cancellation
 * cleanup, and exclude CD command/effect producers and parameter FIFO mutation
 * until resume, except checked status-only commands. A sampled
 * dispatch-zero flag or caller-supplied generation establishes none of these.
 * Storage starts zeroed and remains alive until successful resume. Generations
 * strictly increase; no reset/cancellation may discard held native resources.
 *
 * Hold transactionally detaches at most one native read and one play event,
 * keeping the actual nodes, opaque arguments and remaining native delay.
 * It refuses deferred read/play work: IRQ/response progress must retire that
 * work normally before retry. Foreign events and deferred replies keep running.
 * No cached pointer can be revoked by this API: only the identified future
 * callbacks are parked. Already-submitted worker/RPC/prefetch/DMA/audio/save IO
 * must continue, and its completion is NOT acknowledged here. No producer or
 * Init drain permit may be manufactured from these results.
 *
 * 1=success, 0=busy (no change), -1=invalid/mismatch (no change).
 * Budget is 1..INT32_MAX native scheduler ticks with service gaps below 2^31.
 * Failure before ownership changes leaves state/native memory untouched. */
int np_cd_park_hold(NpCdPark *state,uint32_t generation,uint32_t budget);
/* Checked held-node identity/absence plus command-state consistency. 1 means
 * ONLY that this scoped scheduler parking remains intact; -2 reports expiry.
 * Neither result certifies IO drainage. Expiry retains resources for resume. */
int np_cd_park_status(NpCdPark *state,uint32_t generation);
/* Same-disc rollback: all held nodes are validated before restoring any.
 * Reuses native insertion without allocation, resuming each callback exactly
 * once through normal scheduler delay adjustment. Works after expiry too.
 * Refuses changed command state/handles or conflicting read/play/deferred
 * producers. Such a conflict needs future explicit owner recovery; it cannot
 * be repaired by discarding work or blind reset. No live experiment is wired. */
int np_cd_park_resume(NpCdPark *state,uint32_t generation);
#endif
