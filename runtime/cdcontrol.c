/* Original default-off guest command adapter. No production hooks or media IO. */
#ifndef NP_CD_CONTROL
#define NP_CD_CONTROL 0
#endif
#if NP_CD_CONTROL!=0 && NP_CD_CONTROL!=1
#error "NP_CD_CONTROL must be 0 or 1"
#endif
#if NP_CD_CONTROL
#if !NP_CD_PROTOCOL || !NP_CD_RESPONSE
#error "NP_CD_CONTROL requires NP_CD_PROTOCOL and NP_CD_RESPONSE"
#endif
#include "cdcontrol.h"
#define CODE __attribute__((section("._psxcore_cd_control.text")))
#define WORD(a) (*(volatile uint32_t *)(uintptr_t)(a))
#define BYTE(a) (*(volatile uint8_t *)(uintptr_t)(a))

static CODE void copy_bytes(void *dst,const void *src,unsigned count) {
    unsigned char *to=dst;const unsigned char *from=src;
    for(unsigned i=0;i<count;++i)to[i]=from[i];
}

static CODE void single(NpCdProtocolPlan *p,uint32_t action,uint32_t irq,uint32_t length,uint8_t status) {
    p->action=action;p->count=1;p->clear_latch=0;p->clear_open_event=0;
    p->replies[0].irq=irq;p->replies[0].length=length;
    p->replies[0].delay=NP_CD_PROTO_ACK_DELAY;
    p->replies[0].payload[0]=status;
}
CODE int np_cd_control_prepare(const NpCdProtocol *s,uint32_t command,
    uint32_t parameters,uint32_t value,uint32_t native,uint32_t admission,NpCdControlPlan *p) {
    if(!p || command>255 || parameters>16 || value>255 || native>255 || admission>1)return -1;
    /* Public protocol planning validates state without touching it. */
    NpCdProtocolPlan base;
    if(np_cd_protocol_command(s,command,(uint8_t)native,&base)!=1)return -1;
    copy_bytes(&p->reply,&base,sizeof(base));p->command=command;p->parameters=parameters;
    p->value=value;p->native_status=native;p->admission=admission;p->applied=0;
    if(s->phase==NP_CD_PROTO_DISABLED || base.action==NP_CD_PROTO_DEFER || base.action==NP_CD_PROTO_DROP)return 1;
    unsigned owned=command==1 || command==8 || command==10 || command==14;
    if(owned) {
        if(s->init_token && command!=1) {p->reply.action=NP_CD_PROTO_DEFER;p->reply.count=0;return 1;}
        unsigned status_command=command==1;
        /* Reuse the protocol's phase/latch status policy. Init's native reset
         * establishes status 02h before its first, late ACK is emitted. */
        np_cd_protocol_command(s,1,(uint8_t)(command==10?2:native),&p->reply);
        uint8_t status=p->reply.replies[0].payload[0];
        if(parameters!=(command==14?1u:0u)) {
            single(&p->reply,NP_CD_PROTO_REPLY,5,2,status|1u);
            p->reply.replies[0].payload[1]=0x20;return 1;
        }
        if(status_command)return 1;
        single(&p->reply,command==8?NP_CD_PROTO_STOP:command==10?NP_CD_PROTO_INIT_ACK:NP_CD_PROTO_SETMODE,
            3,1,command==8?status&0xdfu:status);
        if(command==8) {
            p->reply.count=2;copy_bytes(&p->reply.replies[1],&p->reply.replies[0],sizeof(p->reply.replies[0]));
            p->reply.replies[1].irq=2;p->reply.replies[1].delay=NP_CD_PROTO_ASYNC_DELAY;
            p->reply.replies[1].payload[0]=status&0x1du;
        }
        return 1;
    }
    /* A READY phase is not permission to admit media while closure drains old
     * work. Do not move the lid/readiness phase just to enforce this gate. */
    if(admission && base.action==NP_CD_PROTO_NATIVE) {
        if(command<32 && ((0x4c7f00fcu>>command)&1u)) {
            np_cd_protocol_command(s,1,(uint8_t)native,&p->reply);
            single(&p->reply,NP_CD_PROTO_REPLY,5,2,p->reply.replies[0].payload[0]|1u);
            p->reply.replies[0].payload[1]=0x80;
        } else {p->reply.action=NP_CD_PROTO_DEFER;p->reply.count=0;}
    } else if(base.action==NP_CD_PROTO_UNRESOLVED) {
        p->reply.action=NP_CD_PROTO_DEFER;
    }
    return 1;
}
CODE int np_cd_control_apply(const NpCdProtocol *s,NpCdControlPlan *p,
    const NpCdControlPermit *permit,NpCdResponse *r) {
    if(!s || !p || !permit || !r || p->applied || permit->reserved ||
       permit->acknowledged>NP_CD_CONTROL_INIT_DRAIN ||
       permit->generation!=s->generation || permit->token!=s->token ||
       p->reply.generation!=s->generation || p->reply.token!=s->token ||
       (p->command!=8 && p->command!=10 && p->command!=14))return -1;
    if(p->reply.action!=(p->command==8?NP_CD_PROTO_STOP:p->command==10?NP_CD_PROTO_INIT_ACK:NP_CD_PROTO_SETMODE))return -1;
    NpCdControlPlan expected;
    if(np_cd_control_prepare(s,p->command,p->parameters,p->value,p->native_status,p->admission,&expected)!=1)return -1;
    const unsigned char *a=(const unsigned char *)p,*b=(const unsigned char *)&expected;
    for(unsigned i=0;i<sizeof(expected);++i)if(a[i]!=b[i])return -1;
    unsigned required=p->command==10?NP_CD_CONTROL_INIT_DRAIN:NP_CD_CONTROL_PRODUCERS;
    if((permit->acknowledged&required)!=required || WORD(0x502ea0))return 0;
    unsigned count=p->command==14?1:2;
    if(np_cd_response_reserved(r,count,s->generation,s->token)!=1)return -1;
    /* No allocation, wait or response publication can fail after this point.
     * Init's response objects are outside the native pool it resets. Its held
     * scheduler nodes must stay excluded from every native producer. */
    if(p->command==14) {
        BYTE(0x59c200)=(uint8_t)p->value;
        WORD(0x502ef4)=(p->value&0x20)?12:24;
    } else {
        ((void (*)(void))(uintptr_t)0x20e4a8u)();
        if(p->command==10)((void (*)(void))(uintptr_t)0x20fbd0u)();
        else {
            BYTE(0x59c201)&=0xfdu;WORD(0x59c214)=1;
            WORD(0x59c204)=0;WORD(0x59c208)=0;
        }
    }
    p->applied=1;
    return 1;
}
CODE int np_cd_control_accept(NpCdProtocol *s,const NpCdControlPlan *p,const NpCdResponse *r) {
    if(!s || !p || !r || p->reply.generation!=s->generation || p->reply.token!=s->token)return -1;
    uint32_t action=p->reply.action;
    if(action==NP_CD_PROTO_NATIVE || action==NP_CD_PROTO_DEFER || action==NP_CD_PROTO_DROP)return 0;
    unsigned effect=action==NP_CD_PROTO_STOP || action==NP_CD_PROTO_SETMODE || action==NP_CD_PROTO_INIT_ACK;
    if(p->applied!=(effect?1u:0u) || p->reply.count<1 || p->reply.count>2 || WORD(0x502ea0))return -1;
    for(unsigned i=0;i<p->reply.count;++i) {
        const NpCdProtocolReply *reply=&p->reply.replies[i];
        if(r[i].state!=NP_CD_RESPONSE_SCHEDULED || !r[i].resource || r[i].event!=r[i].resource ||
           r[i].generation!=s->generation || r[i].token!=s->token ||
           r[i].irq!=reply->irq || r[i].length!=reply->length || r[i].cursor)return -1;
        for(unsigned j=0;j<8;++j)if(r[i].payload[j]!=reply->payload[j])return -1;
    }
    if(action==NP_CD_PROTO_INIT_ACK && np_cd_response_reserved(r+1,1,s->generation,s->token)!=1)return -1;
    return np_cd_protocol_accept(s,&p->reply,p->reply.count);
}
#endif
