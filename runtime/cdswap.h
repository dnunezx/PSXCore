/* Native multi-disc selection for a validated frontend-supplied disc set. */
#ifndef PSXCORE_CD_SWAP_H
#define PSXCORE_CD_SWAP_H
#include <stdint.h>
#define NP_CD_SWAP_MAGIC UINT32_C(0x5357504e)
typedef struct {
    uint32_t magic,version,bytes,ready,count,current,selected,request,completed;
    uint32_t latch,idle,buttons,input_visits,seen,owns_lid;
    int32_t result;
    uint32_t sectors[4],reserved[12];
} NpCdSwap;
_Static_assert(sizeof(NpCdSwap)==128,"manual swap record");
extern NpCdSwap np_cd_swap;
extern unsigned char np_cd_swap_tocs[4][1024];
extern char np_cd_swap_begin[],np_cd_swap_end[];
extern void np_cd_swap_input(void);
/* Native active-low report byte order, slot-zero player 1 only. No RPC here. */
void np_cd_swap_buttons(unsigned char *first,unsigned char *second,unsigned port);
#endif
