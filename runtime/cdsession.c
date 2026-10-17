/* Original default-off session owner; no live hook, gate receipt or swap. */
#ifndef NP_CD_SESSION
#define NP_CD_SESSION 0
#endif
#if NP_CD_SESSION!=0 && NP_CD_SESSION!=1
#error "NP_CD_SESSION must be 0 or 1"
#endif
#if NP_CD_SESSION
#if !NP_CD_CONTROL || !NP_CD_RESPONSE || !NP_CD_PROTOCOL
#error "NP_CD_SESSION requires CD control, response and protocol"
#endif
#include "cdsession.h"
#define CODE __attribute__((section("._psxcore_cd_session.text")))
#define DISPATCHING (*(volatile uint32_t *)(uintptr_t)0x502ea0u)

static CODE int valid(const NpCdSession *s) {
    return s && s->protocol.generation && s->protocol.token &&
        s->protocol.phase<=NP_CD_PROTO_EMPTY && s->count<=2 &&
        s->expired<=1 && s->fault<=1;
}
static CODE int terminal(const NpCdResponse *r) {
    return r->state==NP_CD_RESPONSE_CONSUMED || r->state==NP_CD_RESPONSE_CANCELLED;
}
static CODE int elapsed(NpCdSession *s,uint32_t now) {
    if(s->count && (uint32_t)(now-s->deadline)<0x80000000u)s->expired=1;
    return s->expired;
}
static CODE int finish(NpCdSession *s) {
    for(unsigned i=0;i<3;++i)if(np_cd_response_reset(&s->responses[i])!=1) {
        s->fault=1;return -1;
    }
    s->count=0;s->action=0;s->generation=0;s->token=0;
    s->completion_token=0;s->poll_token=0;s->deadline=0;s->expired=0;
    return 1;
}
CODE int np_cd_session_init(NpCdSession *s) {
    if(!s)return -1;
    const unsigned char *p=(const unsigned char *)s;
    for(unsigned i=0;i<sizeof(*s);++i)if(p[i])return -1;
    return np_cd_protocol_init(&s->protocol);
}
CODE int np_cd_session_change(NpCdSession *s,uint32_t phase) {
    if(!valid(s) || s->fault)return -1;
    if(DISPATCHING || s->count)return 0;
    return np_cd_protocol_change(&s->protocol,phase);
}
/* Release only reservations, before any effect/publication has occurred. */
static CODE int release(NpCdResponse *r,unsigned count) {
    for(unsigned i=0;i<count;++i)
        if(np_cd_response_cancel(r+i)!=1 || np_cd_response_reset(r+i)!=1)return -1;
    return 1;
}
static CODE int publish(NpCdSession *s,NpCdControlPlan *p,NpCdResponse *r,
    unsigned count,const NpCdControlPermit *permit) {
    int result=np_cd_response_reserve(r,count,p->reply.generation,p->reply.token);
    if(result!=1)return result;
    if(p->reply.action==NP_CD_PROTO_STOP || p->reply.action==NP_CD_PROTO_SETMODE ||
       p->reply.action==NP_CD_PROTO_INIT_ACK) {
        result=np_cd_control_apply(&s->protocol,p,permit,r);
        if(result!=1) {
            if(release(r,count)!=1)s->fault=1;
            return result;
        }
    }
    /* Under the required lease these prevalidated held nodes cannot disappear.
     * Retain every resource if an invariant fails after effects/publication. */
    for(unsigned i=0;i<p->reply.count;++i)
        if(np_cd_response_queue(r+i,&p->reply.replies[i],
            p->reply.replies[i].delay==NP_CD_PROTO_ACK_DELAY?16934u:50802u)!=1) {
            s->fault=1;return -1;
        }
    if(np_cd_control_accept(&s->protocol,p,r)!=1){s->fault=1;return -1;}
    return 1;
}
static CODE int permit_valid(const NpCdSession *s,const NpCdControlPlan *p,
    const NpCdControlPermit *permit) {
    unsigned action=p->reply.action;
    if(action!=NP_CD_PROTO_STOP && action!=NP_CD_PROTO_SETMODE && action!=NP_CD_PROTO_INIT_ACK)return 1;
    unsigned bits=action==NP_CD_PROTO_INIT_ACK?NP_CD_CONTROL_INIT_DRAIN:NP_CD_CONTROL_PRODUCERS;
    return permit && !permit->reserved && permit->acknowledged<=NP_CD_CONTROL_INIT_DRAIN &&
        (permit->acknowledged&bits)==bits && permit->generation==s->protocol.generation &&
        permit->token==s->protocol.token;
}
CODE int np_cd_session_command(NpCdSession *s,const NpCdSessionRequest *q,
    const NpCdControlPermit *permit) {
    if(!valid(s) || s->fault || !q || !q->budget || q->budget>INT32_MAX)return -1;
    if(DISPATCHING)return 0;
    NpCdControlPlan p;
    if(np_cd_control_prepare(&s->protocol,q->command,q->parameters,q->value,
        q->native_status,q->admission,&p)!=1)return -1;
    if(p.reply.action==NP_CD_PROTO_DROP)return NP_CD_SESSION_DROP;
    if(p.reply.action==NP_CD_PROTO_DEFER)return 0;
    /* Never consume the last tokens needed for Init completion/retirement. */
    if(p.reply.action==NP_CD_PROTO_INIT_ACK && s->protocol.token>UINT32_MAX-3u)return -1;
    if(!permit_valid(s,&p,permit))return 0;
    unsigned poll=0;
    if(s->count) {
        elapsed(s,q->now);
        if(s->action==NP_CD_PROTO_INIT_ACK && q->command==1) {
            if(s->protocol.token>=UINT32_MAX-(s->completion_token?1u:2u))return -1;
            if(s->responses[0].state!=NP_CD_RESPONSE_CONSUMED ||
               s->responses[2].state!=NP_CD_RESPONSE_IDLE)return 0;
            poll=1;
        } else {
            /* A successor may obsolete Stop's second response, but never its
             * unread ACK or a delivered completion. No Init cancellation is
             * inferred from a new command or a timeout. */
            if(s->expired || s->action!=NP_CD_PROTO_STOP ||
               s->responses[0].state!=NP_CD_RESPONSE_CONSUMED ||
               s->responses[2].state!=NP_CD_RESPONSE_IDLE)return 0;
            if(np_cd_response_cancel(&s->responses[1])!=1)return 0;
            if(finish(s)!=1)return -1;
        }
    }
    if(p.reply.action==NP_CD_PROTO_NATIVE)return NP_CD_SESSION_NATIVE;
    unsigned count=p.reply.action==NP_CD_PROTO_INIT_ACK?2u:p.reply.count;
    NpCdResponse *r=&s->responses[poll?2:0];
    int result=publish(s,&p,r,count,permit);
    if(result!=1)return result;
    if(poll)s->poll_token=p.reply.token;
    else {
        s->count=count;s->action=p.reply.action;s->generation=p.reply.generation;
        s->token=p.reply.token;s->deadline=q->now+q->budget;
    }
    return NP_CD_SESSION_ACCEPTED;
}
CODE int np_cd_session_open_event(NpCdSession *s,uint32_t error_bit,
    uint32_t now,uint32_t budget) {
    if(!valid(s) || s->fault || !budget || budget>INT32_MAX)return -1;
    if(DISPATCHING || s->count)return 0;
    NpCdControlPlan p;
    int result=np_cd_protocol_open_event(&s->protocol,error_bit,&p.reply);
    if(result!=1)return result;
    p.applied=0;
    result=publish(s,&p,s->responses,1,0);
    if(result==1) {
        s->count=1;s->action=p.reply.action;s->generation=p.reply.generation;
        s->token=p.reply.token;s->deadline=now+budget;
    }
    return result;
}
CODE int np_cd_session_init_complete(NpCdSession *s,const NpCdSessionCompletion *c) {
    if(!valid(s) || s->fault || !c || c->native_status>255 || !c->ticks || c->ticks>INT32_MAX ||
       s->action!=NP_CD_PROTO_INIT_ACK || c->generation!=s->generation ||
       c->init_token!=s->token || c->init_token!=s->protocol.init_token || s->completion_token)return -1;
    if(DISPATCHING)return 0;
    NpCdProtocolPlan p;
    if(np_cd_protocol_init_complete(&s->protocol,c->init_token,(uint8_t)c->native_status,&p)!=1)return -1;
    NpCdResponse *r=&s->responses[1];
    int result=np_cd_response_reserved(r,1,s->generation,s->token);
    if(result!=1)return result;
    r->token=p.token;
    result=np_cd_response_queue(r,&p.replies[0],c->ticks);
    if(result!=1){r->token=s->token;return result;}
    s->completion_token=p.token;
    if(np_cd_protocol_accept(&s->protocol,&p,1)!=1){s->fault=1;return -1;}
    return 1;
}
CODE int np_cd_session_service(NpCdSession *s,uint32_t now) {
    if(!valid(s) || s->fault)return -1;
    if(DISPATCHING)return 0;
    if(!s->count)return 1;
    elapsed(s,now);
    for(unsigned i=0;i<3;++i) {
        NpCdResponse *r=&s->responses[i];
        if(i>=s->count && i!=2)continue;
        if(i==2 && !s->poll_token)continue;
        uint32_t token=i==2?s->poll_token:i==1 && s->completion_token?s->completion_token:s->token;
        if(r->generation!=s->generation || r->token!=token || r->state<NP_CD_RESPONSE_RESERVED ||
           r->state>NP_CD_RESPONSE_CANCELLED){s->fault=1;return -1;}
        if(r->state==NP_CD_RESPONSE_DELIVERED && np_cd_response_consume(r)<0){s->fault=1;return -1;}
    }
    if(s->poll_token && terminal(&s->responses[2])) {
        if(np_cd_response_reset(&s->responses[2])!=1){s->fault=1;return -1;}
        s->poll_token=0;
    }
    /* Plan order for command replies; status may progress while the separately
     * held Init completion waits for its actual native-effects receipt. */
    for(unsigned i=0;i<3;++i) {
        NpCdResponse *r=&s->responses[i];
        if(i && !terminal(&s->responses[0]))continue;
        if(r->state==NP_CD_RESPONSE_DUE && np_cd_response_deliver(r)<0){s->fault=1;return -1;}
    }
    unsigned done=1;
    for(unsigned i=0;i<s->count;++i)if(!terminal(&s->responses[i]))done=0;
    if(done && !s->poll_token) {
        if(s->action==NP_CD_PROTO_INIT_ACK && (s->responses[1].state!=NP_CD_RESPONSE_CONSUMED ||
           np_cd_protocol_init_retire(&s->protocol,s->token,1)!=1)){s->fault=1;return -1;}
        unsigned expired=s->expired;
        if(finish(s)!=1)return -1;
        return expired?NP_CD_SESSION_EXPIRED:1;
    }
    return s->expired?NP_CD_SESSION_EXPIRED:0;
}
#endif
