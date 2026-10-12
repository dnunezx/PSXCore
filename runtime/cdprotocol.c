/* Original bounded protocol core, enabled only for isolated integration work.
 * Plans are based on documented PS1 controller behavior. It performs no native
 * calls, delays, response publication, lifecycle reset or storage operation. */
#ifndef NP_CD_PROTOCOL
#define NP_CD_PROTOCOL 0
#endif
#if NP_CD_PROTOCOL != 0 && NP_CD_PROTOCOL != 1
#error "NP_CD_PROTOCOL must be 0 or 1"
#endif
#if NP_CD_PROTOCOL
#include "cdprotocol.h"
#define CD_CODE __attribute__((section("._psxcore_cd_protocol.text")))
static CD_CODE int valid(const NpCdProtocol *s) {
    return s && s->generation && s->token && s->phase<=NP_CD_PROTO_EMPTY &&
        s->change_latch<=1 && s->open_event_pending<=1 && !s->reserved &&
        s->init_completion_queued<=1 &&
        (!s->init_token || (s->init_token<s->token && s->phase!=NP_CD_PROTO_DISABLED && !s->open_event_pending)) &&
        (!s->init_completion_queued || s->init_token) &&
        (!s->open_event_pending || (s->phase==NP_CD_PROTO_OPEN && s->change_latch)) &&
        (s->phase!=NP_CD_PROTO_DISABLED || (!s->change_latch && !s->open_event_pending)) &&
        (s->phase!=NP_CD_PROTO_OPEN || s->change_latch);
}
static CD_CODE void clear_plan(const NpCdProtocol *s,NpCdProtocolPlan *p) {
    p->generation=s->generation;p->token=s->token;
    p->action=NP_CD_PROTO_NATIVE;p->count=0;p->clear_latch=0;p->clear_open_event=0;
    for(unsigned i=0;i<2;++i) {
        p->replies[i].irq=0;p->replies[i].length=0;p->replies[i].delay=0;
        for(unsigned j=0;j<8;++j)p->replies[i].payload[j]=0;
    }
}
static CD_CODE uint8_t status(const NpCdProtocol *s,uint8_t native) {
    unsigned latch=s->change_latch?0x10u:0;
    if(s->phase==NP_CD_PROTO_READY)return (native&0xefu)|latch;
    if(s->phase==NP_CD_PROTO_DETECT)return 2u|latch;
    return latch;
}
static CD_CODE void reply(NpCdProtocolPlan *p,unsigned i,unsigned irq,
    unsigned length,unsigned delay,uint8_t stat) {
    p->action=NP_CD_PROTO_REPLY;p->count=i+1;
    p->replies[i].irq=irq;p->replies[i].length=length;p->replies[i].delay=delay;
    p->replies[i].payload[0]=stat;
}
CD_CODE int np_cd_protocol_init(NpCdProtocol *s) {
    if(!s)return -1;
    s->generation=1;s->token=1;s->phase=NP_CD_PROTO_DISABLED;
    s->change_latch=0;s->open_event_pending=0;s->init_token=0;
    s->init_completion_queued=0;s->reserved=0;
    return 1;
}
CD_CODE int np_cd_protocol_change(NpCdProtocol *s,uint32_t phase) {
    if(!valid(s) || phase>NP_CD_PROTO_EMPTY || s->generation==UINT32_MAX ||
       s->token==UINT32_MAX)return -1;
    if(s->phase==phase)return 0;
    if(s->open_event_pending || s->init_token)return -1;
    unsigned allowed=0;
    switch(s->phase) {
    case NP_CD_PROTO_DISABLED:allowed=phase==NP_CD_PROTO_READY;break;
    case NP_CD_PROTO_READY:allowed=phase==NP_CD_PROTO_OPEN;break;
    case NP_CD_PROTO_OPEN:allowed=phase==NP_CD_PROTO_SPINUP || phase==NP_CD_PROTO_EMPTY;break;
    case NP_CD_PROTO_SPINUP:allowed=phase==NP_CD_PROTO_DETECT || phase==NP_CD_PROTO_EMPTY || phase==NP_CD_PROTO_OPEN;break;
    case NP_CD_PROTO_DETECT:allowed=phase==NP_CD_PROTO_READY || phase==NP_CD_PROTO_EMPTY || phase==NP_CD_PROTO_OPEN;break;
    case NP_CD_PROTO_EMPTY:allowed=phase==NP_CD_PROTO_SPINUP || phase==NP_CD_PROTO_OPEN;break;
    }
    if(!allowed)return -1;
    ++s->generation;s->phase=phase;
    if(phase==NP_CD_PROTO_OPEN){s->change_latch=1;s->open_event_pending=1;}
    return 1;
}
CD_CODE int np_cd_protocol_command(const NpCdProtocol *s,uint32_t command,
    uint8_t native,NpCdProtocolPlan *p) {
    if(!valid(s) || !p || s->token==UINT32_MAX)return -1;
    clear_plan(s,p);
    if(s->phase==NP_CD_PROTO_DISABLED)return 1;
    if(s->open_event_pending){p->action=NP_CD_PROTO_DEFER;return 1;}
    uint8_t stat=status(s,native);
    if(command==0x0a) {
        if(s->init_token){p->action=NP_CD_PROTO_DROP;return 1;}
        reply(p,0,3,1,NP_CD_PROTO_ACK_DELAY,stat);
        p->action=NP_CD_PROTO_INIT_ACK;
        return 1;
    }
    if(command==1) {
        reply(p,0,3,1,NP_CD_PROTO_ACK_DELAY,stat);
        p->clear_latch=s->phase!=NP_CD_PROTO_OPEN && s->change_latch;
        return 1;
    }
    if(s->phase==NP_CD_PROTO_READY)return 1;
    /* Explicit unavailable-media policy. The remaining controls/parameter
     * validation need their own native binding; never silently pass them on. */
    switch(command) {
    case 0x1a:
        if(s->phase==NP_CD_PROTO_EMPTY) {
            reply(p,0,3,1,NP_CD_PROTO_ACK_DELAY,stat);
            reply(p,1,5,8,NP_CD_PROTO_ASYNC_DELAY,stat|8u);
            p->replies[1].payload[1]=0x40;
            return 1;
        }
        /* Fall through to the immediate unavailable error. */
        __attribute__((fallthrough));
    case 2:case 3:case 4:case 5:case 6:case 7:case 0x10:case 0x11:
    case 0x12:case 0x13:case 0x14:case 0x15:case 0x16:case 0x1b:case 0x1e:
        reply(p,0,5,2,NP_CD_PROTO_ACK_DELAY,stat|1u);
        p->replies[0].payload[1]=0x80;
        return 1;
    default:p->action=NP_CD_PROTO_UNRESOLVED;return 1;
    }
}
CD_CODE int np_cd_protocol_init_complete(const NpCdProtocol *s,uint32_t token,
    uint8_t native,NpCdProtocolPlan *p) {
    if(!valid(s) || !p || !token || token!=s->init_token || s->token==UINT32_MAX)return -1;
    clear_plan(s,p);
    if(s->init_completion_queued){p->action=NP_CD_PROTO_DEFER;return 0;}
    reply(p,0,2,1,NP_CD_PROTO_ASYNC_DELAY,status(s,native));
    p->action=NP_CD_PROTO_INIT_COMPLETE;
    return 1;
}
CD_CODE int np_cd_protocol_init_retire(NpCdProtocol *s,uint32_t token,uint32_t completed) {
    if(!valid(s) || !token || token!=s->init_token || completed>1 ||
       (completed && !s->init_completion_queued) || s->token==UINT32_MAX)return -1;
    s->init_token=0;s->init_completion_queued=0;++s->token;
    return 1;
}
CD_CODE int np_cd_protocol_open_event(const NpCdProtocol *s,uint32_t error_bit,
    NpCdProtocolPlan *p) {
    if(!valid(s) || !p || s->token==UINT32_MAX || (error_bit!=1 && error_bit!=4))return -1;
    clear_plan(s,p);
    if(!s->open_event_pending){p->action=NP_CD_PROTO_DEFER;return 0;}
    reply(p,0,5,2,NP_CD_PROTO_ASYNC_DELAY,0x10u|error_bit);
    p->replies[0].payload[1]=8;p->clear_open_event=1;
    return 1;
}
CD_CODE int np_cd_protocol_accept(NpCdProtocol *s,const NpCdProtocolPlan *p,
    uint32_t queued_count) {
    if(!valid(s) || !p || p->generation!=s->generation || p->token!=s->token ||
       s->token==UINT32_MAX)return -1;
    if((p->action!=NP_CD_PROTO_REPLY && p->action!=NP_CD_PROTO_INIT_ACK &&
        p->action!=NP_CD_PROTO_INIT_COMPLETE) || p->count<1 || p->count>2 || queued_count!=p->count)return 0;
    for(unsigned i=0;i<p->count;++i)if(!p->replies[i].length || p->replies[i].length>8 ||
        (p->replies[i].irq!=3 && p->replies[i].irq!=5 &&
         !(p->action==NP_CD_PROTO_INIT_COMPLETE && p->replies[i].irq==2)) ||
        p->replies[i].delay>NP_CD_PROTO_ASYNC_DELAY)return -1;
    if(p->clear_latch>1 || p->clear_open_event>1 ||
       (p->clear_latch && p->clear_open_event) ||
       (p->clear_latch && (s->phase==NP_CD_PROTO_OPEN || !s->change_latch)) ||
       (p->clear_open_event && !s->open_event_pending))return -1;
    if(p->clear_latch && (p->count!=1 || p->replies[0].irq!=3 || p->replies[0].length!=1))return -1;
    if(p->clear_open_event && (p->count!=1 || p->replies[0].irq!=5 || p->replies[0].length!=2 ||
        p->replies[0].payload[1]!=8))return -1;
    if(p->action==NP_CD_PROTO_INIT_ACK || p->action==NP_CD_PROTO_INIT_COMPLETE) {
        unsigned complete=p->action==NP_CD_PROTO_INIT_COMPLETE;
        if(p->count!=1 || p->clear_latch || p->clear_open_event || s->open_event_pending ||
           s->phase==NP_CD_PROTO_DISABLED || p->replies[0].length!=1 ||
           p->replies[0].irq!=(complete?2u:3u) ||
           p->replies[0].delay!=(complete?NP_CD_PROTO_ASYNC_DELAY:NP_CD_PROTO_ACK_DELAY) ||
           (complete?(!s->init_token || s->init_completion_queued):s->init_token))return -1;
        if(complete)s->init_completion_queued=1;
        else s->init_token=s->token;
    }
    if(p->clear_latch)s->change_latch=0;
    if(p->clear_open_event)s->open_event_pending=0;
    ++s->token;
    return 1;
}
#endif
