/* Original native resource adapter. No production caller, media IO or reset. */
#ifndef NP_CD_RESPONSE
#define NP_CD_RESPONSE 0
#endif
#if NP_CD_RESPONSE != 0 && NP_CD_RESPONSE != 1
#error "NP_CD_RESPONSE must be 0 or 1"
#endif
#if NP_CD_RESPONSE
#if !NP_CD_PROTOCOL
#error "NP_CD_RESPONSE requires NP_CD_PROTOCOL"
#endif
#if NP_IGR_RETURN
#error "Dormant CD arenas overlap the IGR private stack; composition is unsupported"
#endif
#include "cdresponse.h"
#include <stddef.h>
#define CD_CODE __attribute__((section("._psxcore_cd_response.text")))
#define CD_POOL __attribute__((section("._psxcore_cd_response_pool.text")))
#define CD_RECEIPT __attribute__((section("._psxcore_cd_response_receipt.text")))
#define WORD(a) (*(volatile uint32_t *)(uintptr_t)(a))
#define BYTE(a) (*(volatile uint8_t *)(uintptr_t)(a))
#define FREE_HEAD 0x502e94u
#define PENDING_HEAD 0x502e98u
#define DISPATCHING 0x502ea0u
#define FREE_SENTINEL 0x502724u
#define ACTIVE 0x502ef8u
#define IRQ_LATCHED 0x502ef0u
#define REGISTERS 0x502ee8u
#define POOL_BEGIN 0x5032c8u
#define POOL_END (POOL_BEGIN+64u*24u)
_Static_assert(offsetof(NpCdResponse,resource)==40,"native response prefix");
_Static_assert(offsetof(NpCdResponse,payload)==24,"native response bytes");
extern void np_cd_response_schedule_reserved(uint32_t ticks,
    void (*callback)(NpCdResponse *),NpCdResponse *response,uint32_t event);

static CD_POOL int event_valid(uint32_t event) {
    return event>=POOL_BEGIN && event<POOL_END && (event-POOL_BEGIN)%24u==0;
}
/* Validate the whole bounded free chain before reserving anything. The native
 * free sentinel is a loaded value, not the address of its GP-relative slot. */
static CD_POOL int free_nodes(uint32_t *first,uint32_t *second,uint32_t excluded) {
    uint32_t node=WORD(FREE_HEAD),seen[2]={0,0};
    unsigned count=0;
    while(node) {
        if(count==64 || node==excluded || !event_valid(node) || WORD(node+4)!=WORD(FREE_SENTINEL))return -1;
        if(count<2)seen[count]=node;
        ++count;node=WORD(node);
    }
    *first=seen[0];*second=seen[1];
    return (int)count;
}
static CD_POOL void release(uint32_t event) {
    WORD(event)=WORD(FREE_HEAD);WORD(event+4)=WORD(FREE_SENTINEL);
    WORD(FREE_HEAD)=event;
}
#if NP_CD_CONTROL
__attribute__((section("._psxcore_cd_control.text")))
int np_cd_response_reserved(const NpCdResponse *r,uint32_t count,
    uint32_t generation,uint32_t token) {
    if(!r || count<1 || count>2 || !generation || !token)return -1;
    if(WORD(DISPATCHING))return 0;
    for(unsigned i=0;i<count;++i) {
        uint32_t a,b,event=r[i].resource;
        if(r[i].state!=NP_CD_RESPONSE_RESERVED || r[i].generation!=generation ||
           r[i].token!=token || r[i].event!=event || !event_valid(event) ||
           WORD(event) || WORD(event+4)!=WORD(FREE_SENTINEL) ||
           free_nodes(&a,&b,event)<0 || (i && event==r[0].resource))return -1;
    }
    return 1;
}
#endif
CD_POOL int np_cd_response_reset(NpCdResponse *r) {
    if(!r || r->resource || r->event || (r->state!=NP_CD_RESPONSE_IDLE && r->state!=NP_CD_RESPONSE_CONSUMED &&
        r->state!=NP_CD_RESPONSE_CANCELLED))return -1;
    unsigned char *bytes=(unsigned char *)r;
    for(unsigned i=0;i<sizeof(*r);++i)bytes[i]=0;
    return 1;
}
CD_CODE int np_cd_response_reserve(NpCdResponse *r,uint32_t count,
    uint32_t generation,uint32_t token) {
    if(!r || count<1 || count>2 || !generation || !token)return -1;
    if(WORD(DISPATCHING))return 0;
    for(unsigned i=0;i<count;++i)if(r[i].state!=NP_CD_RESPONSE_IDLE || r[i].resource || r[i].event)return -1;
    uint32_t nodes[2];int available=free_nodes(&nodes[0],&nodes[1],0);
    if(available<0)return -1;
    if((unsigned)available<count)return 0;
    WORD(FREE_HEAD)=WORD(nodes[count-1]);
    for(unsigned i=0;i<count;++i) {
        np_cd_response_reset(&r[i]);
        r[i].event=nodes[i];r[i].resource=nodes[i];r[i].state=NP_CD_RESPONSE_RESERVED;
        r[i].generation=generation;r[i].token=token;
        WORD(nodes[i])=0;
    }
    return 1;
}
/* The native scheduler owns its node through the whole callback batch. This
 * callback only publishes readiness; it never cancels/recycles that node. */
static CD_POOL void due(NpCdResponse *r) {
    if(r->state==NP_CD_RESPONSE_SCHEDULED) {
        r->event=0;r->resource=0;r->state=NP_CD_RESPONSE_DUE;
    }
}
CD_CODE int np_cd_response_queue(NpCdResponse *r,const NpCdProtocolReply *p,
    uint32_t ticks) {
    if(!r || !p || r->state!=NP_CD_RESPONSE_RESERVED || !event_valid(r->resource) ||
       !r->generation || !r->token || !ticks || ticks>INT32_MAX ||
       (p->irq!=2 && p->irq!=3 && p->irq!=5) || p->length<1 || p->length>8 ||
       p->delay>NP_CD_PROTO_ASYNC_DELAY)return -1;
    if(WORD(DISPATCHING))return 0;
    uint32_t a,b;
    if(r->event!=r->resource || WORD(r->resource) ||
       WORD(r->resource+4)!=WORD(FREE_SENTINEL) || free_nodes(&a,&b,r->resource)<0)return -1;
    r->irq=p->irq;r->length=p->length;r->cursor=0;
    for(unsigned i=0;i<16;++i)r->payload[i]=i<8?p->payload[i]:0;
    r->state=NP_CD_RESPONSE_SCHEDULED;
    np_cd_response_schedule_reserved(ticks,due,r,r->resource);
    return 1;
}
CD_CODE int np_cd_response_deliver(NpCdResponse *r) {
    if(!r || r->state!=NP_CD_RESPONSE_DUE || r->resource ||
       r->length<1 || r->length>8 || (r->irq!=2 && r->irq!=3 && r->irq!=5))return -1;
    if(WORD(DISPATCHING) || WORD(ACTIVE) || WORD(IRQ_LATCHED) || (BYTE(REGISTERS+2)&7))return 0;
    /* Unlike the native generic callback, never replace payload[0]. Unlike
     * its pool allocator, do not reuse a response whose bytes remain unread. */
    BYTE(REGISTERS)=(BYTE(REGISTERS)|0x20u)&0x7fu;
    BYTE(REGISTERS+2)=(uint8_t)r->irq;
    WORD(ACTIVE)=(uint32_t)(uintptr_t)r;
    r->state=NP_CD_RESPONSE_DELIVERED;
    ((void (*)(void))(uintptr_t)0x20e638u)();
    return 1;
}
CD_RECEIPT int np_cd_response_consume(NpCdResponse *r) {
    if(!r || r->state!=NP_CD_RESPONSE_DELIVERED || r->length<1 || r->length>8 ||
       r->cursor>r->length)return -1;
    if(WORD(DISPATCHING) || r->cursor!=r->length || WORD(ACTIVE)==(uint32_t)(uintptr_t)r)return 0;
    r->state=NP_CD_RESPONSE_CONSUMED;
    return 1;
}
CD_RECEIPT int np_cd_response_cancel(NpCdResponse *r) {
    if(!r || (r->state!=NP_CD_RESPONSE_RESERVED && r->state!=NP_CD_RESPONSE_SCHEDULED &&
       r->state!=NP_CD_RESPONSE_DUE))return -1;
    if(WORD(DISPATCHING))return 0;
    if(r->state==NP_CD_RESPONSE_DUE && (r->resource || r->event))return -1;
    if(r->state!=NP_CD_RESPONSE_DUE) {
        if(!event_valid(r->resource))return -1;
        uint32_t a,b;
        if(free_nodes(&a,&b,r->resource)<0)return -1;
        if(r->state==NP_CD_RESPONSE_SCHEDULED) {
            uint32_t node=WORD(PENDING_HEAD),previous=0;unsigned found=0;
            for(unsigned i=0;node && i<64;++i) {
                if(!event_valid(node) || WORD(node+4)!=previous)return -1;
                if(node==r->resource)++found;
                previous=node;node=WORD(node);
            }
            if(node || found!=1 || WORD(r->resource+12)!=(uint32_t)(uintptr_t)due ||
               WORD(r->resource+16)!=(uint32_t)(uintptr_t)r)return -1;
            ((void (*)(uint32_t))(uintptr_t)0x2011d8u)(r->resource);
        } else {
            if(WORD(r->resource) || WORD(r->resource+4)!=WORD(FREE_SENTINEL))return -1;
            release(r->resource);
        }
    }
    r->event=0;r->resource=0;r->state=NP_CD_RESPONSE_CANCELLED;
    return 1;
}
#endif
