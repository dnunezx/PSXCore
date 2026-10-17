/* Original default-off same-disc admission experiment; no live caller. */
#ifndef NP_CD_GATE
#define NP_CD_GATE 0
#endif
#if NP_CD_GATE!=0 && NP_CD_GATE!=1
#error "NP_CD_GATE must be 0 or 1"
#endif
#if NP_CD_GATE
#if !NP_CD_PARK || !NP_CD_RESPONSE
#error "NP_CD_GATE requires native event parking and response scheduler support"
#endif
#include "cdgate.h"
#define CODE __attribute__((section("._psxcore_cd_gate.text")))
#define WORD(a) (*(volatile uint32_t *)(uintptr_t)(a))
#define BYTE(a) (*(volatile uint8_t *)(uintptr_t)(a))
NpCdGate np_cd_gate __attribute__((section("._psxcore_cd_gate.data"),aligned(16)))={0};
static CODE uint32_t clock_now(void) {return ((uint32_t (*)(void))(uintptr_t)0x201008u)();}
static CODE void charge_write(void) {WORD(WORD(0x502710u)+708u)-=15u;}
static CODE int fault(void) {np_cd_gate.fault=1;np_cd_gate.result=NP_CD_GATE_FAULT;return -1;}
static CODE int valid(void) {
    return np_cd_gate.phase<=NP_CD_GATE_HELD && np_cd_gate.fault<=1 &&
        np_cd_gate.parameter_count<=4 && np_cd_gate.overflow<=1 &&
        (np_cd_gate.phase==NP_CD_GATE_HELD ? np_cd_gate.park.held==1 &&
            np_cd_gate.park.generation==np_cd_gate.generation : !np_cd_gate.park.held);
}
CODE int np_cd_gate_request(uint32_t generation,uint32_t budget) {
    if(!valid() || np_cd_gate.fault || !generation || generation<=np_cd_gate.generation ||
       !budget || budget>INT32_MAX)return -1;
    if(np_cd_gate.phase || WORD(0x502ea0u))return 0;
    np_cd_gate.generation=generation;np_cd_gate.phase=NP_CD_GATE_REQUESTED;
    np_cd_gate.started=clock_now();np_cd_gate.budget=budget;np_cd_gate.result=0;
    return 1;
}
/* Called before delegating a mutating command, at that command's own MMIO
 * boundary, or by post-cleanup service on deadline. No native handler is
 * invoked here: scheduler service must not start RPCs or wait on IO. */
static CODE int resume(uint32_t result) {
    if(np_cd_park_resume(&np_cd_gate.park,np_cd_gate.generation)!=1)return fault();
    for(unsigned i=0;i<4;++i)BYTE(0x59c21cu+i)=np_cd_gate.parameters[i];
    if(np_cd_gate.overflow)BYTE(0x59c220u)=np_cd_gate.parameters[4];
    WORD(0x59c218u)=np_cd_gate.parameter_count;
    np_cd_gate.phase=NP_CD_GATE_IDLE;np_cd_gate.result=result;
    return 1;
}
CODE int np_cd_gate_write(uint32_t address,uint32_t value) {
    if(!valid())return fault();
    uint32_t decoded=(address&15u)|((BYTE(0x502ee8u)&3u)<<4);
    if(np_cd_gate.phase==NP_CD_GATE_HELD) {
        if(WORD(0x59c218u))fault(); /* Never hide an unexpected native FIFO mutation. */
        if(np_cd_gate.fault && (decoded==1 || decoded==2)){charge_write();return -1;}
        if(decoded==2) {
            charge_write();
            np_cd_gate.parameters[np_cd_gate.parameter_count]=(uint8_t)value;
            if(np_cd_gate.parameter_count<4)++np_cd_gate.parameter_count;
            else np_cd_gate.overflow=1;
            return 1;
        }
        if(decoded==1) {
            if(value==1)np_cd_gate.parameter_count=0; /* Native Getstat ignores parameters. */
            else if(resume(NP_CD_GATE_COMMAND_RESUME)!=1){charge_write();return -1;}
        } else if(decoded==0x13 && (value&0x40u)) {
            np_cd_gate.parameter_count=0;
            return 2; /* Native ack runs, real FIFO count stays frozen. */
        }
    } else if(np_cd_gate.fault){charge_write();return -1;}
    return 0;
}
CODE void np_cd_gate_service(void) {
    if(!valid()){fault();return;}
    if(np_cd_gate.fault || WORD(0x502ea0u))return;
    if(np_cd_gate.visits!=UINT32_MAX)++np_cd_gate.visits;
    if(np_cd_gate.phase==NP_CD_GATE_HELD) {
        int status=np_cd_park_status(&np_cd_gate.park,np_cd_gate.generation);
        if(status==-2)resume(NP_CD_GATE_DEADLINE_RESUME);
        else if(status!=1)fault();
    } else if(np_cd_gate.phase==NP_CD_GATE_REQUESTED) {
        uint32_t elapsed=clock_now()-np_cd_gate.started;
        if(elapsed>=np_cd_gate.budget) {
            np_cd_gate.phase=NP_CD_GATE_IDLE;np_cd_gate.result=NP_CD_GATE_REQUEST_EXPIRED;
            return;
        }
        /* Do not seize a partially staged native command. Admission starts
         * only after the native FIFO is naturally empty. */
        if(WORD(0x59c218u))return;
        int result=np_cd_park_hold(&np_cd_gate.park,np_cd_gate.generation,np_cd_gate.budget-elapsed);
        if(result<0){fault();return;}
        if(!result)return;
        for(unsigned i=0;i<5;++i)np_cd_gate.parameters[i]=BYTE(0x59c21cu+i);
        np_cd_gate.parameter_count=0;np_cd_gate.overflow=0;
        np_cd_gate.phase=NP_CD_GATE_HELD;np_cd_gate.result=NP_CD_GATE_PARKED;
    }
}
#endif
