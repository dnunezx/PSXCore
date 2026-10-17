/* Original dormant, bounded command owner. No admission hooks or media IO. */
#ifndef PSXCORE_CD_SESSION_H
#define PSXCORE_CD_SESSION_H
#include "cdcontrol.h"

enum { NP_CD_SESSION_BUSY=0, NP_CD_SESSION_ACCEPTED=1,
    NP_CD_SESSION_NATIVE=2, NP_CD_SESSION_DROP=3, NP_CD_SESSION_EXPIRED=-2 };
typedef struct {
    NpCdProtocol protocol;
    NpCdResponse responses[3]; /* command pair, plus Init status polling */
    uint32_t count,action,generation,token,completion_token,poll_token;
    uint32_t deadline,expired,fault;
} NpCdSession;
typedef struct {
    uint32_t command,parameters,value,native_status,admission,now,budget;
} NpCdSessionRequest;
typedef struct {
    uint32_t generation,init_token,native_status,ticks;
} NpCdSessionCompletion;
_Static_assert(sizeof(NpCdSession)==236,"CD session storage layout");

/* All calls inherit the checked revision, POPS gp, exclusive producer lease,
 * serialized main-thread and post-cleanup requirements of cdresponse.h.
 * These APIs DO NOT establish that lease. Storage is caller-owned and must
 * survive callbacks and unread native response bytes. No live caller exists.
 * Init accepts only entirely zeroed, fresh storage, never an active owner. */
int np_cd_session_init(NpCdSession *session);
/* Logical phase input only, requiring all owned replies to have retired.
 * Does not open a lid, certify readiness/drain, or replace media. */
int np_cd_session_change(NpCdSession *session,uint32_t phase);
/* One command transaction plus one status reply during Init. No unbounded
 * queue: BUSY means the caller retains the command and must retry/service.
 * NATIVE is delegation under the caller's continuing lease, not native-work
 * retirement. DROP implements only the already-owned Init duplicate policy.
 * A valid effect permit is required BEFORE retiring an obsolete Stop reply.
 * Tick budgets are 1..INT32_MAX, modulo 32-bit guest time, with service gaps
 * strictly below 2^31 ticks. They never certify
 * effects, IO completion or cancellation. Requests do not extend an Init's
 * deadline. No response storage is reused while native bytes remain unread. */
int np_cd_session_command(NpCdSession *session,const NpCdSessionRequest *request,
    const NpCdControlPermit *permit);
int np_cd_session_open_event(NpCdSession *session,uint32_t error_bit,
    uint32_t now,uint32_t budget);
/* Input from a future guarded native-effects completion producer, NOT a
 * timer/polling inference. No such producer is installed. Matching Init tags
 * and held scheduler-node identity are checked before retagging/publication.
 * Capture status at completion; completion enqueue does not retire Init. */
int np_cd_session_init_complete(NpCdSession *session,
    const NpCdSessionCompletion *completion);
/* Bounded post-cleanup service. Actual scheduler callbacks, native byte reads
 * and IRQ acknowledgements must continue independently. 1=idle/retired,
 * 0=pending, -2=deadline exceeded, -1=fault. Expiry keeps ownership and continues
 * receipt/delivery service; it never cancels unread replies, synthesizes Init
 * completion, restores media admission, or permits a phase change. Faults after
 * effects fail closed and retain storage for diagnosis; no unsafe reset API. */
int np_cd_session_service(NpCdSession *session,uint32_t now);
#endif
