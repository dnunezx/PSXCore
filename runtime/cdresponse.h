/* Original, default-off ownership adapter for the pinned POPS CD controller. */
#ifndef PSXCORE_CD_RESPONSE_H
#define PSXCORE_CD_RESPONSE_H
#include "cdprotocol.h"

enum {
    NP_CD_RESPONSE_IDLE, NP_CD_RESPONSE_RESERVED, NP_CD_RESPONSE_SCHEDULED,
    NP_CD_RESPONSE_DUE, NP_CD_RESPONSE_DELIVERED, NP_CD_RESPONSE_CONSUMED,
    NP_CD_RESPONSE_CANCELLED
};
/* First 40 bytes have the native response-reader layout. These objects are
 * caller-owned; NEVER place them in POPS's eight-entry response allocation pool.
 * The native reader can clear event/cursor without releasing our ownership. */
typedef struct {
    uint32_t event, unused[2], irq, length, cursor;
    uint8_t payload[16];
    uint32_t resource, state, generation, token;
} NpCdResponse;
_Static_assert(sizeof(NpCdResponse)==56,"CD response ownership layout");

/* Dormant binding, not an armed hook. All native-facing calls require the
 * checked POPS revision, expected instruction windows, POPS gp, serialized
 * guest main-thread ownership OUTSIDE scheduler dispatch/cleanup, and exclusion
 * of competing native CD response producers. Other threads/interrupts must not
 * mutate the native scheduler during a call. No callback may wait or do IO.
 * The caller must keep these objects alive until a terminal receipt, service
 * delivery after scheduler cleanup, and preserve native IRQ/byte consumption.
 * A sampled idle value does not establish the required publication lease.
 * Return 1=success, 0=busy/not complete, -1=invalid; failed calls do not mutate.
 * Zero-initialize new storage. Reset accepts only idle or terminal objects. */
int np_cd_response_reset(NpCdResponse *response);
/* Atomically reserve 1..2 native scheduler nodes for idle consecutive objects.
 * Reserve BOTH Init resources before accepting its ACK/starting native effects;
 * the second node may remain RESERVED until actual native Init completion.
 * This adapter deliberately does not call native Init/reset or protocol accept. */
int np_cd_response_reserve(NpCdResponse *responses,uint32_t count,
    uint32_t generation,uint32_t token);
/* Read-only check for NP_CD_CONTROL, with the same exclusive owner contract.
 * Verifies held native node identity before effects, without queuing a reply. */
int np_cd_response_reserved(const NpCdResponse *responses,uint32_t count,
    uint32_t generation,uint32_t token);
/* Copies the entire reply snapshot before publication. ticks is the verified
 * native scheduler delay, 1..INT32_MAX; reply.delay remains a planner policy.
 * Uses an already-reserved node: scheduling never allocates another resource.
 * Caller accepts the protocol plan only after ALL its replies are scheduled
 * and any effect-bearing command has started its verified native effects. */
int np_cd_response_queue(NpCdResponse *response,const NpCdProtocolReply *reply,
    uint32_t ticks);
/* Service in plan order after native scheduler cleanup. Delivery waits for
 * BOTH an empty active response and cleared CD IRQ state, preserving unread
 * bytes even if the PS1 acknowledges IRQ before reading the response. */
int np_cd_response_deliver(NpCdResponse *response);
/* Only an exhausted native byte cursor AND released active pointer yield a
 * CONSUMED receipt. Queuing or IRQ publication is not this receipt. Init owner
 * may retire completed=1 only after its completion has this matching receipt. */
int np_cd_response_consume(NpCdResponse *response);
/* Reserved/due work can be retired; scheduled work requires verified native
 * pending-list identity and synchronous cancellation outside dispatch. Never
 * revoke a delivered response or unread bytes. CANCELLED is not consumption.
 * The Init owner still needs an independent native-effects cancellation
 * receipt before retiring completed=0, even after all its replies cancel. */
int np_cd_response_cancel(NpCdResponse *response);
#endif
