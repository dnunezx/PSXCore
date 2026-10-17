/* Dormant PS1 controller reply planning. No native IO, patches or swap API. */
#ifndef PSXCORE_CD_PROTOCOL_H
#define PSXCORE_CD_PROTOCOL_H
#include <stdint.h>
enum {
    NP_CD_PROTO_DISABLED,NP_CD_PROTO_READY,NP_CD_PROTO_OPEN,
    NP_CD_PROTO_SPINUP,NP_CD_PROTO_DETECT,NP_CD_PROTO_EMPTY
};
enum {NP_CD_PROTO_NATIVE,NP_CD_PROTO_REPLY,NP_CD_PROTO_DEFER,NP_CD_PROTO_UNRESOLVED,
    NP_CD_PROTO_INIT_ACK,NP_CD_PROTO_INIT_COMPLETE,NP_CD_PROTO_DROP,
    NP_CD_PROTO_STOP,NP_CD_PROTO_SETMODE};
enum {NP_CD_PROTO_ACK_DELAY,NP_CD_PROTO_ASYNC_DELAY};
typedef struct {
    uint32_t generation,token,phase,change_latch,open_event_pending;
    uint32_t init_token,init_completion_queued,reserved;
} NpCdProtocol;
typedef struct {
    uint32_t irq,length,delay;
    uint8_t payload[8];
} NpCdProtocolReply;
typedef struct {
    uint32_t generation,token,action,count,clear_latch,clear_open_event;
    NpCdProtocolReply replies[2];
} NpCdProtocolPlan;
_Static_assert(sizeof(NpCdProtocol)==32,"CD protocol state layout");
_Static_assert(sizeof(NpCdProtocolPlan)==64,"CD protocol plan layout");
/* All calls need one serialized guest owner and owned caller storage.
 * A phase change is a logical input, never a drain or native media transition.
 * Future integration must establish the exclusive lease and retirement of old
 * replies before using it. No runtime caller or phase-change control is wired.
 * Return 1 on success, 0 when no change/commit is made, -1 for invalid input or
 * exhausted generations/tokens. Negative results preserve supplied state. */
int np_cd_protocol_init(NpCdProtocol *state);
int np_cd_protocol_change(NpCdProtocol *state,uint32_t phase);
int np_cd_protocol_command(const NpCdProtocol *state,uint32_t command,
    uint8_t native_status,NpCdProtocolPlan *plan);
/* Init ACK is an effect-bearing proposal: before accepting it, the native
 * owner must reserve BOTH reply/event resources and start verified controller
 * Init effects. No reset, cancellation, mode or motor mutation happens here.
 * Init does not establish TOC readiness. Duplicate Init has DROP action until
 * the owner's delivered-completion or verified-cancellation receipt retires.
 * Capture the completion status at actual native completion, not at ACK time.
 * Phase transitions are deferred while this operation owns its generation. */
int np_cd_protocol_init_complete(const NpCdProtocol *state,uint32_t init_token,
    uint8_t native_status,NpCdProtocolPlan *plan);
/* completed=1 requires queued completion AND actual native delivery;
 * completed=0 requires verified native cancellation/obsolete-event retirement.
 * This only retires controller Init ownership, never PS1 consumption or IO. */
int np_cd_protocol_init_retire(NpCdProtocol *state,uint32_t init_token,
    uint32_t completed);
/* The lid event's status error bit differs between reference descriptions.
 * An owner must explicitly select 1 or 4 after target validation; no implicit
 * hardware profile is chosen. Opening reason is 08h in either case. */
int np_cd_protocol_open_event(const NpCdProtocol *state,uint32_t error_bit,
    NpCdProtocolPlan *plan);
/* Emitters must reserve ALL response/event resources before issuing any reply.
 * Accept only after every planned response is queued successfully. Failure,
 * stale generations, partial queueing and replay preserve the latch/event.
 * This acknowledges queue publication only, never PS1 consumption or IO drain.
 * An unresolved/deferred/native plan has no adapter commit. */
int np_cd_protocol_accept(NpCdProtocol *state,const NpCdProtocolPlan *plan,
    uint32_t queued_count);
#endif
