/* Native POPS quit -> independent SDK runtime -> memory-card ELF.
 * Return sequence follows LUNA/Neutrino: clean ROM IOP, memory-card modules,
 * SifLoadElf, then ExecPS2 with the configured path and returned GP.
 * No automatic save, save-worker wakeup, drain, or browser fallback. */
#include "psxcore_igr.h"
#include <kernel.h>
#include <sifrpc.h>
#include <sifcmd.h>
#include <loadfile.h>
#include <iopcontrol.h>
#include <iopheap.h>
#include <sbv_patches.h>
#include <ps2sdkapi.h>
#include <fileio.h>
#include <string.h>
#include "target.h"

void _libcglue_init(void) {}
void _libcglue_deinit(void) {}
DISABLE_PATCHED_FUNCTIONS();
DISABLE_EXTRA_TIMERS_FUNCTIONS();
PS2_DISABLE_AUTOSTART_PTHREAD();

#include "igrloader_config.h"
_Static_assert(sizeof(NP_IGR_RETURN_PATH)<=NP_IGR_PATH_BYTES,"IGR return path size");
static char return_path[]=NP_IGR_RETURN_PATH;
static char *return_argv[]={return_path,NULL};
/* The resident loader and its stack end below 0xef000. Return executables
 * must load above that boundary, as with Neutrino's resident return code.
 * phase: 1 entry, 2 reset, 3 modules, 4 SDK ELF load, 7 exec.
 * bytes is reserved (zero); the SDK does not report the ELF file size.
 * This is diagnostic progress, never a save-completion/durability claim. */
volatile struct { unsigned magic,version,phase; int error; unsigned entry,bytes; }
    np_igr_return_status={0x5257504e,1,0,0,0,0};

/* POPStarter's native reset contract includes the argument terminator and
 * keeps command/RPC reception installed until the reset is acknowledged. */
static int reset_iop(void) {
    static struct {
        SifCmdResetData_t data;
        unsigned padding[2];
    } packet __attribute__((aligned(64)));
    _Static_assert(sizeof(packet)==112,"Native reset packet size");
    static const char command[]="rom0:UDNL ";
    extern int _iop_reboot_count;
    _iop_reboot_count++;
    sceSifStopDma();
    memset(&packet,0,sizeof(packet));
    packet.data.header.psize=sizeof(packet);
    packet.data.header.cid=SIF_CMD_RESET_CMD;
    packet.data.arglen=sizeof(command);
    packet.data.mode=NP_IGR_RESET_DEBUG ? (int)0x80000000u : 0;
    memcpy(packet.data.arg,command,sizeof(command));
    SifDmaTransfer_t dma={.src=&packet,.dest=(void *)(uintptr_t)(unsigned)sceSifGetReg(SIF_SYSREG_SUBADDR),
        .size=sizeof(packet),.attr=SIF_DMA_ERT|SIF_DMA_INT_O};
    sceSifWriteBackDCache(&packet,sizeof(packet));
    sceSifSetReg(SIF_REG_SMFLAG,SIF_STAT_BOOTEND);
    if(!sceSifSetDma(&dma,1))return 0;
    sceSifSetReg(SIF_REG_SMFLAG,SIF_STAT_SIFINIT);
    sceSifSetReg(SIF_REG_SMFLAG,SIF_STAT_CMDINIT);
    sceSifSetReg(SIF_SYSREG_RPCINIT,0);
    sceSifSetReg(SIF_SYSREG_SUBADDR,0);
    return 1;
}
static unsigned ticks(void) {
    unsigned value;__asm__ volatile("mfc0 %0,$9":"=r"(value));return value;
}
static void fail(int error) __attribute__((noreturn));
static void fail(int error) {
    np_igr_return_status.error=error;
    /* Leave a visible failure indication without using old POPS libraries. */
    *(volatile unsigned long *)0x120000e0=0x000000ff;
    for(;;)__asm__ volatile("nop");
}
void np_igr_clean_exec(void *entry,void *gp,int argc,char **argv) __attribute__((noreturn));
void np_igr_exec_failed(void) __attribute__((noreturn));
void np_igr_exec_failed(void) { fail(-14); }
static int read_exact(int fd,void *buffer,unsigned bytes) {
    unsigned done=0;
    while(done<bytes) {
        int n=fioRead(fd,(unsigned char *)buffer+done,bytes-done);
        if(n<=0 || (unsigned)n>bytes-done)return 0;
        done+=(unsigned)n;
    }
    return 1;
}
static int target_layout(NpIgrTarget *target) {
    unsigned char header[52],ph[8*32];uint32_t offset;unsigned count;
    int fd=fioOpen(return_path,FIO_O_RDONLY);if(fd<0)return 0;
    int bytes=fioLseek(fd,0,2); /* SEEK_END; native fileIO flags/offsets. */
    int ok=bytes>=52 && fioLseek(fd,0,0)==0 && read_exact(fd,header,sizeof(header)) &&
        np_igr_target_header(header,(unsigned)bytes,&offset,&count) &&
        fioLseek(fd,(int)offset,0)==(int)offset && read_exact(fd,ph,count*32) &&
        np_igr_target_plan(header,ph,(unsigned)bytes,target);
    int closed=fioClose(fd);
    return ok && closed>=0;
}
int main(int argc,char **argv) {
    (void)argc;(void)argv;
    np_igr_return_status.phase=1;
    if(!np_igr_path_valid(return_path,sizeof(return_path)))fail(-1);
    ResetEE(0x7f);
    np_igr_return_status.phase=2;
    SifInitRpc(0);
    unsigned started=ticks();
    while(!reset_iop())if((unsigned)(ticks()-started)>1474560000u)fail(-2);
    while(!SifIopSync())if((unsigned)(ticks()-started)>1474560000u)fail(-3);
    SifInitRpc(0);
    if(SifInitIopHeap()<0)fail(-8);
    if(SifLoadFileInit()<0)fail(-4);
    if(sbv_patch_enable_lmb()<0)fail(-15);
    if(sbv_patch_disable_prefix_check()<0)fail(-16);
    np_igr_return_status.phase=3;
    static const char *const modules[]={"rom0:SIO2MAN","rom0:MCMAN","rom0:MCSERV"};
    for(unsigned i=0;i<3;++i)if(SifLoadModule(modules[i],0,NULL)<0)fail(-5-(int)i);
    if((unsigned)GetMemorySize()!=0x02000000u)fail(-17);
    NpIgrTarget target;
    if(!target_layout(&target))fail(-18);
    np_igr_return_status.phase=4;
    t_ExecData elf={0};
    if(SifLoadElf(return_path,&elf)<0 || elf.epc!=target.entry)fail(-9);
    np_igr_return_status.entry=elf.epc;
    fioExit();SifExitIopHeap();SifLoadFileExit();SifExitRpc();
    /* Old guest communication state remains available until loading ends.
     * Then clean every user-memory gap around the admitted PT_LOAD ranges.
     * Packed targets may omit trailing zeros in their unpacked data: inherited
     * POPS bytes there can corrupt pointers. Preserve the loaded target, our
     * low loader/private stack, and the top 64 KiB native ExecPS2 handoff. */
    uintptr_t cursor=NP_IGR_USER_FIRST;
    for(unsigned i=0;i<target.count;++i) {
        memset((void *)cursor,0,target.ranges[i].first-cursor);
        cursor=target.ranges[i].end;
    }
    memset((void *)cursor,0,NP_IGR_USER_END-cursor);
    FlushCache(0);FlushCache(2);
    np_igr_return_status.phase=7;
    np_igr_clean_exec((void *)(uintptr_t)elf.epc,(void *)(uintptr_t)elf.gp,1,return_argv);
}
