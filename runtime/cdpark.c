/* Original default-off scheduler ownership binding. Never an IO-drain API. */
#ifndef NP_CD_PARK
#define NP_CD_PARK 0
#endif
#if NP_CD_PARK!=0 && NP_CD_PARK!=1
#error "NP_CD_PARK must be 0 or 1"
#endif
#if NP_CD_PARK
#if !NP_CD_RESPONSE
#error "NP_CD_PARK requires the checked held-node scheduler shim"
#endif
#include "cdpark.h"
#define CODE __attribute__((section("._psxcore_cd_park.text")))
#define WORD(a) (*(volatile uint32_t *)(uintptr_t)(a))
#define FREE 0x502e94u
#define PENDING 0x502e98u
#define DISPATCH 0x502ea0u
#define SENTINEL 0x502724u
#define POOL 0x5032c8u
#define READ 0x20e7f8u
#define PLAY 0x20ea68u
extern void np_cd_response_schedule_reserved(uint32_t,void (*)(void *),void *,uint32_t);

static CODE unsigned kind(uint32_t callback) {return callback==READ?1u:callback==PLAY?2u:0u;}
static CODE int event_valid(uint32_t event) {
    return event>=POOL && event<POOL+64u*24u && (event-POOL)%24u==0;
}
static CODE uint32_t clock_now(void) {return ((uint32_t (*)(void))(uintptr_t)0x201008u)();}
static CODE uint32_t control(unsigned i) {
    if(!i)return WORD(0x59c200u)&0xffffbfffu; /* IRQ delivery may clear seek bit 6. */
    return WORD(0x59c200u+(i==7?36u:4u*i));
}
/* One bounded audit of both lists, including disjointness and duplicate/cycle
 * rejection. Held nodes can belong to other adapters; do not require all 64
 * nodes to be on these lists. Never follow an unvalidated node address. */
static CODE int inspect(const NpCdPark *s,uint32_t *read_event,uint32_t *play_event) {
    uint64_t seen=0;
    uint32_t node=WORD(FREE),previous=0;
    *read_event=0;*play_event=0;
    for(unsigned chain=0;chain<2;++chain) {
        if(chain){node=WORD(PENDING);previous=0;}
        for(unsigned count=0;node;++count) {
            if(count==64 || !event_valid(node))return -1;
            uint64_t bit=(uint64_t)1<<((node-POOL)/24u);
            if(seen&bit || WORD(node+4)!=(chain?previous:WORD(SENTINEL)))return -1;
            seen|=bit;
            if(chain) {
                unsigned k=kind(WORD(node+12));
                if(k) {
                    uint32_t *slot=k==1?read_event:play_event;
                    if(*slot || WORD(node+20))return -1;
                    *slot=node;
                }
            }
            previous=node;node=WORD(node);
        }
    }
    if(s && s->held)for(unsigned i=0;i<s->count;++i) {
        const NpCdParkEvent *e=&s->events[i];
        if(!event_valid(e->event) || !kind(e->callback) || !e->remaining || e->remaining>INT32_MAX ||
           seen&((uint64_t)1<<((e->event-POOL)/24u)) || WORD(e->event) || WORD(e->event+4) ||
           WORD(e->event+8)!=e->due || WORD(e->event+12)!=e->callback ||
           WORD(e->event+16)!=e->argument || WORD(e->event+20) ||
           (i && e->event==s->events[0].event))return -1;
    }
    return 1;
}
static CODE int deferred(void) {
    /* Do not leave a hole at the native deferred cursor or compact unrelated
     * replies. Deferred CD work must progress before acquiring this scope. */
    for(unsigned i=0;i<8;++i)if(kind(WORD(0x59d5d8u+8u*i)))return 1;
    return 0;
}
static CODE void detach(uint32_t node) {
    uint32_t next=WORD(node),previous=WORD(node+4);
    if(previous)WORD(previous)=next;else WORD(PENDING)=next;
    if(next)WORD(next+4)=previous;
    WORD(node)=0;WORD(node+4)=0;
}
static CODE int validate(const NpCdPark *s,uint32_t generation) {
    if(!s || s->held!=1 || s->count>2 || !generation || generation!=s->generation ||
       !s->budget || s->budget>INT32_MAX || s->expired>1)return -1;
    if(WORD(DISPATCH))return 0;
    uint32_t read_event,play_event;
    if(inspect(s,&read_event,&play_event)!=1 || read_event || play_event || deferred())return -1;
    for(unsigned i=0;i<2;++i)if(WORD(0x502f00u+4u*i)!=s->handles[i])return -1;
    unsigned mask=0;
    for(unsigned i=0;i<s->count;++i) {
        unsigned k=kind(s->events[i].callback),bit=1u<<(k-1);
        if(mask&bit || s->handles[k-1]!=s->events[i].event)return -1;
        mask|=bit;
    }
    if((s->handles[0]!=0)!=!!(mask&1u) || (s->handles[1]!=0)!=!!(mask&2u))return -1;
    for(unsigned i=0;i<8;++i)if(control(i)!=s->control[i])return -1;
    return 1;
}
CODE int np_cd_park_hold(NpCdPark *s,uint32_t generation,uint32_t budget) {
    if(!s || !generation || generation<=s->generation || !budget || budget>INT32_MAX || s->held>1)return -1;
    if(s->held || WORD(DISPATCH))return 0;
    uint32_t read_event,play_event;
    if(inspect(0,&read_event,&play_event)!=1)return -1;
    if(deferred())return 0;
    uint32_t handles[2]={WORD(0x502f00u),WORD(0x502f04u)};
    if(handles[0]!=read_event || handles[1]!=play_event)return 0;
    uint32_t now=clock_now();
    /* No allocation, native callback or fallible operation after validation.
     * Preserve pending-list order, including equal-time read/play events. */
    unsigned count=0;
    for(uint32_t node=WORD(PENDING);node;node=WORD(node))if(node==read_event || node==play_event) {
        NpCdParkEvent *e=&s->events[count++];
        e->event=node;e->callback=WORD(node+12);e->argument=WORD(node+16);e->due=WORD(node+8);
        uint32_t remaining=e->due-now;
        e->remaining=remaining && remaining<=INT32_MAX?remaining:1u;
    }
    s->generation=generation;s->count=count;s->started=now;s->budget=budget;s->expired=0;
    for(unsigned i=0;i<2;++i)s->handles[i]=handles[i];
    for(unsigned i=0;i<8;++i)s->control[i]=control(i);
    for(unsigned i=0;i<count;++i)detach(s->events[i].event);
    s->held=1;
    return 1;
}
CODE int np_cd_park_status(NpCdPark *s,uint32_t generation) {
    int result=validate(s,generation);
    if(result!=1)return result;
    if((uint32_t)(clock_now()-s->started)>=s->budget)s->expired=1;
    return s->expired?-2:1;
}
CODE int np_cd_park_resume(NpCdPark *s,uint32_t generation) {
    int result=validate(s,generation);
    if(result!=1)return result;
    /* The shim uses the pinned scheduler's own clock, delay adjustment and
     * insertion tail. Held nodes cannot be allocated by other producers. */
    for(unsigned i=0;i<s->count;++i) {
        const NpCdParkEvent *e=&s->events[i];
        np_cd_response_schedule_reserved(e->remaining,(void (*)(void *))(uintptr_t)e->callback,
            (void *)(uintptr_t)e->argument,e->event);
    }
    s->held=0;s->count=0;
    return 1;
}
#endif
