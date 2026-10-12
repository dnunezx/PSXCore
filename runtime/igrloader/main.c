/* Native menu quit -> independent SDK runtime -> memory-card ELF.
 * No automatic save, save-worker wakeup, drain, or browser fallback. */
#include "psxcore_elf.h"
#include "psxcore_igr.h"
#include <kernel.h>
#include <sifrpc.h>
#include <sifcmd.h>
#include <loadfile.h>
#include <iopcontrol.h>
#define NEWLIB_PORT_AWARE
#include <fileio.h>
#include <ps2sdkapi.h>
#include <string.h>

void _libcglue_init(void) {}
void _libcglue_deinit(void) {}
DISABLE_PATCHED_FUNCTIONS();
DISABLE_EXTRA_TIMERS_FUNCTIONS();
PS2_DISABLE_AUTOSTART_PTHREAD();

#include "igrloader_config.h"
_Static_assert(sizeof(NP_IGR_RETURN_PATH)<=NP_IGR_PATH_BYTES,"IGR return path size");
static char return_path[]=NP_IGR_RETURN_PATH;
static char *return_argv[]={return_path,NULL};
/* Independent record survives target loads, because they start at 1 MiB.
 * phase: 1 entry, 2 reset, 3 modules, 4 read, 5 checked, 6 copy, 7 exec.
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
    if(SifLoadFileInit()<0)fail(-4);
    np_igr_return_status.phase=3;
    static const char *const modules[]={"rom0:SIO2MAN","rom0:MCMAN","rom0:MCSERV"};
    for(unsigned i=0;i<3;++i)if(SifLoadModule(modules[i],0,NULL)<0)fail(-5-(int)i);
    if(fioInit()<0)fail(-8);
    np_igr_return_status.phase=4;
    int fd=fioOpen(return_path,O_RDONLY);
    if(fd<0)fail(-9);
    int bytes=fioLseek(fd,0,SEEK_END);
    if(bytes<52 || (unsigned)bytes>NP_ELF_MAX_BYTES || fioLseek(fd,0,SEEK_SET)!=0) {
        fioClose(fd);fail(-10);
    }
    unsigned char *stage=(void *)0x01800000;
    unsigned got=0;
    while(got<(unsigned)bytes) {
        unsigned chunk=(unsigned)bytes-got;if(chunk>16384)chunk=16384;
        int n=fioRead(fd,stage+got,(int)chunk);
        if(n<=0 || (unsigned)n>chunk){fioClose(fd);fail(-11);}
        got+=(unsigned)n;
    }
    if(fioClose(fd)<0)fail(-12);
    NpElf elf;
    if(np_elf_check(stage,got,&elf))fail(-13);
    np_igr_return_status.entry=elf.entry;
    np_igr_return_status.bytes=got;
    np_igr_return_status.phase=5;
    /* All validation precedes every target-memory write. The existing core
     * contract excludes this loader/stack, kernel and staging buffer. */
    np_igr_return_status.phase=6;
    for(unsigned i=0;i<elf.count;++i) {
        const NpSegment *s=&elf.segments[i];
        memcpy((void *)(uintptr_t)s->address,stage+s->offset,s->filesz);
        memset((void *)(uintptr_t)(s->address+s->filesz),0,s->memsz-s->filesz);
    }
    fioExit();SifLoadFileExit();SifExitRpc();
    FlushCache(0);FlushCache(2);
    np_igr_return_status.phase=7;
    /* Like the existing bootstrap, SDK crt0 establishes the target's GP. */
    ExecPS2((void *)(uintptr_t)elf.entry,NULL,1,return_argv);
    fail(-14);
}
