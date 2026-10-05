/* Original version-checked runtime probe; explicit tracing changes staged RAM only. */
#include "psxcore_boot.h"
#include "psxcore_launch.h"
#include "psxcore_patch.h"
#include "config.h"
#include "saveguards.h"
#include <kernel.h>
#include <sifrpc.h>
#include <iopcontrol.h>
#include <iopcontrol_special.h>
#include <iopheap.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <stdarg.h>
#include <ee_regs.h>
_Static_assert(offsetof(NpBootPlan,argc)==136 && offsetof(NpBootPlan,argv)==140,"transfer offsets");
extern void np_runtime_transfer(const NpBootPlan *) __attribute__((noreturn));
extern void np_mount_hook(void),np_disc_hook(void),np_card_hook(void);
extern void np_post_0(void),np_post_1(void),np_post_2(void),np_post_3(void),np_post_4(void),np_post_5(void),np_post_6(void);
extern void np_core_hook(void),np_sector_rpc_hook(void);
extern void np_sector_consumer_hook(void),np_sector_request_hook(void),np_rpc_completion_hook(void);
extern void np_sector_consumer_quiet(void);
extern void np_asset_copy_hook(void),np_asset_dma_hook(void),np_graphics_copy_hook(void);
extern void np_graphics_pixels_hook(void),np_graphics_upload_hook(void);
extern void np_graphics_readback_start(void);
extern uint32_t np_disc_probe_enabled;
extern uint32_t np_firmware_license_enabled;
extern uint32_t np_asset_trace_enabled;
extern uint32_t np_native_request_enabled, np_trace_enabled;
extern char np_save_partition[40];
extern void np_save_mount_hook(void);
extern void np_save_open_hook(void),np_save_write_hook(void),np_save_close_hook(void);
extern void np_save_rename_hook(void),np_save_sync_hook(void),np_save_wait_hook(void);
extern void np_save_sema_hook(void),np_save_thread_hook(void),np_save_start_hook(void);
static NpPatchSet patch_set;
static int queue_word(uint32_t address, uint32_t expected, uint32_t replacement) {
    return np_patch_add(&patch_set,address,&expected,&replacement,4)?-1:0;
}
static int install_hooks(const NpElf *elf,int game,int minimal,int assets,int native) {
    const uint32_t sites[24]={0x214968,0x2149a0,0x200790,0x2007b0,0x200828,0x200834,0x200868,0x200870,0x200888,0x2008a0,0x2008dc,0x200910,0x210068,0x2101b4,
        0x20e880,0x20eae4,0x20ea04,0x20eca4,0x20ef14,0x20f0e4,0x20ff14,0x20e9a8,0x20e550,0x21a918};
    const uint32_t targets[24]={0x233650,0x233650,0x210540,0x20cf00,0x22d2e0,0x22d480,0x22dc30,0x214418,0x20c750,0x200940,0x200d10,0x200d50,0x230330,0x230330,
        0x210368,0x210368,0x210290,0x210290,0x210290,0x210290,0x22e850,0x209e00,0x209e00,0x209e00};
    const uintptr_t hooks[24]={native?(uintptr_t)np_save_mount_hook:(uintptr_t)np_mount_hook,(uintptr_t)np_mount_hook,(uintptr_t)np_disc_hook,(uintptr_t)np_card_hook,
        (uintptr_t)np_post_0,(uintptr_t)np_post_1,(uintptr_t)np_post_2,(uintptr_t)np_post_3,(uintptr_t)np_post_4,(uintptr_t)np_post_5,(uintptr_t)np_post_6,
        (uintptr_t)np_core_hook,(uintptr_t)np_sector_rpc_hook,(uintptr_t)np_sector_rpc_hook,
        (uintptr_t)(minimal&&!assets?np_sector_consumer_quiet:np_sector_consumer_hook),(uintptr_t)(minimal&&!assets?np_sector_consumer_quiet:np_sector_consumer_hook),(uintptr_t)np_sector_request_hook,(uintptr_t)np_sector_request_hook,
        (uintptr_t)np_sector_request_hook,(uintptr_t)np_sector_request_hook,(uintptr_t)np_rpc_completion_hook,(uintptr_t)np_asset_copy_hook,(uintptr_t)np_asset_dma_hook,(uintptr_t)np_graphics_copy_hook};
    uint32_t *words[24]={0};unsigned count=game?24:3;
    for(unsigned i=0;i<count;++i) {
        if(hooks[i]<0xf0000 || hooks[i]>=0xf7000 || (hooks[i]&3))return -1;
        for(unsigned j=0;j<elf->count;++j) {
            const NpSegment *segment=&elf->segments[j];
            if(sites[i]>=segment->address && sites[i]+8<=segment->address+segment->filesz)
                words[i]=(void *)(NP_BOOT_STAGE+segment->offset+sites[i]-segment->address);
        }
        /* Validate the interface call before replacing it; preserve its delay slot. */
        if(!words[i] || *words[i]!=(0x0c000000u|(targets[i]>>2)))return -1;
    }
    for(unsigned i=0;i<count;++i) {
        if(i>=21 && !assets)continue;
        if(minimal && i>=4 && i!=14 && i!=15 && !(assets && i>=21))continue;
        if(queue_word(sites[i],*words[i],0x0c000000u|((uint32_t)hooks[i]>>2)))return -1;
    }
    return 0;
}
/* Required v2 save guards. Preserve every original argument/delay slot and
 * prepare all redirects transactionally with the other staged patches. */
static int install_save_guards(const NpElf *elf) {
    const uintptr_t hooks[9]={(uintptr_t)np_save_open_hook,(uintptr_t)np_save_write_hook,
        (uintptr_t)np_save_close_hook,(uintptr_t)np_save_rename_hook,(uintptr_t)np_save_sync_hook,
        (uintptr_t)np_save_wait_hook,(uintptr_t)np_save_sema_hook,(uintptr_t)np_save_thread_hook,
        (uintptr_t)np_save_start_hook};
    return np_runtime_save_plan((void *)NP_BOOT_STAGE,3166988,elf,hooks,&patch_set)?-1:0;
}
/* Observe only verified image-worker boundaries. The first site displaces one
 * scalar comparison, executed by the hook on return; its delay slot stays put. */
static int install_graphics_hooks(const NpElf *elf) {
    const uint32_t sites[2]={0x224330,0x224b44};
    const uint32_t expected[2]={0x02a7102b,0x0c086da0};
    const uint32_t delays[2]={0x24030004,0xad850014};
    const uintptr_t hooks[2]={(uintptr_t)np_graphics_pixels_hook,(uintptr_t)np_graphics_upload_hook};
    uint32_t *words[2]={0};
    for(unsigned i=0;i<2;++i) {
        if(hooks[i]<0xf0000 || hooks[i]>=0xf7000 || (hooks[i]&3))return -1;
        for(unsigned j=0;j<elf->count;++j) {
            const NpSegment *s=&elf->segments[j];
            if(sites[i]>=s->address && sites[i]+8<=s->address+s->filesz)
                words[i]=(void *)(NP_BOOT_STAGE+s->offset+sites[i]-s->address);
        }
        if(!words[i] || words[i][0]!=expected[i] || words[i][1]!=delays[i])return -1;
    }
    for(unsigned i=0;i<2;++i)
        if(queue_word(sites[i],*words[i],0x0c000000u|((uint32_t)hooks[i]>>2)))return -1;
    return 0;
}
/* PCSX2 v2.6.3 FIFO writes consume the already-programmed DMA address/count.
 * Reapply the native readback descriptor after the two manual FIFO writes.
 * The redirect's li a1,2 precedes the hook; its fixed CHCR store is equivalent. */
static int install_readback_repair(const NpElf *elf) {
    const uintptr_t hook=(uintptr_t)np_graphics_readback_start;
    if(hook<0xf0000 || hook>=0xf7000 || (hook&3))return -1;
    for(unsigned i=0;i<elf->count;++i) {
        const NpSegment *s=&elf->segments[i];
        if(s->address<=0x21bfe8 && 0x21bff0<=s->address+s->filesz) {
            uint32_t *words=(void *)(NP_BOOT_STAGE+s->offset+0x21bfe8-s->address);
            if(words[0]!=0xaca70000 || words[1]!=0x24050002)return -1;
            return queue_word(0x21bfe8,words[0],0x0c000000u|((uint32_t)hook>>2));
        }
    }
    return -1;
}
/* Explicit validated-image experiments: replace stock verification jobs with
 * one SHA-1 check of our original header. Retain one job for worker signaling. */
static int adapt_disc_checks(const NpElf *elf,int tekken,const unsigned char *selected_header) {
    static const unsigned char header_sha1[20]={0xb2,0x9b,0xb8,0x80,0xc5,0x51,0x96,0xcd,0xd8,0xd4,0x5b,0x81,0xa3,0xea,0x29,0xf2,0x32,0x19,0x6a,0x18};
    static const unsigned char tekken_header_sha1[20]={0x5f,0xa2,0xa7,0x1e,0xc1,0xa0,0x08,0x2d,0xaf,0x4d,0x5a,0xcf,0x94,0x3d,0x13,0x11,0xbe,0xed,0x46,0x5b};
    for(unsigned i=0;i<elf->count;++i) {
        const NpSegment *s=&elf->segments[i];
        if(s->address<=0x2fe670 && 0x2fe670+96<=s->address+s->filesz) {
            uint32_t *table=(void *)(NP_BOOT_STAGE+s->offset+0x2fe670-s->address);
            if(table[0]!=2400 || table[1]!=64 || table[3]!=1 || table[8]!=38 || table[9]!=0 || table[10]!=35)return -1;
            /* Job offset/count use 512-byte sectors: count 512 is 256 KiB. */
            if(table[17]!=0 || table[18]!=512)return -1;
            if(queue_word(0x2fe670+32,table[8],1) || queue_word(0x2fe670+36,table[9],0) ||
                queue_word(0x2fe670+40,table[10],0) ||
                np_patch_add(&patch_set,0x2fe670+76,(unsigned char *)table+76,
                    selected_header?selected_header:tekken?tekken_header_sha1:header_sha1,20))return -1;
            return 0;
        }
    }
    return -1;
}
/* Pinned experiment: use the existing 72-byte firmware string slot for both
 * license-comparison branches. Its contents are supplied by a guarded sector-4
 * hook after a real native read; no firmware/game payload is stored in source. */
static int adapt_firmware_license(const NpElf *elf) {
    for(unsigned i=0;i<elf->count;++i) {
        const NpSegment *s=&elf->segments[i];
        const uint32_t site=0x263d48;
        if(s->address<=site && site+4<=s->address+s->filesz) {
            uint32_t *word=(void *)(NP_BOOT_STAGE+s->offset+site-s->address);
            if(*word!=0x24a5ea7c)return -1;
            return queue_word(site,*word,0x24a5e9f0);
        }
    }
    return -1;
}
/* PCSX2 diagnostic serial output survives FILEIO/RPC replacement. */
static void trace(const char *format,...) {
    char text[256];va_list args;va_start(args,format);
    int n=vsnprintf(text,sizeof(text),format,args);va_end(args);
    if(n<0)return;
    if(n>=(int)sizeof(text))n=(int)sizeof(text)-1;
    for(int i=0;i<n;++i)*R_EE_SIO_TXFIFO=(unsigned char)text[i];
}
static const unsigned char elf_hash[32]={
0x59,0xdf,0x33,0x89,0xc4,0xdf,0x88,0xa5,0x72,0xda,0xa7,0x20,0xb0,0x55,0x07,0xc5,
0x2c,0x34,0xed,0xdf,0xa0,0x03,0x1a,0x6f,0xbe,0xec,0x55,0xe0,0xc2,0xd0,0xfc,0xb1};
static const unsigned char img_hash[32]={
0x33,0x38,0xb2,0x38,0xd8,0x4d,0x7d,0x58,0x6b,0x71,0x66,0x77,0xe3,0xa1,0xc0,0x3b,
0x20,0x88,0xb8,0x82,0xec,0xfa,0x17,0xf9,0x1f,0xc3,0x37,0x98,0x93,0x1c,0xa3,0xba};
static int read_known(const char *path,void *target,unsigned size,const unsigned char hash[32]) {
    FILE *f=fopen(path,"rb");if(!f)return -1;
    if(fseek(f,0,SEEK_END)){fclose(f);return -1;}
    long n=ftell(f);
    if(n!=(long)size || fseek(f,0,SEEK_SET)){fclose(f);return -1;}
    size_t got=fread(target,1,size,f);int result=fclose(f);
    if(got!=size || result)return -1;
    unsigned char actual[32];np_boot_sha256(target,size,actual);
    return memcmp(actual,hash,32)?-1:0;
}
static int profile(const NpElf *elf) {
    static const unsigned expected[4][3]={
        {0x100000,0,0x100000},{0x200000,0x49808,0x49808},
        {0x249880,0x2b95e0,0x61c0c0},{0x70000000,0,0x3c30}};
    if(elf->entry!=0x200008 || elf->count!=4)return -1;
    for(unsigned i=0;i<4;++i)if(elf->segments[i].address!=expected[i][0] ||
        elf->segments[i].filesz!=expected[i][1] || elf->segments[i].memsz!=expected[i][2])return -1;
    return 0;
}
int main(void) {
    SifInitRpc(0);init_scr();
    scr_printf("PSXCore experimental runtime bootstrap\n");
    if((unsigned)GetMemorySize()<NP_BOOT_STACK)goto rejected;
    char mode[32]={0};FILE *config=fopen("host:/psxcore-runtime-mode.txt","rb");
    if(!config)goto rejected;
    size_t got=fread(mode,1,sizeof(mode)-1,config);int extra=fgetc(config),bad_read=ferror(config),closed=fclose(config);
    if(closed || bad_read || extra!=EOF || !np_runtime_mode_valid(mode,got)) {
        trace("PSXCORE rejected=runtime-mode\n");goto rejected;
    }
    int native=!strcmp(mode,"launch\n");
    NpLaunchRequest request;
    NpCompatResult resolved;
    if(native) {
        FILE *f=fopen("host:/psxcore-launch.bin","rb");
        if(!f){trace("PSXCORE rejected=launch-file\n");goto rejected;}
        size_t n=fread(&request,1,sizeof(request),f);
        int trailing=fgetc(f),io_error=ferror(f),close_error=fclose(f);
        const char *reason=np_launch_check(&request,n,&resolved);
        if(reason || trailing!=EOF || io_error || close_error) {
            trace("PSXCORE rejected=%s\n",reason?reason:"launch-file-size-or-io");goto rejected;
        }
        memcpy(np_save_partition,request.save_partition,sizeof(np_save_partition));
        np_native_request_enabled=1;
        np_trace_enabled=request.options.diagnostics;
        trace("PSXCORE launch=v2 profile=%s revision=%u fixes=%x environment=%u diagnostics=%u\n",
            resolved.profile,(unsigned)resolved.profile_revision,(unsigned)resolved.fixes,
            (unsigned)request.environment,(unsigned)request.options.diagnostics);
        trace("PSXCORE saves=%s key=%s mode=%s\n",request.save_partition,request.save_key,
            request.save_mode==NP_SAVE_SHARED?"shared-explicit":"isolated");
    }
    int tekken=!strcmp(mode,"hddlicensetekken\n");
    int hdd=mode[0]=='h',image=mode[0]!='s',probe=!strcmp(mode,"hdddisc\n");
    int verify=!strcmp(mode,"hddverify\n");
    int assets=!strcmp(mode,"hddlicenseassets\n");
    int repair=tekken || !strcmp(mode,"hddlicensegraphicsfix\n") || !strcmp(mode,"hddlicensereadback\n");
    int graphics=!strcmp(mode,"hddlicensegraphics\n") || !strcmp(mode,"hddlicensegraphicsfix\n");
    int minimal=repair || graphics || assets || !strcmp(mode,"hddlicensequiet\n");
    int license=minimal || !strcmp(mode,"hddlicense\n");
    int adapt=license || !strcmp(mode,"hddadapt\n"),game=adapt || verify || !strcmp(mode,"hddgame\n"),hooks=probe || game || !strcmp(mode,"hddtrace\n");
    if(native) {
        hdd=image=adapt=game=hooks=minimal=1;
        license=!!(resolved.fixes&NP_FIX_LICENSE);
        repair=!!(resolved.fixes&NP_FIX_READBACK);
    }
    if(read_known("host:/POPS.ELF",(void *)NP_BOOT_STAGE,3166988,elf_hash)) {
        trace("PSXCORE rejected=runtime-file-identity\n");goto rejected;
    }
    if(read_known("host:/IOPRP252.IMG",(void *)NP_BOOT_IOP,265233,img_hash)) {
        trace("PSXCORE rejected=iop-file-identity\n");goto rejected;
    }
    NpElf elf;const char *error=np_boot_elf_check((void *)NP_BOOT_STAGE,3166988,&elf);
    if(error || profile(&elf)) {
        trace("PSXCORE rejected=%s\n",error?error:"runtime-layout-profile");goto rejected;
    }
    if(hooks && install_hooks(&elf,game,minimal,assets,native)) {
        trace("PSXCORE rejected=interface-patch-plan\n");goto rejected;
    }
    if(native && install_save_guards(&elf)) {
        trace("PSXCORE rejected=save-guard-patch-plan\n");goto rejected;
    }
    if(graphics && install_graphics_hooks(&elf)) {
        trace("PSXCORE rejected=graphics-patch-plan\n");goto rejected;
    }
    if(repair && install_readback_repair(&elf)) {
        trace("PSXCORE rejected=readback-patch-plan\n");goto rejected;
    }
    if(adapt && adapt_disc_checks(&elf,tekken,native?request.header_sha1:NULL)) {
        trace("PSXCORE rejected=disc-check-patch-plan\n");goto rejected;
    }
    if(license && adapt_firmware_license(&elf)) {
        trace("PSXCORE rejected=license-patch-plan\n");goto rejected;
    }
    error=np_patch_apply((void *)NP_BOOT_STAGE,3166988,&elf,&patch_set);
    if(error){trace("PSXCORE rejected=%s\n",error);goto rejected;}
    np_firmware_license_enabled=license;
    np_asset_trace_enabled=assets;
    np_disc_probe_enabled=verify?2:probe;
    NpBootPlan *plan=(void *)NP_BOOT_PLAN;
    memset(plan,0,sizeof(*plan));plan->elf=elf;plan->argc=1;
    if(native) {
        error=np_launch_argument(&request,plan->strings[0],sizeof(plan->strings[0]));
        if(error){trace("PSXCORE rejected=%s\n",error);goto rejected;}
    } else strcpy(plan->strings[0],game?"hdd0:PP.PSXCOREGAME:/disc/disc0":hdd?"hdd0:__.POPS:/disc/disc0":"POPS.ELF");
    plan->argv[0]=(uint32_t)plan->strings[0];
    trace("PSXCORE runtime=verified profile=known argv0=%s hdd-branch=%s mode=%.*s\n",
        plan->strings[0],hdd?"yes":"no",(int)(got-1),mode);
    trace("PSXCORE runtime=prepared ram-hooks=%s patches=%u file=unmodified\n",
        hooks?"yes":"no",patch_set.count);
    if(adapt)trace("PSXCORE compatibility=original-header-check jobs=1 stock-title-checks=no file=unmodified\n");
    if(tekken)trace("PSXCORE compatibility=title-profile game=SLUS_004.02 tracks=3\n");
    if(license)trace("PSXCORE compatibility=firmware-license-experiment source=pinned-native-sector file=unmodified\n");
    if(assets)trace("PSXCORE diagnostic=asset-reads samples-max=64 boot-sectors=excluded\n");
    if(graphics)trace("PSXCORE diagnostic=graphics-uploads samples-max=16 boundary=completed-gather no-worker-wait=yes\n");
    if(repair)trace("PSXCORE compatibility=pcsx2-readback-descriptor restore-before-start=yes file=unmodified\n");
    fflush(stdout);
    if(image) {
        trace("PSXCORE iop=image-reset-begin\n");
        int reset=SifIopRebootBuffer((void *)NP_BOOT_IOP,265233);
        trace("PSXCORE iop=image-reset-submitted result=%d\n",reset);
        if(reset!=1)goto failed;
        unsigned i;for(i=0;i<300 && !SifIopSync();++i)usleep(10000);
        if(i==300)goto failed;
        SifInitRpc(0);
        trace("PSXCORE iop=image-synchronized\n");
        int patched=sbv_patch_enable_lmb();
        trace("PSXCORE iop=lmb-patch result=%d\n",patched);
        if(patched<=0)goto failed;
    }
    trace("PSXCORE iop=service-probe-begin\n");
    int load=SifLoadFileInit(),heap=SifInitIopHeap();
    void *allocation=heap>=0?SifAllocIopHeap(64):NULL;
    trace("PSXCORE iop=services loadfile=%d heap=%d allocation=%s\n",load,heap,allocation?"passed":"FAILED");
    if(load<0 || heap<0 || !allocation)goto failed;
    SifFreeIopHeap(allocation);
    trace("PSXCORE runtime=transfer entry=00200008 argc=1 source=external-file-unmodified\n");
    fflush(stdout);TerminateLibrary();np_runtime_transfer(plan);
rejected:
    trace("PSXCORE runtime=rejected before-iop-change=yes\n");goto wait;
failed:
    trace("PSXCORE runtime=preparation-failed\n");
wait:
    for(;;)nopdelay();
    return 0;
}
