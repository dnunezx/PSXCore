/* Backing SDK/filesystem work must never run on native IOPCD's 2 KiB stack.
 * One serialized slot, borrowed until synchronous completion, no heap copies.
 * The worker acquires the existing bridge lock only after the caller sleeps. */
#ifdef NP_HOST_TEST
#include "worker_stub.h"
#else
#include "irx_imports.h"
#endif
#include "worker.h"
#include <errno.h>
#include <stdint.h>
#include <string.h>

#define NP_FILE_WORKER_STACK 0x6000u
#define NP_FILE_WORKER_GUARD 0x400u
static int worker_thread=-1,worker_gate=-1,worker_pending=-1,worker_done=-1;
static NpFileWork *volatile work;
static volatile int worker_ready;
static volatile uint32_t *stack_guard;
static unsigned operations_seen;
static int guard_ok(void) {
    if(!stack_guard)return 0;
    for(unsigned i=0;i<NP_FILE_WORKER_GUARD/sizeof(*stack_guard);++i)
        if(stack_guard[i]!=0xa5a5a5a5)return 0;
    return 1;
}
static int stack_init(void) {
    iop_thread_info_t status;
    if(ReferThreadStatus(GetThreadId(),&status)<0 ||
       status.stackSize!=(int)NP_FILE_WORKER_STACK || !status.stack)return -EIO;
    uintptr_t base=(uintptr_t)status.stack;
    if((base&3) || base>UINTPTR_MAX-NP_FILE_WORKER_STACK)return -EIO;
#ifdef _IOP
    /* Only our allocated stack, and safely below this active frame. */
    uintptr_t local=(uintptr_t)&status;
    if(base+NP_FILE_WORKER_STACK>0x200000 || local<base+NP_FILE_WORKER_GUARD ||
       local+sizeof(status)>base+NP_FILE_WORKER_STACK)return -EIO;
#endif
    stack_guard=status.stack;
    for(unsigned i=0;i<NP_FILE_WORKER_GUARD/sizeof(*stack_guard);++i)
        stack_guard[i]=0xa5a5a5a5;
    return 0;
}
/* A released semaphore wait must not let the caller return while the worker
 * still owns its stack request/buffers. These private semaphores remain live
 * throughout registered-device lifetime; retry interrupted waits. */
static void wait_owned(int sema) {
    while(WaitSema(sema)<0)DelayThread(1000);
}
static void worker_main(void *arg) {
    (void)arg;
    worker_ready=stack_init()==0;
    SignalSema(worker_done);
    for(;;) {
        wait_owned(worker_pending);
        NpFileWork *request=work;
        if(!request)continue;
        if(!guard_ok())worker_ready=0;
        request->result=!worker_ready?-EIO:(request->operation&NP_FILE_SDK)?
            np_file_sdk_execute(request):np_file_native_execute(request);
        if(!guard_ok()) {
            worker_ready=0;
            request->result=-EIO;
            printf("PSXCORE file-worker=stack-budget-exhausted operation=%d\n",request->operation);
        }
        unsigned operation=(unsigned)request->operation&~NP_FILE_SDK;
        if(operation<=NP_FILE_DEVCTL && !(operations_seen&(1u<<operation))) {
            operations_seen|=1u<<operation;
            uint64_t result=(uint64_t)request->result;
            printf("PSXCORE file-worker=first-completion operation=%d result=%08x:%08x guard=%s\n",
                request->operation,(unsigned)(result>>32),(unsigned)result,worker_ready?"intact":"FAILED");
        }
        /* Result and descriptor changes are published before completion.
         * Do not touch borrowed request memory after signaling its caller. */
        work=NULL;
        SignalSema(worker_done);
    }
}
int64_t np_file_worker_call(NpFileWork *request) {
    if(!request || !request->file)return -EINVAL;
    if(worker_thread<0 || worker_gate<0)return -EIO;
    if(GetThreadId()==worker_thread) {
        /* Native ABI conversion calls the SDK-facing bridge on this same
         * worker. Execute that one hop directly, before its bridge lock is
         * acquired. Backing paths are mass:, so they cannot recurse to pfs. */
        if((request->operation&NP_FILE_SDK) && worker_ready)return np_file_sdk_execute(request);
        return -EDEADLK;
    }
    if(WaitSema(worker_gate)<0)return -EIO;
    if(!worker_ready){SignalSema(worker_gate);return -EIO;}
    request->result=-EIO;
    work=request;
    if(SignalSema(worker_pending)<0) {
        work=NULL;
        SignalSema(worker_gate);
        return -EIO;
    }
    wait_owned(worker_done);
    int64_t result=request->result;
    SignalSema(worker_gate);
    return result;
}
int np_file_worker_stop(void) {
    /* Called solely during failed startup, before any native registration. */
    if(worker_thread>=0) {
        iop_thread_info_t status;
        if(ReferThreadStatus(worker_thread,&status)<0)return -EIO;
        if(status.status!=THS_DORMANT && TerminateThread(worker_thread)<0)return -EIO;
        if(DeleteThread(worker_thread)<0)return -EIO;
        worker_thread=-1;
    }
    worker_ready=0;stack_guard=NULL;work=NULL;operations_seen=0;
    int *semas[]={&worker_gate,&worker_pending,&worker_done};
    for(unsigned i=0;i<3;++i)if(*semas[i]>=0) {
        if(DeleteSema(*semas[i])<0)return -EIO;
        *semas[i]=-1;
    }
    return 0;
}
int np_file_worker_start(void) {
    if(worker_thread>=0 || worker_gate>=0 || worker_pending>=0 || worker_done>=0)return -EIO;
    iop_sema_t sema={0};sema.max=1;sema.initial=1;
    worker_gate=CreateSema(&sema);
    if(worker_gate<0)return -EIO;
    sema.initial=0;
    worker_pending=CreateSema(&sema);
    if(worker_pending<0)return -EIO;
    worker_done=CreateSema(&sema);
    if(worker_done<0)return -EIO;
    iop_thread_t thread={0};thread.attr=TH_C;thread.thread=worker_main;
    thread.stacksize=NP_FILE_WORKER_STACK;thread.priority=40;
    worker_thread=CreateThread(&thread);
    if(worker_thread<0 || StartThread(worker_thread,NULL)<0)return -EIO;
    wait_owned(worker_done);
    if(!worker_ready)return -EIO;
    printf("PSXCORE file-worker=ready thread=%d stack=%u guard=%u native-caller=queued\n",
        worker_thread,NP_FILE_WORKER_STACK,NP_FILE_WORKER_GUARD);
    return 0;
}
