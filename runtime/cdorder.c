/* Experimental guest-only admission: wait for prior CD IRQ acknowledgement.
 * Existing native handlers, event delays and MMIO cycle charges remain owned
 * by POPS. One guest owner; bounded snapshots protect queued parameters. */
#include <stdint.h>
#define CODE __attribute__((section("._psxcore_cd_order.text"),noinline))
#define DATA __attribute__((section("._psxcore_cd_order.data")))
#define WORD(a) (*(volatile uint32_t *)(uintptr_t)(a))
#define BYTE(a) (*(volatile uint8_t *)(uintptr_t)(a))
typedef struct { uint32_t command,count,params[2]; } Pending;
static struct {
    uint32_t magic,revision,head,count,draining,enqueued,executed,overflow;
    Pending pending[8];
} state DATA={0x43444f52,1,0,0,0,0,0,0,{{0,0,{0,0}}}};
extern void np_cd_order_execute(uint32_t command);
static CODE int blocked(void) {
    return WORD(0x502ef0)!=0 || (BYTE(0x502eea)&31)!=0;
}
CODE int np_cd_order_submit(uint32_t command) {
    if(!state.count && !state.draining && !blocked())return 0;
    /* Stop on bounded-capacity exhaustion rather than execute out of order. */
    if(state.count==8 || WORD(0x59c218)>4) {
        ++state.overflow;
        for(;;)__asm__ volatile("nop");
    }
    Pending *p=&state.pending[(state.head+state.count)&7];
    p->command=command;p->count=WORD(0x59c218);
    p->params[0]=WORD(0x59c21c);p->params[1]=WORD(0x59c220);
    WORD(0x59c218)=0;
    ++state.count;++state.enqueued;
    return 1;
}
CODE void np_cd_order_drain(void) {
    if(!state.count || state.draining || blocked())return;
    const uint32_t saved_count=WORD(0x59c218);
    const uint32_t saved0=WORD(0x59c21c),saved1=WORD(0x59c220);
    const Pending p=state.pending[state.head];
    state.head=(state.head+1)&7;--state.count;state.draining=1;
    WORD(0x59c218)=p.count;WORD(0x59c21c)=p.params[0];WORD(0x59c220)=p.params[1];
    np_cd_order_execute(p.command);
    WORD(0x59c218)=saved_count;WORD(0x59c21c)=saved0;WORD(0x59c220)=saved1;
    ++state.executed;state.draining=0;
}
