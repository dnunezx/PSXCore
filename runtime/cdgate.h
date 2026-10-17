/* Original same-disc diagnostic admission owner. No media replacement API. */
#ifndef PSXCORE_CD_GATE_H
#define PSXCORE_CD_GATE_H
#include "cdpark.h"
enum {NP_CD_GATE_IDLE,NP_CD_GATE_REQUESTED,NP_CD_GATE_HELD};
enum {NP_CD_GATE_NONE,NP_CD_GATE_PARKED,NP_CD_GATE_COMMAND_RESUME,
    NP_CD_GATE_DEADLINE_RESUME,NP_CD_GATE_REQUEST_EXPIRED,NP_CD_GATE_FAULT};
typedef struct {
    NpCdPark park;
    uint32_t generation,phase,started,budget,result,fault,visits;
    uint32_t parameter_count,overflow;
    uint8_t parameters[5],reserved[3];
} NpCdGate;
_Static_assert(sizeof(NpCdGate)==148,"CD gate layout");
extern NpCdGate np_cd_gate;
extern char np_cd_gate_begin[],np_cd_gate_code_end[],np_cd_gate_end[];
void np_cd_gate_write_entry(void);
void np_cd_gate_tail(void);
/* All functions require the pinned guest revision, POPS gp and serialized
 * guest main-thread ownership. Only the checked scheduler return hook may
 * service acquisition: dispatch-zero by itself is insufficient. No hook is
 * armed in the production bootstrap. Arena/stack persistence is unproven.
 * Request from an original guest controller, never external memory writes.
 * Generations strictly increase; budget is 1..INT32_MAX guest scheduler ticks
 * with service gaps below 2^31. Returns 1=queued, 0=busy, -1=invalid. */
int np_cd_gate_request(uint32_t generation,uint32_t budget);
/* MMIO binding: inactive calls preserve the original writer. While held,
 * parameter writes use a bounded shadow FIFO and retain native write cost;
 * IRQ acknowledge/parameter flush and status-only commands still progress.
 * Every other command FIRST resumes the same disc, restores staged parameters,
 * then executes at its original MMIO boundary. No command is silently queued,
 * dropped or reported successful. On a resource mismatch it fails closed.
 * This abort-on-command policy is diagnostic only, NOT a replacement gate.
 * Prefix result: -1=blocked fault, 0=delegate original inputs, 1=shadow
 * parameter, 2=delegate with acknowledge bit 6 suppressed. The entry shim
 * preserves incoming registers for handled writes; delegated calls retain the
 * original native writer's return value and register behavior. */
int np_cd_gate_write(uint32_t address,uint32_t value);
/* Called only after due dispatch AND cancellation cleanup. Acquires parking,
 * checks held state and resumes on deadline. Never issues media IO, executes
 * a command, waits, or acknowledges sector/DMA/audio/save drainage. */
void np_cd_gate_service(void);
#endif
