/* Native swap scheduler state and transparent main-thread binding. */
#ifndef PSXCORE_CD_SWAP_RUNTIME_H
#define PSXCORE_CD_SWAP_RUNTIME_H
#include <stdint.h>
typedef struct {
    uint32_t magic,version,bytes,sequence;
    uint32_t phase,visits,started,opened,closed;
    int32_t result;
    uint32_t retries,thread_id,entry_sp,entry_status,stack_base,stack_end;
    uint32_t rejected,after_visits,initial_cd,final_cd,enabled,reserved[3];
} NpCdSwapRuntime;
_Static_assert(sizeof(NpCdSwapRuntime)==96,"native swap scheduler state");
extern NpCdSwapRuntime np_cd_swap_runtime;
extern char np_cd_swap_context[],np_cd_swap_context_end[];
extern char np_cd_swap_runtime_code_end[],np_cd_swap_runtime_end[];
void np_cd_swap_context_boot(void),np_cd_swap_runtime_tail(void);
void np_cd_swap_runtime_tick(uint32_t sp,uint32_t status);
#endif
