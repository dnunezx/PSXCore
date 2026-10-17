/* Native ATA shutdown after the POPS IOP reset. No EE callback survives
 * the game handoff, so the complete button path stays on the IOP. */
#ifdef NP_HOST_TEST
#include "powerbutton_stub.h"
#else
#include "irx_imports.h"
#endif
IRX_ID("psxcore_powerbutton",1,0);
static int request_sema;
static volatile int requested;

static void button_pressed(void *arg) {
    (void)arg;
    /* Called by poweroff.irx in CDVD interrupt context. HDD callbacks and
     * DEV9's settling delay require normal thread context. Latch once. */
    if(!requested) {
        requested=1;
        iSignalSema(request_sema);
    }
}
static void shutdown_thread(void *arg) {
    (void)arg;
    WaitSema(request_sema);
    /* Use the unchanged native driver's shutdown callbacks and power-down
     * sequence before requesting console standby. No game-save drain. */
    dev9Shutdown();
    PoweroffShutdown();
    /* Console power should now be off. Never resume storage or run the
     * shutdown callbacks a second time if hardware has not powered down. */
    for(;;)WaitSema(request_sema);
}
int _start(int argc,char **argv) {
    (void)argc;(void)argv;
    iop_sema_t sema={.attr=0,.option=0,.initial=0,.max=1};
    request_sema=CreateSema(&sema);
    if(request_sema<0)return MODULE_NO_RESIDENT_END;
    iop_thread_t thread={.attr=TH_C,.option=0,.thread=shutdown_thread,
        .stacksize=0x1000,.priority=0x27};
    int id=CreateThread(&thread);
    if(id<0){DeleteSema(request_sema);return MODULE_NO_RESIDENT_END;}
    if(StartThread(id,NULL)<0) {
        DeleteThread(id);DeleteSema(request_sema);
        return MODULE_NO_RESIDENT_END;
    }
    SetPowerButtonHandler(button_pressed,NULL);
    return MODULE_RESIDENT_END;
}
