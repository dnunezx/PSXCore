/* Original manual guest producer. No external request/guest-memory writes. */
#if NP_CD_SWAP!=0 && NP_CD_SWAP!=1
#error "NP_CD_SWAP must be 0 or 1"
#endif
#if NP_CD_SWAP
#include "cdswap.h"
#include "cdreopen.h"
#include "cdswap_runtime.h"
#define CODE __attribute__((section("._psxcore_cd_swap.text")))
#define WORD(a) (*(volatile uint32_t *)(uintptr_t)(a))
#define HALF(a) (*(volatile uint16_t *)(uintptr_t)(a))
#define UNCACHED(p) ((void *)((uintptr_t)(p)|0x20000000u))
NpCdSwapRuntime np_cd_swap_runtime __attribute__((section("._psxcore_cd_swap_runtime.data"),aligned(16)))={.magic=0x52504344,.version=1,.bytes=96,.enabled=1};
NpCdSwap np_cd_swap __attribute__((section("._psxcore_cd_swap.data"),aligned(64)))={0};
unsigned char np_cd_swap_tocs[4][1024] __attribute__((section("._psxcore_cd_swap.tocs"),aligned(64)))={0};
static CODE int ready(void) {
    /* Existing guarded native worker clears dirty only after write/close/
     * rename/sync returns. Wait for that worker; never force a save or flush. */
    return WORD(0x507920u)==1 && WORD(0x527930u)==1 && !WORD(0x85aa80u) &&
        !WORD(0x85aa84u) && WORD(0x85aa94u)==UINT32_MAX && WORD(0x85cac4u)==UINT32_MAX;
}
CODE void np_cd_swap_buttons(unsigned char *first,unsigned char *second,unsigned port) {
    volatile NpCdSwap *c=UNCACHED(&np_cd_swap);
    volatile NpCdSwapRuntime *s=UNCACHED(&np_cd_swap_runtime);
    if(c->magic!=NP_CD_SWAP_MAGIC || c->version!=1 || c->bytes!=sizeof(*c) ||
       c->ready!=1 || c->count<2 || c->count>4 || !s->enabled || s->rejected || port ||
       second!=first+1 || first[-2] || (first[-1]!=0x41 && first[-1]!=0x73 && first[-1]!=0x79) ||
       WORD(0x70003738u))return;
    uint32_t pressed=(~((uint32_t)*first<<8 | *second))&65535u;
    c->buttons=pressed;
    if(c->input_visits!=UINT32_MAX)++c->input_visits;
    if(!pressed){c->idle=1;c->latch=0;return;}
    if(c->latch || c->request!=c->completed)goto consume;
    /* L2+R2+Select with exactly one direction selects the corresponding
     * registered route: Up=0, Right=1, Down=2, Left=3. Require neutral first. */
    if(!c->idle || (pressed&0x0103u)!=0x0103u)return;
    uint32_t dir=pressed&0xf000u,slot;
    if(dir==0x1000u)slot=0;else if(dir==0x2000u)slot=1;
    else if(dir==0x4000u)slot=2;else if(dir==0x8000u)slot=3;else return;
    c->latch=1;c->idle=0;
    if(slot>=c->count || c->request==UINT32_MAX){c->result=-20;goto consume;}
    c->selected=slot;c->result=0;__asm__ volatile("sync":::"memory");++c->request;
consume:
    /* Consume the whole report while a selector chord/request is held.
     * A neutral report must precede insertion and the next request. */
    c->idle=0;*first=255;*second=255;
}
static CODE uint32_t now(void){return ((uint32_t (*)(void))(uintptr_t)0x201008u)();}
static CODE int main_thread(volatile uint32_t *b){return ((int (*)(void))(uintptr_t)0x22e710u)()==(int)b[5];}
CODE void np_cd_swap_runtime_tick(uint32_t sp,uint32_t status) {
    volatile NpCdSwap *c=UNCACHED(&np_cd_swap);
    volatile NpCdSwapRuntime *s=UNCACHED(&np_cd_swap_runtime);
    volatile uint32_t *b=UNCACHED(np_cd_swap_context);
    ++s->sequence;__asm__ volatile("sync":::"memory");
    if(s->visits!=UINT32_MAX)++s->visits;
    s->entry_sp=sp;s->entry_status=status;
    uint32_t base=b[14],bytes=b[15],end=base+bytes;
    if(s->rejected || b[4]!=1 || (int32_t)b[5]<=0 || (int32_t)b[6]<0 ||
       b[13]!=0x200008u || b[16]!=0x50a670u || !base || !bytes || end<base ||
       end>0x2000000u || sp<base || sp>end || sp-base<8192u || (status&0x30006u)!=0x30000u){
        s->rejected=1;s->phase=4;s->result=-10;goto done;
    }
    s->stack_base=base;s->stack_end=end;
    if(c->magic!=NP_CD_SWAP_MAGIC || c->version!=1 || c->bytes!=sizeof(*c) || c->ready!=1 ||
       c->count<2 || c->count>4 || c->current>=c->count)goto done;
    if(!s->phase){if(!ready() || !main_thread(b))goto done;s->phase=1;s->thread_id=b[5];}
    if(!s->enabled || c->request==c->completed){
        if(s->phase==3 && s->after_visits!=UINT32_MAX)++s->after_visits;
        goto done;
    }
    uint32_t tick=now();
    if(c->seen!=c->request){c->seen=c->request;s->phase=1;s->started=tick;s->retries=0;}
    if(c->selected>=c->count){s->result=-20;goto failed;}
    if(s->phase==1){
        if((uint32_t)(tick-s->started)>=0x40000000u){s->result=-14;goto failed;}
        if(!ready() || WORD(0x502ea0u) || WORD(0x502898u)!=UINT32_MAX)goto done;
        if(!main_thread(b)){s->result=-13;goto failed;}
        s->initial_cd=HALF(0x59c200u);
        if(s->initial_cd&0x1000u){if(!c->owns_lid){s->result=-11;goto failed;}}
        else {HALF(0x59c200u)=0x1000u;c->owns_lid=1;}
        s->opened=tick;s->phase=2;goto done;
    }
    if((uint32_t)(tick-s->opened)>=0x20000000u || s->retries>=4096u){s->result=-12;goto failed;}
    if(!c->idle || !ready() || (uint32_t)(tick-s->opened)<0x2000000u)goto done;
    if(!main_thread(b)){s->result=-13;goto failed;}
    ++s->retries;
    int result=np_cd_reopen_target(c->selected,UNCACHED(np_cd_swap_tocs[c->selected]),c->sectors[c->selected]);
    s->result=result;
    if(result>0){
        c->current=c->selected;c->owns_lid=0;c->result=1;c->completed=c->request;
        s->closed=now();s->final_cd=HALF(0x59c200u);s->phase=3;
    }else if(result<0)goto failed;
    goto done;
failed:
    s->phase=4;s->final_cd=HALF(0x59c200u);c->result=s->result;c->completed=c->request;
done:
    __asm__ volatile("sync":::"memory");++s->sequence;
}
#endif
