/* Original version-checked runtime probe; explicit tracing changes staged RAM only. */
#include "psxcore_boot.h"
#include "psxcore_launch.h"
#include "psxcore_patch.h"
#include "psxcore_runtime.h"
#include "psxcore_hdtv.h"
#include "psxcore_file_patch.h"
#include "file_storage.h"
#include "startup_diag.h"
#include "config.h"
#ifndef NP_GPU_PACKET_FASTPATH
#define NP_GPU_PACKET_FASTPATH 1
#endif
#ifndef NP_CD_COMMAND_ORDER
#define NP_CD_COMMAND_ORDER 1
#endif
#if (NP_GPU_PACKET_FASTPATH != 0 && NP_GPU_PACKET_FASTPATH != 1) || \
    (NP_CD_COMMAND_ORDER != 0 && NP_CD_COMMAND_ORDER != 1)
#error "Packet fast path and CD command ordering must be 0 or 1"
#endif
#if NP_GPU_PACKET_FASTPATH
extern void np_packet_fast(void);
extern char np_packet_fast_template[],np_packet_fast_end[];
#include "polygonpacket_patch.h"
#endif
#if NP_CD_COMMAND_ORDER
extern void np_cd_order_command(void),np_cd_order_after_ack(void);
extern char np_cd_order_begin[],np_cd_order_end[];
#endif
#include "gamepatches.h"
#include "saveguards.h"
#include "gtenative_three_patch.h"
#if NP_CD_SWAP
#include "cdswap_patch.h"
#endif
#ifndef NP_IGR_WAIT
#define NP_IGR_WAIT 0
#endif
#if NP_IGR_WAIT != 0 && NP_IGR_WAIT != 1
#error "NP_IGR_WAIT must be 0 or 1"
#endif
#if NP_IGR_WAIT
#include "igrwait.h"
#endif
#ifndef NP_CD_OBSERVE
#define NP_CD_OBSERVE 0
#endif
#ifndef NP_CD_RENDEZVOUS
#define NP_CD_RENDEZVOUS 0
#endif
#if NP_CD_RENDEZVOUS != 0 && NP_CD_RENDEZVOUS != 1
#error "NP_CD_RENDEZVOUS must be 0 or 1"
#endif
#if NP_CD_RENDEZVOUS && !NP_CD_OBSERVE
#error "NP_CD_RENDEZVOUS requires the independent CD worker observations"
#endif
#if NP_CD_RENDEZVOUS
#include "cdrendezvous.h"
#endif
#if NP_CD_OBSERVE != 0 && NP_CD_OBSERVE != 1
#error "NP_CD_OBSERVE must be 0 or 1"
#endif
#if NP_CD_OBSERVE
#include "cdobserve.h"
extern void np_cd_read_rpc(void),np_cd_prefetch_rpc(void),np_cd_prefetch_callback(void);
extern void np_cd_response_retire(void),np_cd_getstat_retire(void),np_cd_deferred_callback(void);
extern void np_cd_ack_irq(void),np_cd_worker_read(void);
extern char np_cd_observe_begin[],np_cd_observe_end[];
extern NpCdObservation np_cd_observation;
#endif
#ifndef NP_IGR_OBSERVE
#define NP_IGR_OBSERVE 0
#endif
#if NP_IGR_OBSERVE != 0 && NP_IGR_OBSERVE != 1
#error "NP_IGR_OBSERVE must be 0 or 1"
#endif
#if NP_IGR_OBSERVE
#include "igrobserve.h"
extern void np_igr_pad_copy_observe(void),np_igr_alarm_observe(void);
extern void np_igr_serial_observe(void),np_igr_event_observe(void);
extern NpIgrBoundaryObservation np_igr_boundary_observation;
extern char np_igr_observe_begin[],np_igr_observe_end[];
extern NpIgrObservation np_igr_observation;
#endif
#include <kernel.h>
#ifndef NP_IGR_PROBE
#define NP_IGR_PROBE 0
#endif
#if NP_IGR_PROBE != 0 && NP_IGR_PROBE != 1
#error "NP_IGR_PROBE must be 0 or 1"
#endif
#if NP_IGR_PROBE && (!NP_IGR_WAIT || !NP_IGR_OBSERVE || NP_CD_OBSERVE)
#error "IGR probe requires WAIT/IGR observations and excludes CD observations"
#endif
#ifndef NP_IGR_QUIT_PROBE
#define NP_IGR_QUIT_PROBE 0
#endif
#if NP_IGR_QUIT_PROBE != 0 && NP_IGR_QUIT_PROBE != 1
#error "NP_IGR_QUIT_PROBE must be 0 or 1"
#endif
#if NP_IGR_QUIT_PROBE && (!NP_IGR_OBSERVE || NP_IGR_PROBE)
#error "IGR quit probe requires observations and excludes the custom wait probe"
#endif
#if NP_IGR_QUIT_PROBE
#include "igrquit.h"
extern void np_igr_quit_probe(void);
extern char np_igr_quit_begin[],np_igr_quit_end[];
extern NpIgrQuitObservation np_igr_quit_observation;
#endif
#ifndef NP_IGR_RETURN
#define NP_IGR_RETURN 0
#endif
#if NP_IGR_RETURN != 0 && NP_IGR_RETURN != 1
#error "NP_IGR_RETURN must be 0 or 1"
#endif
#if NP_IGR_RETURN && (NP_IGR_QUIT_PROBE || NP_IGR_PROBE)
#error "Native IGR return excludes quit deferral and custom wait probes"
#endif
#if NP_IGR_RETURN
#include "psxcore_igr.h"
#include "igrreturn.h"
#include "igrmenu.h"
#include "igrloader_symbols.h"
extern unsigned char np_igr_loader_blob[];
extern unsigned int size_np_igr_loader_blob;
_Static_assert(NP_IGR_LOADER_ENTRY==0x84000,"IGR loader entry");
_Static_assert(NP_IGR_LOADER_END<=0xe0000,"IGR loader private stack overlap");
#endif
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
extern void np_memory_emit_finalize(void);
extern void np_memory_group_finalize(void);
extern void np_memory_wait_writer(void),np_memory_wait_writer_tail(void),np_memory_wait_dma(void);
extern void np_memory_wait_reader(void),np_memory_wait_video(void),np_memory_wait_worker(void);
extern void np_video_trace_entry(void),np_video_trace_decision(void),np_video_trace_exit(void);
extern void np_video_trace_csr(void),np_video_trace_command(void),np_video_trace_latch(void);
extern void np_video_trace_reset(void),np_video_trace_crt(void);
extern void np_video_interlace_applied(void);
extern void np_rgb24_buffered(void),np_rgb24_dispatch(void),np_rgb24_update(void);
static NpPatchSet patch_set;
extern void np_hdtv_begin(void),np_hdtv_display(void),np_hdtv_crt(void),np_hdtv_height(void),np_hdtv_end(void);
#if NP_INTERLACE_BUFFERED_PRESENT
#if !NP_RGB24_SCANOUT_ADAPTER
#error NP_INTERLACE_BUFFERED_PRESENT requires NP_RGB24_SCANOUT_ADAPTER
#endif
extern void np_interlace_fill(void),np_interlace_flat(void),np_interlace_textured(void);
extern void np_interlace_rectangle(void),np_interlace_line(void),np_interlace_end(void);
static int install_interlace_presentation_adapter(void) {
    const uint32_t sites[5]={0x21ce88,0x21daf0,0x21e3d8,0x21fa1c,0x220694};
    const uint32_t expected[5][2]={{0x8c622268,0x34420002},{0x8c622268,0x34440002},
        {0x8cc22268,0x34430002},{0x8c622268,0x34420002},{0x8c622268,0x34540002}};
    const uintptr_t hooks[5]={(uintptr_t)np_interlace_fill,(uintptr_t)np_interlace_flat,
        (uintptr_t)np_interlace_textured,(uintptr_t)np_interlace_rectangle,(uintptr_t)np_interlace_line};
    const uintptr_t end=(uintptr_t)np_interlace_end;
    if(end<=0xf7900 || end>0xf8000)return -1;
    for(unsigned i=0;i<5;++i) {
        if(hooks[i]<0xf7900 || hooks[i]>=end || (hooks[i]&3))return -1;
        const uint32_t replacement[2]={0x08000000u|((uint32_t)hooks[i]>>2),0};
        if(np_patch_add(&patch_set,sites[i],expected[i],replacement,8))return -1;
    }
    return 0;
}
#endif
static int queue_word(uint32_t address,uint32_t expected,uint32_t replacement);
#if NP_GP1_IDLE_CONTROL && !NP_MEMORY_ACCESS_TRACE
#include "gp1idle_patch.h"
#endif
#ifndef NP_POPS_BLOCK_LIMIT
#define NP_POPS_BLOCK_LIMIT 0
#endif
#if NP_POPS_BLOCK_LIMIT < 0 || NP_POPS_BLOCK_LIMIT > 256
#error "POPS block limit must be 0 (native default) or 1..256 instructions"
#endif
extern void np_code_guard_compile(void),np_code_guard_validate(void),np_code_guard_capture(void);
extern char np_code_guard_resident_start[],np_code_guard_resident_end[];
static int install_code_guard(const NpElf *elf) {
    const uintptr_t start=(uintptr_t)np_code_guard_resident_start;
    const uintptr_t end=(uintptr_t)np_code_guard_resident_end;
    const uintptr_t hook=(uintptr_t)np_code_guard_compile;
    const uintptr_t validator=(uintptr_t)np_code_guard_validate;
    const uintptr_t capture=(uintptr_t)np_code_guard_capture;
    if(start!=0xff000 || end<=start || end>0x100000 ||
       hook<start || hook>=end || validator<start || validator>=end ||
       capture<start || capture>=end || (hook&3) || (validator&3) || (capture&3))return -1;
    /* Pin the compiler call/delay, cache reservation and the decoder's raw
     * opcode field plus inclusive 48-byte record iteration contract. */
    const uint32_t sites[4]={0x20273c,0x202288,0x2041a4,0x202970};
    const uint32_t expected[4][2]={{0x0c08215a,0x0100282d},
        {0x2442e000,0x0043102b},{0xae050010,0xae11001c},
        {0x26100030,0x02d0102b}};
    for(unsigned j=0;j<4;++j) {
        int found=0;
        for(unsigned i=0;i<elf->count;++i) {
            const NpSegment *s=&elf->segments[i];
            if(sites[j]>=s->address && sites[j]+8<=s->address+s->filesz) {
                const void *words=(const void *)(NP_BOOT_STAGE+s->offset+sites[j]-s->address);
                if(memcmp(words,expected[j],8))return -1;
                found=1;break;
            }
        }
        if(!found)return -1;
    }
    /* Inline comparisons/snapshots add less than 12 KiB to the original
     * maximum block. Reserve it before native allocation/compilation. */
    const uint32_t replacement[2]={0x08000000u|((uint32_t)capture>>2),expected[2][0]};
    return queue_word(sites[0],expected[0][0],0x0c000000u|((uint32_t)hook>>2)) ||
           queue_word(sites[1],expected[1][0],0x2442b000) ||
           np_patch_add(&patch_set,sites[2],expected[2],replacement,8)?-1:0;
}
#if NP_PCM_BUFFER_RATE
extern void np_pcm_buffer_rate(void),np_pcm_buffer_rate_end(void);
static int install_pcm_buffer_rate(void) {
    const uintptr_t hook=(uintptr_t)np_pcm_buffer_rate;
    const uintptr_t end=(uintptr_t)np_pcm_buffer_rate_end;
    if(hook<0xf3cd0 || end<=hook || end>0xf4cd0 || (hook&3))return -1;
    const uint32_t expected[4]={0x8cb20008,0x2e4200fc,0x14600020,0x0082900a};
    const uint32_t replacement[4]={0x08000000u|((uint32_t)hook>>2),0,expected[2],expected[3]};
    return np_patch_add(&patch_set,0x213f3c,expected,replacement,sizeof(expected))?-1:0;
}
#endif
#if NP_RASTER_COST_FASTPATH
extern void np_raster_div100(void),np_raster_cost_end(void);
static int install_raster_cost_fastpath(void) {
    const uint32_t sites[4]={0x21ea94,0x21eae4,0x21f94c,0x21f998};
    const uint32_t expected[4][2]={{0x0c08f01a,0},{0x0c08f01a,0x0040202d},
        {0x0c08f01a,0},{0x0c08f01a,0x0040202d}};
    const uintptr_t hook=(uintptr_t)np_raster_div100;
    const uintptr_t end=(uintptr_t)np_raster_cost_end;
    if(hook<0xf3cd0 || end<=hook || end>0xf4cd0 || (hook&3))return -1;
    for(unsigned i=0;i<4;++i) {
        const uint32_t replacement[2]={0x0c000000u|((uint32_t)hook>>2),expected[i][1]};
        if(np_patch_add(&patch_set,sites[i],expected[i],replacement,8))return -1;
    }
    return 0;
}
#endif
#if NP_MDEC_BLOCK_FASTPATH
extern void np_mdec_fast_begin(void),np_mdec_fast_end(void);
extern void np_mdec_dc(void),np_mdec_flat(void),np_mdec_rle_dc(void),np_mdec_clear(void);
static int install_mdec_block_fastpaths(void) {
#if !NP_MEMORY_ACCESS_TRACE
    extern void np_mdec_row_exact(void),np_mdec_sparse2_exact(void),np_mdec_row_bridge(void),np_mdec_row_end(void);
    extern char np_video_worker_records[];
    const uintptr_t row=(uintptr_t)np_mdec_row_exact,row_bridge=(uintptr_t)np_mdec_row_bridge;
    const uintptr_t row_end=(uintptr_t)np_mdec_row_end,row_arena=(uintptr_t)np_video_worker_records;
    const uintptr_t sparse=(uintptr_t)np_mdec_sparse2_exact;
    if((row&15) || row_arena<0xf0000 || row_arena>0xf6800 || row!=row_arena ||
       sparse<=row || sparse>=row_bridge || (sparse&3) ||
       row_bridge<=row || (row_bridge&3) || row_end<=row_bridge || row_end>row_arena+2048)return -1;
#endif

    extern void np_mdec_chroma_select(void),np_mdec_chroma_end(void);
    extern char np_video_csr_records[];
    const uintptr_t chroma=(uintptr_t)np_mdec_chroma_select;
    const uintptr_t chroma_end=(uintptr_t)np_mdec_chroma_end;
    const uintptr_t chroma_arena=(uintptr_t)np_video_csr_records;
    uintptr_t chroma_floor=chroma_arena;
#if NP_GPU_DMA_RASTER_SPLIT
    extern char np_dma_raster_cost[];
    chroma_floor=(uintptr_t)np_dma_raster_cost+4;
#endif
    if((chroma&15) || chroma_arena<0xf0000 || chroma_arena>0xf6800 ||
       chroma<chroma_floor || chroma_end<=chroma || chroma_end>chroma_arena+2048)return -1;

    extern void np_mdec_rle_general(void),np_mdec_rle_general_end(void);
    const uintptr_t rle=(uintptr_t)np_mdec_rle_general;
    const uintptr_t rle_end=(uintptr_t)np_mdec_rle_general_end;
    if((rle&3) || rle<(uintptr_t)np_mdec_fast_end || rle_end<=rle || rle_end>0xf4cd0)return -1;


    extern void np_mdec_color_fast(void),np_mdec_color_end(void);
    extern char np_video_command_records[];
    const uintptr_t color=(uintptr_t)np_mdec_color_fast;
    const uintptr_t color_end=(uintptr_t)np_mdec_color_end;
    const uintptr_t color_arena=(uintptr_t)np_video_command_records;
    if((color&3) || color_arena<0xf0000 || color_arena>0xf6800 || color<color_arena || color_end<=color || color_end>color_arena+2048)return -1;


    /* The full POPS hash/layout is checked before this transactional plan.
     * Guard the displaced prologues and clear call. Keep helpers in the unused
     * trace-ring reservation. Native DMA completion and decoder state remain
     * untouched; each helper falls back for inputs it cannot simplify. */
    const uintptr_t begin=(uintptr_t)np_mdec_fast_begin;
    const uintptr_t end=(uintptr_t)np_mdec_fast_end;
    const uintptr_t hooks[4]={(uintptr_t)np_mdec_dc,(uintptr_t)np_mdec_flat,
        (uintptr_t)np_mdec_rle_dc,(uintptr_t)np_mdec_clear};
    const uint32_t sites[4]={0x20d3b8,0x20d9f8,0x20d2a8,0x20d2cc};
    const uint32_t expected[4][4]={
        {0x27bdfeb0,0xffb00100,0x03a0582d,0xffb10108},
        {0x27bdffd0,0x3c02005a,0xffb00000,0x0080802d},
        {0x27bdffe0,0xffb20010,0x00a0902d,0xffb10008},
        {0x0c0827bc,0x00c0802d,0x0220102d,0x26310002}};
    if(begin<0xf3cd0 || end<=begin || end>0xf4cd0)return -1;
    for(unsigned i=0;i<4;++i) {
        if(hooks[i]<begin || hooks[i]>=end || (hooks[i]&3))return -1;
        const uint32_t replacement[4]={(i==3?0x0c000000u:0x08000000u)|((uint32_t)hooks[i]>>2),
            i==3?expected[i][1]:0,expected[i][2],expected[i][3]};
        /* The native padding correction owns the words after the clear call. */
        const size_t length=i==3?8:sizeof(replacement);
        if(np_patch_add(&patch_set,sites[i],expected[i],replacement,length))return -1;
    }
    return 0;
}
#endif
extern void np_gte_orgb(void),np_gte_orgb_end(void);
extern void np_gte_irgb(void),np_gte_irgb_end(void);
extern void np_gte_zero_depth(void),np_gte_zero_depth_end(void);
static int install_gte_zero_depth_adapter(void) {
    const uint32_t expected[7]={0x016c102a,0x54400005,0x3c0b0002,
        0x15600002,0x01ab001b,0x000001cd,0x00005812};
    const uintptr_t hook=(uintptr_t)np_gte_zero_depth;
    const uintptr_t end=(uintptr_t)np_gte_zero_depth_end;
    if(hook<(uintptr_t)np_gte_irgb_end || end<=hook || end>0xf7900 || (hook&3))return -1;
    uint32_t replacement[7];memcpy(replacement,expected,sizeof(expected));
    replacement[3]=0x08000000u|((uint32_t)hook>>2);
    replacement[4]=replacement[5]=replacement[6]=0;
    return np_patch_add(&patch_set,0x208a70,expected,replacement,sizeof(expected))?-1:0;
}
static int install_gte_color_adapter(void) {
    /* The pinned ORGB helper shifts to five bits, then PPAC5 discards three
     * more bits. Validate its complete body transactionally before redirecting
     * generated calls to a correctly scaled, saturating RGB555 conversion. */
    static const uint32_t expected[16]={
        0x9f8401d4,0x9f8501d8,0x9f8601dc,0x000421c2,
        0x000529c2,0x000631c2,0x3084001f,0x30a5001f,
        0x30c6001f,0x70a41688,0x70c21588,0x700217c8,
        0xa7820224,0x03e00008,0,0};
    const uintptr_t hook=(uintptr_t)np_gte_orgb,end=(uintptr_t)np_gte_orgb_end;
    if(hook<0xf7800 || end<=hook || end>0xf8000 || (hook&3))return -1;
    uint32_t replacement[16];memcpy(replacement,expected,sizeof(expected));
    replacement[0]=0x08000000u|((uint32_t)hook>>2);replacement[1]=0;
    for(unsigned i=0;i<2;++i)
        if(np_patch_add(&patch_set,0x2084a0+i*32,expected+i*8,replacement+i*8,32))return -1;
    /* The matching input helper expands to byte colors with PEXT5, leaving
     * IR1..IR3 sixteen times too small. Guard its complete native body too. */
    static const uint32_t irgb_expected[12]={
        0x8f820220,0x70021788,0x304400ff,0x00022a02,
        0x00023402,0x30a500ff,0x30c600ff,0xaf8401d4,
        0xaf8501d8,0xaf8601dc,0x03e00008,0};
    const uintptr_t input=(uintptr_t)np_gte_irgb,input_end=(uintptr_t)np_gte_irgb_end;
    if(input<end || input_end<=input || input_end>0xf8000 || (input&3))return -1;
    uint32_t irgb_replacement[12];memcpy(irgb_replacement,irgb_expected,sizeof(irgb_expected));
    irgb_replacement[0]=0x08000000u|((uint32_t)input>>2);irgb_replacement[1]=0;
    if(np_patch_add(&patch_set,0x2084e0,irgb_expected,irgb_replacement,32) ||
       np_patch_add(&patch_set,0x208500,irgb_expected+8,irgb_replacement+8,16))return -1;
    return 0;
}
#if NP_GPU_DMA_RASTER_SPLIT
#if NP_GPU_DMA_RASTER_SPLIT != 1
#error "GPU DMA raster split accepts a boolean build flag only"
#endif
extern void np_dma_fill_cost(void),np_dma_sprite_cost(void),np_dma_polygon_cost(void);
extern void np_dma_line_cost(void),np_dma_copy_cost(void);
extern void np_dma_reset_linked(void),np_dma_reset_block(void),np_dma_harvest_cost(void);
static int install_gpu_dma_raster_split(const NpElf *elf) {
    const uint32_t sites[8]={0x21d598,0x21eb34,0x21f9e4,0x220654,
        0x225f14,0x21ae40,0x21ae68,0x21aa68};
    const uint32_t expected[8][3]={{0x00822021,0xaca42260,0},
        {0x00431021,0xac822260,0},{0x00431021,0xac822260,0},
        {0x00431021,0xac822260,0},{0x00431021,0xac822260,0},
        {0xac402260,0xac54226c,0},{0xac602260,0xac60226c,0},
        {0xaca3226c,0x03e00008,0x42000038}};
    const uintptr_t hooks[8]={(uintptr_t)np_dma_fill_cost,(uintptr_t)np_dma_sprite_cost,
        (uintptr_t)np_dma_polygon_cost,(uintptr_t)np_dma_line_cost,(uintptr_t)np_dma_copy_cost,
        (uintptr_t)np_dma_reset_linked,(uintptr_t)np_dma_reset_block,(uintptr_t)np_dma_harvest_cost};
    /* Validate the whole revision-pinned plan before adding any replacement.
     * Native mode/quota delay slots survive both counter-reset redirects. */
    for(unsigned j=0;j<8;++j) {
        const uint32_t length=j==7?12:8;
        int found=0;
        if(hooks[j]<0xf0000 || hooks[j]>=0xf7000 || (hooks[j]&3))return -1;
        for(unsigned i=0;i<elf->count;++i) {
            const NpSegment *s=&elf->segments[i];
            if(s->address<=sites[j] && sites[j]+length<=s->address+s->filesz) {
                const void *words=(const void *)(NP_BOOT_STAGE+s->offset+sites[j]-s->address);
                if(memcmp(words,expected[j],length))return -1;
                found=1;break;
            }
        }
        if(!found)return -1;
    }
    for(unsigned j=0;j<8;++j) {
        uint32_t replacement[3]={0x08000000u|((uint32_t)hooks[j]>>2),
            j==5 || j==6?expected[j][1]:0,0};
        if(np_patch_add(&patch_set,sites[j],expected[j],replacement,j==7?12:8))return -1;
    }
    return 0;
}
#endif
#if NP_SPU_CAPTURE_IRQ
extern void np_spu_capture_irq(void),np_spu_capture_reset(void),np_spu_capture_end(void);
static int install_spu_capture_adapter(void) {
    /* Full firmware identity is checked first. Guard the complete native
     * cursor update transactionally, including the untouched mask/store. */
    const uint32_t expected[4]={0x8c62026c,0x005e1021,0x304201ff,0xac62026c};
    const uintptr_t hook=(uintptr_t)np_spu_capture_irq,end=(uintptr_t)np_spu_capture_end;
    if(hook<0xf0000 || end<=hook || end>0xf7000 || (hook&3))return -1;
    const uint32_t replacement[4]={0x08000000u|((uint32_t)hook>>2),0,
        expected[2],expected[3]};
    if(np_patch_add(&patch_set,0x211464,expected,replacement,sizeof(expected)))return -1;
    const uintptr_t reset=(uintptr_t)np_spu_capture_reset;
    if(reset<hook || reset>=end || (reset&3))return -1;
    const uint32_t reset_expected[4]={0x0000202d,0xaf808828,0xaf808824,0x0080282d};
    const uint32_t reset_replacement[4]={0x08000000u|((uint32_t)reset>>2),0,
        reset_expected[2],reset_expected[3]};
    return np_patch_add(&patch_set,0x200dd4,reset_expected,reset_replacement,sizeof(reset_expected)) ? -1 : 0;
}
#endif
#if NP_SPU_REVERB_READONLY
extern void np_spu_reverb_start(void),np_spu_reverb_end(void);
extern void np_spu_reverb_call(void),np_spu_reverb_tail(void),np_spu_reverb_context_end(void);
extern void np_spu_reverb_lsame(void),np_spu_reverb_rsame(void);
extern void np_spu_reverb_ldiff(void),np_spu_reverb_rdiff(void);
extern void np_spu_reverb_lapf1(void),np_spu_reverb_rapf1(void);
extern void np_spu_reverb_lapf2(void),np_spu_reverb_rapf2(void);
static int install_spu_reverb_write_guards(void) {
    const uint32_t sites[8]={0x212648,0x212734,0x212850,0x212914,
        0x212bf8,0x212c90,0x212d38,0x212e30};
    const uint32_t expected[8][4]={
        {0xa5480000,0x0004bbc3,0x8f240a44,0x8fa5002c},
        {0xa52e0000,0x25290002,0x0189282b,0x8fae002c},
        {0xa6860000,0x26940002,0x8f2f09ec,0x87030000},
        {0xa5470000,0x254a0002,0x8fac0044,0x8fa2005c},
        {0xa5870000,0x24073def,0x8f2f0a20,0x0143c021},
        {0xa5ee0000,0x25ef0002,0xafaf009c,0x240f3e4c},
        {0xa5ee0000,0x25ef0002,0xafaf00a8,0x0003f00a},
        {0xa4cc0000,0x24c60002,0x000b5bc3,0x00a6202b}};
    const uintptr_t hooks[8]={(uintptr_t)np_spu_reverb_lsame,(uintptr_t)np_spu_reverb_rsame,
        (uintptr_t)np_spu_reverb_ldiff,(uintptr_t)np_spu_reverb_rdiff,
        (uintptr_t)np_spu_reverb_lapf1,(uintptr_t)np_spu_reverb_rapf1,
        (uintptr_t)np_spu_reverb_lapf2,(uintptr_t)np_spu_reverb_rapf2};
    const uintptr_t start=(uintptr_t)np_spu_reverb_start,end=(uintptr_t)np_spu_reverb_end;
    if(start<0xf0000 || end<=start || end>0xf7000)return -1;
    for(unsigned i=0;i<8;++i) {
        if(hooks[i]<start || hooks[i]>=end || (hooks[i]&3))return -1;
        const uint32_t replacement[4]={0x08000000u|((uint32_t)hooks[i]>>2),0,
            expected[i][2],expected[i][3]};
        if(np_patch_add(&patch_set,sites[i],expected[i],replacement,sizeof(replacement)))return -1;
    }
    /* Full firmware identity pins the leaf's unused at register and its
     * only caller. Preserve both its ordinary return and IRQ tail return. */
    extern char np_reverb_exact_start[],np_reverb_exact_end[];
    extern void np_reverb_exact32(void),np_reverb_recompile(void),np_reverb_recompile_end(void);
    const uintptr_t exact=(uintptr_t)np_reverb_exact32;
    const uintptr_t exact_start=(uintptr_t)np_reverb_exact_start;
    const uintptr_t exact_end=(uintptr_t)np_reverb_exact_end;
    const uintptr_t bridge=(uintptr_t)np_reverb_recompile;
    const uintptr_t bridge_end=(uintptr_t)np_reverb_recompile_end;
    if(exact_start<0xf8000 || exact_end<=exact_start || exact_end>0xff000 ||
       exact<exact_start || exact>=exact_end || bridge<exact_start ||
       bridge_end<=bridge || bridge_end>exact_end || ((exact|bridge)&3))return -1;
    const uintptr_t call=(uintptr_t)np_spu_reverb_call;
    const uintptr_t tail=(uintptr_t)np_spu_reverb_tail;
    const uintptr_t context_end=(uintptr_t)np_spu_reverb_context_end;
    if(call<0xf7900 || tail<=call || context_end<=tail || context_end>0xf8000 ||
       (call&3) || (tail&3))return -1;
    const uint32_t call_expected[2]={0x0c0848e8,0x24500350};
    const uint32_t tail_expected[2]={0x080842ce,0x27bd0130};
    const uint32_t call_replacement[2]={0x0c000000u|((uint32_t)call>>2),call_expected[1]};
    const uint32_t tail_replacement[2]={0x08000000u|((uint32_t)tail>>2),tail_expected[1]};
    return np_patch_add(&patch_set,0x21133c,call_expected,call_replacement,8) ||
           np_patch_add(&patch_set,0x212ffc,tail_expected,tail_replacement,8)?-1:0;
}
#endif
#if NP_AUDIO_GAP_DECLICK
extern void np_audio_gap_declick(void);
/* The native output callback copies/zero-fills 251 input stereo frames here,
 * then resamples them. Change only its temporary PCM gap edges, before
 * resampling. The firmware, input ring, voice state, clocks and DMA stay native. */
static int install_audio_gap_adapter(const NpElf *elf) {
    const uint32_t expected[2]={0x0c08516c,0x24060100};
    const uintptr_t hook=(uintptr_t)np_audio_gap_declick;
    if(hook<0xf0000 || hook>=0xf7000 || (hook&3))return -1;
    for(unsigned i=0;i<elf->count;++i) {
        const NpSegment *s=&elf->segments[i];
        if(s->address<=0x213ff4 && 0x213ffc<=s->address+s->filesz) {
            const void *words=(const void *)(NP_BOOT_STAGE+s->offset+0x213ff4-s->address);
            if(memcmp(words,expected,sizeof(expected)))return -1;
            return queue_word(0x213ff4,expected[0],0x0c000000u|((uint32_t)hook>>2));
        }
    }
    return -1;
}
#endif
#if NP_GP0_PACKET_COALESCE || NP_GP0_PACKET_TRACE || NP_GP0_SCALAR_WRITE
extern void np_gp0_publish(void),np_gp0_read_barrier(void),np_gp0_status_barrier(void);
extern void np_gp0_control_barrier(void),np_gp0_video_barrier(void);
#if NP_GP0_SCALAR_WRITE
extern void np_gp0_scalar_write(void);
#endif
/* Only the pinned CPU single-word caller can defer a publication. GPU reads,
 * GP1/reset and VBlank flush before executing their original native paths.
 * The complete firmware identity/layout is checked before planning these words. */
static int install_gp0_packet_adapter(const NpElf *elf) {
#if NP_GP0_SCALAR_WRITE && !NP_MEMORY_ACCESS_TRACE
    extern void np_gp0_upload_copy(void),np_gp0_upload_copy_end(void);
    extern char np_video_worker_records[];
    const uintptr_t upload=(uintptr_t)np_gp0_upload_copy,upload_end=(uintptr_t)np_gp0_upload_copy_end;
    const uintptr_t upload_arena=(uintptr_t)np_video_worker_records;
    if(upload_arena<0xf0000 || upload_arena>0xf6800 || (upload&15) ||
       upload<upload_arena || upload_end<=upload || upload_end>upload_arena+2048)return -1;
    /* Pin the s3==4 admission, bounded word count and displaced copy branch.
     * The helper returns before the original atomic FIFO consumption. */
    const uint32_t upload_expected[6]={0x1663000b,0x02a2380b,0x8fa40000,
        0x3082000f,0x14400008,0x0080182d};
    int upload_found=0;
    for(unsigned i=0;i<elf->count;++i) {
        const NpSegment *segment=&elf->segments[i];
        if(segment->address<=0x224338 && 0x224350<=segment->address+segment->filesz) {
            const void *words=(const void *)(NP_BOOT_STAGE+segment->offset+0x224338-segment->address);
            if(memcmp(words,upload_expected,sizeof(upload_expected)))return -1;
            upload_found=1;break;
        }
    }
    const uint32_t upload_replacement[2]={0x08000000u|((uint32_t)upload>>2),0};
    if(!upload_found || np_patch_add(&patch_set,0x224344,upload_expected+3,upload_replacement,8))return -1;
#endif

    const uint32_t sites[5]={0x21a974,0x20b4b0,0x20b4c0,0x21a568,0x21aeb0};
    const uint32_t expected[5][2]={{0x0c08ba1c,0x8e042b14},
        {0x0c0869a6,0},{0x0c086b0c,0},{0x27bdffe0,0xffb10008},{0x27bdfff0,0x3c027000}};
    const uintptr_t hooks[5]={(uintptr_t)np_gp0_publish,
        (uintptr_t)np_gp0_read_barrier,(uintptr_t)np_gp0_status_barrier,
        (uintptr_t)np_gp0_control_barrier,(uintptr_t)np_gp0_video_barrier};
    for(unsigned j=0;j<5;++j) {
        int found=0;
        if(hooks[j]<0xf0000 || hooks[j]>=0xf7000 || (hooks[j]&3))return -1;
        for(unsigned i=0;i<elf->count;++i) {
            const NpSegment *s=&elf->segments[i];
            if(s->address<=sites[j] && sites[j]+8<=s->address+s->filesz) {
                const uint32_t *words=(const void *)(NP_BOOT_STAGE+s->offset+sites[j]-s->address);
                if(memcmp(words,expected[j],sizeof(expected[j])))return -1;
                found=1;break;
            }
        }
        if(!found)return -1;
    }
#if NP_GP0_SCALAR_WRITE
    const uintptr_t scalar=(uintptr_t)np_gp0_scalar_write;
    int found=0;
    if(scalar<0xf0000 || scalar>=0xf7000 || (scalar&3))return -1;
    for(unsigned i=0;i<elf->count;++i) {
        const NpSegment *s=&elf->segments[i];
        if(s->address<=0x20b508 && 0x20b510<=s->address+s->filesz) {
            const uint32_t *words=(const void *)(NP_BOOT_STAGE+s->offset+0x20b508-s->address);
            if(words[0]!=0x0c086a02 || words[1]!=0x24050001)return -1;
            found=1;break;
        }
    }
    if(!found || queue_word(0x20b508,0x0c086a02,0x0c000000u|((uint32_t)scalar>>2)))return -1;
#endif
    for(unsigned j=0;j<5;++j) {
        if(queue_word(sites[j],expected[j][0],(j==1 || j==2?0x0c000000u:0x08000000u)|
                      ((uint32_t)hooks[j]>>2)))return -1;
        if(j>=3 && queue_word(sites[j]+4,expected[j][1],0))return -1;
    }
    return 0;
}
#endif
#if NP_VIDEO_TRANSITION_FIX && !NP_MEMORY_ACCESS_ADAPTER
#error "The video transition candidate requires the memory adapter candidate"
#endif
#if NP_RGB24_SCANOUT_ADAPTER
/* Use the pinned POPS native RGB24 converter instead of direct PS24 scanout.
 * Dispatch, buffer selection and incremental updates are scoped to RGB24;
 * the native mode transition restores ordinary scanout. */
static int install_rgb24_scanout_adapter(const NpElf *elf) {
    static const uint32_t sites[5]={0x222280,0x221ce4,0x21d284,0x224c00,0x225eb8};
    static const uint32_t expected[5][2]={
        {0x8e0222a0,0x30420002},{0x3c020001,0x00621024},
        {0x0c088996,0x2408ffff},{0x0c088996,0x30a501ff},
        {0x0c088996,0x8fa70040}};
    const uintptr_t hooks[5]={(uintptr_t)np_rgb24_buffered,(uintptr_t)np_rgb24_dispatch,
        (uintptr_t)np_rgb24_update,(uintptr_t)np_rgb24_update,(uintptr_t)np_rgb24_update};
    for(unsigned j=0;j<5;++j)
        if(hooks[j]<0xf0000 || hooks[j]>=0xf7000 || (hooks[j]&3))return -1;
    /* Validate the complete plan first; update calls retain their native
     * argument-building delay slots and link register conventions. */
    for(unsigned j=0;j<5;++j) {
        int found=0;
        for(unsigned i=0;i<elf->count;++i) {
            const NpSegment *s=&elf->segments[i];
            if(s->address<=sites[j] && sites[j]+8<=s->address+s->filesz) {
                const uint32_t *words=(const void *)(NP_BOOT_STAGE+s->offset+sites[j]-s->address);
                if(words[0]!=expected[j][0] || words[1]!=expected[j][1])return -1;
                found=1;break;
            }
        }
        if(!found)return -1;
    }
    for(unsigned j=0;j<5;++j) {
        uint32_t replacement[2]={(j<2?0x08000000u:0x0c000000u)|((uint32_t)hooks[j]>>2),
            j<2?0:expected[j][1]};
        if(np_patch_add(&patch_set,sites[j],expected[j],replacement,8))return -1;
    }
    return 0;
}
#endif
extern uint32_t np_disc_probe_enabled;
extern uint32_t np_firmware_license_enabled;
extern uint32_t np_asset_trace_enabled;
extern uint32_t np_native_request_enabled, np_trace_enabled;
extern void np_save_open_hook(void),np_save_write_hook(void),np_save_close_hook(void);
extern void np_save_rename_hook(void),np_save_sync_hook(void),np_save_wait_hook(void);
extern void np_save_sema_hook(void),np_save_thread_hook(void),np_save_start_hook(void);
static int queue_word(uint32_t address, uint32_t expected, uint32_t replacement) {
    return np_patch_add(&patch_set,address,&expected,&replacement,4)?-1:0;
}
extern void np_cpu_loop_cycle_boundary(void),np_cpu_loop_cycle_boundary_end(void);
static int install_cpu_loop_cycle_boundary(void) {
    const uint32_t expected[4]={0x0000202d,0xaf9e883c,0x0c081b2a,0xaee06b80};
    const uintptr_t hook=(uintptr_t)np_cpu_loop_cycle_boundary;
    const uintptr_t end=(uintptr_t)np_cpu_loop_cycle_boundary_end;
    if(hook<0xf0000 || end<=hook || end>0xf7000 || (hook&3))return -1;
    const uint32_t replacement[4]={expected[0],expected[1],
        0x0c000000u|((uint32_t)hook>>2),expected[3]};
    return np_patch_add(&patch_set,0x202788,expected,replacement,sizeof(expected))?-1:0;
}
/* Experimental shared adaptation of the checksum-pinned POPS scalar emitter.
 * Its three-word memory/NOOP reservation is finalized before generated code
 * can run. The original epilogue delay slot is retained. */
#if NP_MEMORY_ACCESS_ADAPTER
static int install_memory_access_adapter(const NpElf *elf) {
    const uint32_t sites[3]={0x205d4c,0x206358,0x205d34};
    const uint32_t expected[3][2]={{0x0180102d,0xdfb10018},{0x0220102d,0xdfb00030},
        {0xdfb00010,0xad800000}};
    const uintptr_t hooks[2]={(uintptr_t)np_memory_emit_finalize,(uintptr_t)np_memory_group_finalize};
    for(unsigned j=0;j<3;++j) {
        int found=0;
        if(j<2 && (hooks[j]<0xf0000 || hooks[j]>=0xf7000 || (hooks[j]&3)))return -1;
        for(unsigned i=0;i<elf->count;++i) {
            const NpSegment *s=&elf->segments[i];
            if(s->address<=sites[j] && sites[j]+8<=s->address+s->filesz) {
                const uint32_t *words=(const void *)(NP_BOOT_STAGE+s->offset+sites[j]-s->address);
                if(words[0]!=expected[j][0] || words[1]!=expected[j][1])return -1;
                found=1;break;
            }
        }
        if(!found)return -1;
    }
    for(unsigned j=0;j<3;++j)
        if(queue_word(sites[j],expected[j][0],j==2?0x0200302d:
                      0x0c000000u|((uint32_t)hooks[j]>>2)))return -1;
    return 0;
}
#endif
#if NP_VIDEO_TRANSITION_FIX
/* Select the applied interlace state at the pinned wait's existing test.
 * Validate the original branch-likely delay load even though it stays intact. */
static int install_video_transition_fix(const NpElf *elf) {
    const uint32_t site=0x21aedc;
    const uint32_t expected[3]={0x8e022214,0x5040000a,0xdfb00000};
    const uintptr_t hook=(uintptr_t)np_video_interlace_applied;
    int found=0;
    if(hook<0xf0000 || hook>=0xf7000 || (hook&3))return -1;
    for(unsigned i=0;i<elf->count;++i) {
        const NpSegment *s=&elf->segments[i];
        if(s->address<=site && site+12<=s->address+s->filesz) {
            const uint32_t *words=(const void *)(NP_BOOT_STAGE+s->offset+site-s->address);
            if(memcmp(words,expected,sizeof(expected)))return -1;
            found=1;break;
        }
    }
    if(!found)return -1;
    return queue_word(site,expected[0],0x08000000u|((uint32_t)hook>>2)) ||
           queue_word(site+4,expected[1],0)?-1:0;
}
#endif
extern void np_clut_refresh_start(void),np_clut_refresh_end(void);
extern void np_clut_refresh_polygon(void),np_clut_refresh_sprite(void);
static int install_clut_refresh(void) {
    const uintptr_t start=(uintptr_t)np_clut_refresh_start;
    const uintptr_t end=(uintptr_t)np_clut_refresh_end;
    const uintptr_t hooks[2]={(uintptr_t)np_clut_refresh_polygon,
        (uintptr_t)np_clut_refresh_sprite};
    const uint32_t sites[2]={0x21ec10,0x21fb04};
    const uint32_t expected[2][8]={
        {0xfc450000,0x24420008,0x24430020,0x24040007,
         0xae230000,0xfc460008,0xfc470010,0xfc440018},
        {0x0285a024,0x24050006,0x24460030,0x0003183e,
         0x8e092658,0x24070007,0xad460000,0x24080045}};
    if(start<0xf7d40 || end<=start || end>0xf7fc0 || (start&3))return -1;
    for(unsigned i=0;i<2;++i) {
        if(hooks[i]<start || hooks[i]>=end || (hooks[i]&3))return -1;
        uint32_t replacement[8];
        memcpy(replacement,expected[i],sizeof(replacement));
        replacement[2]=0x08000000u|((uint32_t)hooks[i]>>2);
        replacement[3]=0;
        if(np_patch_add(&patch_set,sites[i],expected[i],replacement,sizeof(replacement)))return -1;
    }
    return 0;
}
extern void np_cd_read_status(void),np_cd_read_status_end(void);
static int install_cd_read_status_fix(void) {
    const uintptr_t hook=(uintptr_t)np_cd_read_status;
    const uintptr_t end=(uintptr_t)np_cd_read_status_end;
    if(hook<0xf7fc0 || end<=hook || end>0xf8000 || (hook&3))return -1;
    /* All three sync words were accepted before this boundary. Keep their
     * mismatch branches and the following sector-header publication intact. */
    const uint32_t expected[8]={0x3c0200ff,0x8e430008,0x3442ffff,0x54620055,
        0xdfb00000,0x8a42000f,0x9a42000c,0xaa020023};
    uint32_t replacement[8];
    memcpy(replacement,expected,sizeof(expected));
    replacement[5]=0x08000000u|((uint32_t)hook>>2);
    replacement[6]=0;
    return np_patch_add(&patch_set,0x20e8ec,expected,replacement,sizeof(expected))?-1:0;
}
#if NP_MEMORY_ACCESS_TRACE && NP_MEMORY_ACCESS_ADAPTER
static int install_memory_wait_trace(const NpElf *elf) {
    const uint32_t sites[6]={0x21a8a0,0x21a9c8,0x21ab98,0x21adb8,0x21aec8,0x21c650};
    const uint32_t delays[6]={0x8e042b18,0x8e042b18,0x8e042b18,0x8e042b18,0x8e042b1c,0x8e242b14};
    const uintptr_t hooks[6]={(uintptr_t)np_memory_wait_writer,(uintptr_t)np_memory_wait_writer_tail,
        (uintptr_t)np_memory_wait_dma,(uintptr_t)np_memory_wait_reader,
        (uintptr_t)np_memory_wait_video,(uintptr_t)np_memory_wait_worker};
    for(unsigned j=0;j<6;++j) {
        int found=0;
        if(hooks[j]<0xf0000 || hooks[j]>=0xf7000 || (hooks[j]&3))return -1;
        for(unsigned i=0;i<elf->count;++i) {
            const NpSegment *s=&elf->segments[i];
            if(s->address<=sites[j] && sites[j]+8<=s->address+s->filesz) {
                const uint32_t *words=(const void *)(NP_BOOT_STAGE+s->offset+sites[j]-s->address);
                if(words[0]!=0x0c08ba18 || words[1]!=delays[j])return -1;
                found=1;break;
            }
        }
        if(!found)return -1;
    }
    for(unsigned j=0;j<6;++j)
        if(queue_word(sites[j],0x0c08ba18,0x0c000000u|((uint32_t)hooks[j]>>2)))return -1;
    return 0;
}
/* The video waiter has one pinned direct caller. Observe its entry, exact
 * loop comparison and common exit without changing either branch target. */
static int install_video_progress_trace(const NpElf *elf) {
    const uint32_t sites[8]={0x21aec0,0x21af00,0x21af0c,0x21cd8c,0x221728,0x221c34,0x2217e0,0x23ad48};
    const uint32_t expected[8][2]={{0x245010f0,0x00000000},
        {0x1083fff1,0x00000000},{0x3c048000,0xdfbf0008},
        {0xdc620000,0x000214f8},{0x00121e02,0xffb00000},{0x10000004,0xac822268},
        {0x08087298,0x27bd0030},{0x0808b908,0x27bd0050}};
    const uintptr_t hooks[8]={(uintptr_t)np_video_trace_entry,
        (uintptr_t)np_video_trace_decision,(uintptr_t)np_video_trace_exit,
        (uintptr_t)np_video_trace_csr,(uintptr_t)np_video_trace_command,(uintptr_t)np_video_trace_latch,
        (uintptr_t)np_video_trace_reset,(uintptr_t)np_video_trace_crt};
    for(unsigned j=0;j<8;++j) {
        int found=0;
        if(hooks[j]<0xf0000 || hooks[j]>=0xf7000 || (hooks[j]&3))return -1;
        for(unsigned i=0;i<elf->count;++i) {
            const NpSegment *s=&elf->segments[i];
            if(s->address<=sites[j] && sites[j]+8<=s->address+s->filesz) {
                const uint32_t *words=(const void *)(NP_BOOT_STAGE+s->offset+sites[j]-s->address);
                if(words[0]!=expected[j][0] || words[1]!=expected[j][1])return -1;
                found=1;break;
            }
        }
        if(!found)return -1;
    }
    for(unsigned j=0;j<8;++j) {
        const uint32_t opcode=(j==0?0x0c000000u:0x08000000u)|((uint32_t)hooks[j]>>2);
        if(queue_word(sites[j],expected[j][0],opcode))return -1;
        /* Replay the CSR load and shift together: the redirected delay slot
         * must not consume v0 before the original hardware read. */
        if(j==3 && queue_word(sites[j]+4,expected[j][1],0))return -1;
    }
    return 0;
}
#endif
static int install_hooks(const NpElf *elf,int game,int minimal,int assets) {
    const uint32_t sites[24]={0x214968,0x2149a0,0x200790,0x2007b0,0x200828,0x200834,0x200868,0x200870,0x200888,0x2008a0,0x2008dc,0x200910,0x210068,0x2101b4,
        0x20e880,0x20eae4,0x20ea04,0x20eca4,0x20ef14,0x20f0e4,0x20ff14,0x20e9a8,0x20e550,0x21a918};
    const uint32_t targets[24]={0x233650,0x233650,0x210540,0x20cf00,0x22d2e0,0x22d480,0x22dc30,0x214418,0x20c750,0x200940,0x200d10,0x200d50,0x230330,0x230330,
        0x210368,0x210368,0x210290,0x210290,0x210290,0x210290,0x22e850,0x209e00,0x209e00,0x209e00};
    const uintptr_t hooks[24]={(uintptr_t)np_mount_hook,(uintptr_t)np_mount_hook,(uintptr_t)np_disc_hook,(uintptr_t)np_card_hook,
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
/* Reapply the queued native readback descriptor after the two manual FIFO
 * writes, before DMA starts, for the shared RGB24 conversion path.
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
static int adapt_disc_checks(const NpElf *elf,int tekken,const unsigned char *selected_header,int file_launch) {
    static const unsigned char header_sha1[20]={0xb2,0x9b,0xb8,0x80,0xc5,0x51,0x96,0xcd,0xd8,0xd4,0x5b,0x81,0xa3,0xea,0x29,0xf2,0x32,0x19,0x6a,0x18};
    static const unsigned char tekken_header_sha1[20]={0x5f,0xa2,0xa7,0x1e,0xc1,0xa0,0x08,0x2d,0xaf,0x4d,0x5a,0xcf,0x94,0x3d,0x13,0x11,0xbe,0xed,0x46,0x5b};
    for(unsigned i=0;i<elf->count;++i) {
        const NpSegment *s=&elf->segments[i];
        if(s->address<=0x2fe670 && 0x2fe670+96<=s->address+s->filesz) {
            uint32_t *table=(void *)(NP_BOOT_STAGE+s->offset+0x2fe670-s->address);
            if(table[0]!=2400 || table[1]!=64 || table[3]!=1 || table[8]!=38 || table[9]!=0 || table[10]!=35)return -1;
            /* Job offset/count use 512-byte sectors. Native prepared HDD
             * requests use 256 KiB; direct files check just their 2 KiB header. */
            if(table[17]!=0 || table[18]!=512)return -1;
            if(file_launch && queue_word(0x2fe670+72,table[18],NP_FILE_HEADER_HASH_BYTES/512))return -1;
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
/* EE serial output survives FILEIO/RPC replacement. */
static void trace(const char *format,...) {
    char text[256];va_list args;va_start(args,format);
    int n=vsnprintf(text,sizeof(text),format,args);va_end(args);
    if(n<0)return;
    if(n>=(int)sizeof(text))n=(int)sizeof(text)-1;
    if(!strncmp(text,"PSXCORE rejected=",17)) {
        char *end=strchr(text,'\n');if(end)*end=0;
        np_startup_error(text+17);
        if(end)*end='\n';
    }
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
int main(int argc,char **argv) {
    int file_launch=argc>1;
    SifInitRpc(0);np_startup_init();
    np_startup_begin(1,"Initialize EE/RPC services");np_startup_done();
    np_startup_begin(2,"Parse launch arguments and request");
    if((unsigned)GetMemorySize()<NP_BOOT_STACK){np_startup_error("ee-memory-size");goto rejected;}
    NpFileLaunchRequest file_request;
    char mode[32]={0};size_t got;
    if(file_launch) {
        const char *error=np_file_prepare(argc,argv,&file_request);
        if(error){trace("PSXCORE rejected=%s\n",error);goto rejected;}
        memcpy(mode,"launch\n",8);got=7;
    } else {
        FILE *config=fopen("host:/psxcore-runtime-mode.txt","rb");
        if(!config){np_startup_error("missing-launch-request: no arguments; HostFS mode unavailable");goto rejected;}
        got=fread(mode,1,sizeof(mode)-1,config);int extra=fgetc(config),bad_read=ferror(config),closed=fclose(config);
        if(closed || bad_read || extra!=EOF || !np_runtime_mode_valid(mode,got)) {
            trace("PSXCORE rejected=runtime-mode\n");goto rejected;
        }
    }
    int native=!strcmp(mode,"launch\n");
    NpLaunchRequest request;
    NpCompatResult resolved;
    if(native) {
        if(file_launch) {
            const char *reason=np_file_launch_check(&file_request,sizeof(file_request),&resolved);
            if(!reason)reason=np_file_launch_native(&file_request,&request);
            if(reason){trace("PSXCORE rejected=%s\n",reason);goto rejected;}
        } else {
            FILE *f=fopen("host:/psxcore-launch.bin","rb");
            if(!f){trace("PSXCORE rejected=launch-file\n");goto rejected;}
            size_t n=fread(&request,1,sizeof(request),f);
            int trailing=fgetc(f),io_error=ferror(f),close_error=fclose(f);
            const char *reason=np_launch_check(&request,n,&resolved);
            if(reason || trailing!=EOF || io_error || close_error) {
                trace("PSXCORE rejected=%s\n",reason?reason:"launch-file-size-or-io");goto rejected;
            }
        }
        np_native_request_enabled=1;
        np_trace_enabled=request.options.diagnostics;
        trace("PSXCORE launch=v%u profile=%s revision=%u fixes=%x environment=%u diagnostics=%u\n",
            file_launch?NP_FILE_VERSION:NP_LAUNCH_VERSION,resolved.profile,(unsigned)resolved.profile_revision,(unsigned)resolved.fixes,
            (unsigned)request.environment,(unsigned)request.options.diagnostics);
        trace("PSXCORE identity=%s game=%s disc-revision=%s\n",
            resolved.game_name?"matched":"unmatched",
            resolved.game_name?resolved.game_name:"unidentified",
            resolved.disc_revision?resolved.disc_revision:"unmatched");
        trace("PSXCORE saves=sony-shared card0=%s card1=%s\n",
            file_launch?file_request.cards[0]:"hdd0:__common:/ps1emu/card0",
            file_launch?file_request.cards[1]:"hdd0:__common:/ps1emu/card1");
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
        repair=0;
    }
#if NP_RGB24_SCANOUT_ADAPTER
    /* Native RGB24 conversion requires the checked readback adapter. Install
     * that prerequisite with the core feature,
     * independently of game profiles and optional fix selections. */
    if(native)repair=1;
#endif
    np_startup_begin(7,"Read and verify POPS.ELF");
    if(file_launch?np_file_read_asset(&file_request,"POPS.ELF",(void *)NP_BOOT_STAGE,3166988,elf_hash):
        read_known("host:/POPS.ELF",(void *)NP_BOOT_STAGE,3166988,elf_hash)) {
        trace("PSXCORE rejected=runtime-file-identity\n");goto rejected;
    }
    np_startup_done();np_startup_begin(8,"Read and verify IOPRP252.IMG");
    if(file_launch?np_file_read_asset(&file_request,"IOPRP252.IMG",(void *)NP_BOOT_IOP,265233,img_hash):
        read_known("host:/IOPRP252.IMG",(void *)NP_BOOT_IOP,265233,img_hash)) {
        trace("PSXCORE rejected=iop-file-identity\n");goto rejected;
    }
    np_startup_done();np_startup_begin(9,"Validate POPS layout and checked adapters");
    NpElf elf;const char *error=np_boot_elf_check((void *)NP_BOOT_STAGE,3166988,&elf);
    if(error || profile(&elf)) {
        trace("PSXCORE rejected=%s\n",error?error:"runtime-layout-profile");goto rejected;
    }
    if(file_launch && np_file_storage_plan(file_request.runtime,&patch_set)) {
        trace("PSXCORE rejected=file-storage-patch-plan\n");goto rejected;
    }
    error=np_runtime_correct(NP_RUNTIME_KNOWN,&patch_set);
    if(error){trace("PSXCORE rejected=%s\n",error);goto rejected;}
    if(native && install_clut_refresh()) {
        trace("PSXCORE rejected=clut-refresh-patch-plan\n");goto rejected;
    }
    if(native)trace("PSXCORE clut-refresh=1 formats=indexed paths=polygon,sprite scope=shared-runtime\n");
    if(install_gte_color_adapter()) {
        trace("PSXCORE rejected=gte-color-patch-plan\n");goto rejected;
    }
    if(install_gte_zero_depth_adapter()) {
        trace("PSXCORE rejected=gte-zero-depth-patch-plan\n");goto rejected;
    }
    error=np_gte_native_three_plan(&patch_set);
    if(error){trace("PSXCORE rejected=%s\n",error);goto rejected;}
    /* Native/file launches use the per-game compatibility selection.
     * Legacy diagnostic modes retain the original exact-check behavior. */
    const int code_validation=!native || (resolved.fixes & NP_FIX_CODE_VALIDATION);
    if(code_validation) {
        if(install_code_guard(&elf)) {
            trace("PSXCORE rejected=code-validation-patch-plan\n");goto rejected;
        }
        trace("PSXCORE code-validation=decoded-instructions option=on invalidation=native\n");
    } else {
        trace("PSXCORE code-validation=native-only option=off\n");
    }
    if(native) {
        const NpGamePatchContext game={&request,resolved.fixes,
            (const void *)NP_BOOT_STAGE,3166988,&elf};
        unsigned applied=0;
        error=np_game_patches_plan(&game,&patch_set,&applied);
        if(error){trace("PSXCORE rejected=%s\n",error);goto rejected;}
        if(applied)trace("PSXCORE game-patches=automatic count=%u\n",applied);
    }
#if NP_POPS_BLOCK_LIMIT
    /* Decoder-window selection is independent of Code Validation. */
    if(queue_word(0x502750,0x100,NP_POPS_BLOCK_LIMIT)) {
        trace("PSXCORE rejected=block-limit-patch-plan\n");goto rejected;
    }
#endif
#if NP_GPU_DMA_RASTER_SPLIT
    if(native) {
        if(install_gpu_dma_raster_split(&elf)) {
            trace("PSXCORE rejected=gpu-dma-raster-patch-plan\n");goto rejected;
        }
        trace("PSXCORE gpu-dma-raster-split=1 scope=shared native-render-budget=preserved experimental=yes\n");
    }
#endif
#if NP_SPU_CAPTURE_IRQ
    if(native) {
        if(install_spu_capture_adapter()) {
            trace("PSXCORE rejected=spu-capture-patch-plan\n");goto rejected;
        }
        trace("PSXCORE spu-capture-irq=1 scope=shared dispatch=native\n");
    }
#endif
#if NP_SPU_REVERB_READONLY
    if(native) {
        if(install_spu_reverb_write_guards()) {
            trace("PSXCORE rejected=spu-reverb-patch-plan\n");goto rejected;
        }
        trace("PSXCORE spu-reverb-readonly=1 scope=shared filtering=native\n");
    }
#endif
#if NP_RASTER_COST_FASTPATH
    if(native) {
        if(install_raster_cost_fastpath()) {
            trace("PSXCORE rejected=raster-cost-patch-plan\n");goto rejected;
        }
        trace("PSXCORE raster-cost-fastpath=1 arithmetic=exact fallback=native\n");
    }
#endif
#if NP_AUDIO_GAP_DECLICK
    if(native) {
        if(install_audio_gap_adapter(&elf)) {
            trace("PSXCORE rejected=audio-gap-patch-plan\n");goto rejected;
        }
        trace("PSXCORE audio-gap-declick=1 scope=temporary-pcm experimental=yes\n");
    }
#endif
#if NP_GP0_PACKET_COALESCE || NP_GP0_PACKET_TRACE || NP_GP0_SCALAR_WRITE
    if(native) {
        if(install_gp0_packet_adapter(&elf)) {
            trace("PSXCORE rejected=gp0-packet-patch-plan\n");goto rejected;
        }
        trace("PSXCORE gp0-packet-coalesce=%u scalar-write=%u trace=%u experimental=yes\n",
              (unsigned)NP_GP0_PACKET_COALESCE,(unsigned)NP_GP0_SCALAR_WRITE,
              (unsigned)NP_GP0_PACKET_TRACE);
    }
#endif
#if NP_GP1_IDLE_CONTROL && !NP_MEMORY_ACCESS_TRACE
    if(native) {
        if(install_gp1_idle_control(&elf)) {
            trace("PSXCORE rejected=gp1-idle-control-patch-plan\n");goto rejected;
        }
        trace("PSXCORE gp1-idle-control=1 ownership=parked-worker scope=shared-runtime\n");
    }
#endif
    if(native) {
        if(install_cd_read_status_fix()) {
            trace("PSXCORE rejected=cd-read-status-patch-plan\n");goto rejected;
        }
        trace("PSXCORE cd-read-status-fix=1 seek-completion=verified-sector scope=shared-runtime\n");
        if(install_cpu_loop_cycle_boundary()) {
            trace("PSXCORE rejected=cpu-loop-cycle-patch-plan\n");goto rejected;
        }
        trace("PSXCORE cpu-loop-cycle-boundary=1 scope=shared-compiler\n");
    }
#if NP_MEMORY_ACCESS_ADAPTER
    if(native) {
        if(install_memory_access_adapter(&elf)) {
            trace("PSXCORE rejected=memory-access-patch-plan\n");goto rejected;
        }
        trace("PSXCORE memory-access-adapter=1 native-io=explicit scope=runtime activation=automatic\n");
#if NP_VIDEO_TRANSITION_FIX
        if(install_video_transition_fix(&elf)) {
            trace("PSXCORE rejected=video-transition-patch-plan\n");goto rejected;
        }
        trace("PSXCORE video-transition-fix=1 state=applied-interlace pending=guarded scope=runtime activation=automatic\n");
#endif
#if NP_MEMORY_ACCESS_TRACE
        if(install_memory_wait_trace(&elf)) {
            trace("PSXCORE rejected=memory-wait-trace-plan\n");goto rejected;
        }
        if(install_video_progress_trace(&elf)) {
            trace("PSXCORE rejected=video-progress-trace-plan\n");goto rejected;
        }
        trace("PSXCORE memory-wait-trace=1 scope=guest calls=6 experimental=yes\n");
#endif
    }
#endif
#if NP_RGB24_SCANOUT_ADAPTER
    if(native && repair) {
        if(install_rgb24_scanout_adapter(&elf)) {
            trace("PSXCORE rejected=rgb24-scanout-patch-plan\n");goto rejected;
        }
        trace("PSXCORE rgb24-scanout-adapter=1 scanout=buffered transport=native scope=runtime activation=automatic\n");
#if NP_INTERLACE_BUFFERED_PRESENT
        if(install_interlace_presentation_adapter()) {
            trace("PSXCORE rejected=interlace-presentation-plan\n");goto rejected;
        }
        trace("PSXCORE interlace-presentation=buffered-both-fields tuple=ntsc-ps15-480 experimental=yes scope=runtime\n");
#endif
    }
#endif
    if(native) {
        error=np_hdtv_plan(request.runtime,resolved.fixes,
            (void *)NP_BOOT_STAGE,3166988,&elf,&patch_set,
            (uintptr_t)np_hdtv_begin,(uintptr_t)np_hdtv_display,
            (uintptr_t)np_hdtv_crt,(uintptr_t)np_hdtv_height,(uintptr_t)np_hdtv_end);
        if(error){trace("PSXCORE rejected=%s\n",error);goto rejected;}
        trace("PSXCORE hdtv-fix=%s output=region-native rendering=unchanged hardware=unverified\n",
            (resolved.fixes&NP_FIX_HDTV)?"on":"off");
    }
    if(hooks && install_hooks(&elf,game,minimal,assets)) {
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
    if(adapt && adapt_disc_checks(&elf,tekken,native?request.header_sha1:NULL,file_launch)) {
        trace("PSXCORE rejected=disc-check-patch-plan\n");goto rejected;
    }
    if(license && adapt_firmware_license(&elf)) {
        trace("PSXCORE rejected=license-patch-plan\n");goto rejected;
    }
    if(native) {
        error=np_compat_add_patches(request.runtime,request.environment,resolved.fixes,&patch_set);
        if(error){trace("PSXCORE rejected=%s\n",error);goto rejected;}
    }
#if NP_MDEC_BLOCK_FASTPATH
    if(native) {
        if(install_mdec_block_fastpaths()) {
            trace("PSXCORE rejected=mdec-block-patch-plan\n");goto rejected;
        }
        trace("PSXCORE mdec-block-fastpath=1 transform=exact scheduling=native\n");
    }
#endif
#if NP_PCM_BUFFER_RATE
    if(native && (resolved.fixes & NP_FIX_ADAPTIVE_AUDIO)) {
        if(install_pcm_buffer_rate()) {
            trace("PSXCORE rejected=pcm-buffer-rate-patch-plan\n");goto rejected;
        }
        trace("PSXCORE adaptive-audio=on output-only=1 minimum-step=3387 nominal-step=3763\n");
    }
#endif
    if(native && !(resolved.fixes & NP_FIX_ADAPTIVE_AUDIO))
        trace("PSXCORE adaptive-audio=off resampler=native\n");
#if !NP_PCM_BUFFER_RATE
    if(native && (resolved.fixes & NP_FIX_ADAPTIVE_AUDIO)) {
        trace("PSXCORE rejected=adaptive-audio-not-built\n");goto rejected;
    }
#endif
#if NP_IGR_OBSERVE
    /* Plan after regular adapters; optional observers reject context overlap. */
    if(native) {
        const uintptr_t targets[4]={(uintptr_t)np_igr_pad_copy_observe,(uintptr_t)np_igr_alarm_observe,
            (uintptr_t)np_igr_serial_observe,(uintptr_t)np_igr_event_observe};
        error=np_runtime_igr_observe_plan((void *)NP_BOOT_STAGE,3166988,&elf,
            (uintptr_t)np_igr_observe_begin,(uintptr_t)np_igr_observe_end,targets,&patch_set);
        if(error){trace("PSXCORE rejected=%s\n",error);goto rejected;}
        trace("PSXCORE igr-observe=1 state=%08x bytes=%u input=unchanged saves=native return=%s\n",
            (unsigned)(uintptr_t)&np_igr_observation,(unsigned)sizeof(np_igr_observation),
            NP_IGR_RETURN?"native-menu":"disabled");
        trace("PSXCORE igr-boundary-observe=1 state=%08x bytes=%u pause=unproven return=%s\n",
            (unsigned)(uintptr_t)&np_igr_boundary_observation,(unsigned)sizeof(np_igr_boundary_observation),
            NP_IGR_RETURN?"native-menu":"disabled");
    }
#endif
#if NP_IGR_QUIT_PROBE
    if(!native){error="igr-quit-native";goto rejected;}
    error=np_runtime_igr_quit_plan((void *)NP_BOOT_STAGE,3166988,&elf,
        (uintptr_t)np_igr_quit_begin,(uintptr_t)np_igr_quit_end,
        (uintptr_t)np_igr_quit_probe,&patch_set);
    if(error){trace("PSXCORE rejected=%s\n",error);goto rejected;}
    trace("PSXCORE igr-quit=probe state=%08x bytes=%u native-quit=deferred pause=unproven return=disabled\n",
        (unsigned)(uintptr_t)&np_igr_quit_observation,(unsigned)sizeof(np_igr_quit_observation));
#endif
#if NP_CD_OBSERVE
    if(native) {
        const uintptr_t targets[8]={(uintptr_t)np_cd_read_rpc,(uintptr_t)np_cd_prefetch_rpc,
            (uintptr_t)np_cd_prefetch_callback,(uintptr_t)np_cd_response_retire,
            (uintptr_t)np_cd_getstat_retire,(uintptr_t)np_cd_deferred_callback,
            (uintptr_t)np_cd_ack_irq,(uintptr_t)np_cd_worker_read};
        error=np_runtime_cd_observe_plan((void *)NP_BOOT_STAGE,3166988,&elf,
            (uintptr_t)np_cd_observe_begin,(uintptr_t)np_cd_observe_end,targets,&patch_set);
        if(error){trace("PSXCORE rejected=%s\n",error);goto rejected;}
        trace("PSXCORE cd-observe=1 state=%08x bytes=%u barrier=unproven swap=disabled\n",
            (unsigned)(uintptr_t)&np_cd_observation,(unsigned)sizeof(np_cd_observation));
    }
#endif
#if NP_CD_RENDEZVOUS
    if(native) {
        error=np_runtime_cd_rendezvous_plan((void *)NP_BOOT_STAGE,3166988,&elf,
            (uintptr_t)np_cd_rendezvous_begin,(uintptr_t)np_cd_rendezvous_end,
            (uintptr_t)np_cd_rendezvous_tail,&patch_set);
        if(error){trace("PSXCORE rejected=%s\n",error);goto rejected;}
        trace("PSXCORE cd-rendezvous=1 state=%08x bytes=%u gate=absent swap=disabled\n",
            (unsigned)(uintptr_t)&np_cd_rendezvous_observation,
            (unsigned)sizeof(np_cd_rendezvous_observation));
    }
#endif
#if NP_IGR_WAIT
    if(!native){error="igr-wait-native";goto rejected;}
    error=np_runtime_igr_wait_check((void *)NP_BOOT_STAGE,3166988,&elf,
        (uintptr_t)np_igr_wait_begin,(uintptr_t)np_igr_wait_end,&patch_set);
    if(error){trace("PSXCORE rejected=%s\n",error);goto rejected;}
#if NP_IGR_PROBE
    error=np_runtime_igr_probe_stack_check((uintptr_t)np_igr_probe_stack,
        (uintptr_t)np_igr_probe_stack_end);
    if(error){trace("PSXCORE rejected=%s\n",error);goto rejected;}
    const uintptr_t completion_hooks[2]={(uintptr_t)np_igr_probe_rpc_complete,
        (uintptr_t)np_igr_probe_file_complete};
    error=np_runtime_igr_completion_plan((void *)NP_BOOT_STAGE,3166988,&elf,
        (uintptr_t)np_igr_wait_begin,(uintptr_t)np_igr_wait_end,completion_hooks,&patch_set);
    if(error){trace("PSXCORE rejected=%s\n",error);goto rejected;}
    trace("PSXCORE igr-wait=probe-one-shot state=%08x bytes=%u delay=256 units=native pause=unavailable return=disabled\n",
        (unsigned)(uintptr_t)&np_igr_probe_observation,(unsigned)sizeof(np_igr_probe_observation));
#else
    trace("PSXCORE igr-wait=available calls=none pause=unavailable return=disabled\n");
#endif
#endif
#if NP_IGR_RETURN
    if(native) {
        if(!size_np_igr_loader_blob || size_np_igr_loader_blob>NP_IGR_LOADER_END-NP_IGR_LOADER_ENTRY ||
           NP_IGR_LOADER_STATUS<NP_IGR_LOADER_ENTRY || NP_IGR_LOADER_STATUS+24>NP_IGR_LOADER_END) {
            error="igr-return-loader-layout";goto rejected;
        }
        error=np_runtime_igr_return_plan((void *)NP_BOOT_STAGE,3166988,&elf,
            NP_IGR_LOADER_ENTRY,NP_IGR_LOADER_END,&patch_set);
        if(error) {
            trace("PSXCORE rejected=%s\n", error);
            goto rejected;
        }
        error=np_runtime_igr_menu_plan((void *)NP_BOOT_STAGE,3166988,&elf,
            NP_IGR_MENU_DRAW,NP_IGR_LOADER_END,&patch_set);
        if(error) {
            trace("PSXCORE rejected=%s\n", error);
            goto rejected;
        }
        trace("PSXCORE igr-return=enabled trigger=native-menu saves=user-owned status=%08x bytes=24\n",
            NP_IGR_LOADER_STATUS);
    }
#endif
#if NP_CD_SWAP
    if(native && np_file_disc_set_selected()) {
        error=np_runtime_cd_swap_plan((void *)NP_BOOT_STAGE,3166988,&elf,
            (uintptr_t)np_cd_swap_context_boot,(uintptr_t)np_cd_swap_runtime_tail,
            (uintptr_t)np_cd_swap_runtime_code_end,(uintptr_t)np_cd_swap_runtime_end,
            (uintptr_t)np_cd_swap_context,(uintptr_t)np_cd_swap_context_end,
            (uintptr_t)&np_cd_swap_runtime,(uintptr_t)&np_cd_reopen,(uintptr_t)np_cd_swap_input,
            (uintptr_t)np_cd_swap_begin,(uintptr_t)np_cd_swap_end,(uintptr_t)&np_cd_swap,
            (uintptr_t)np_cd_swap_tocs,&patch_set);
        if(error) {
            trace("PSXCORE rejected=%s\n", error);
            goto rejected;
        }
        trace("PSXCORE disc-swap=native shortcut=L2+R2+Select+direction\n");
    }
#endif
#if NP_GPU_PACKET_FASTPATH
    if(native) {
        error=np_polygon_packet_plan((const void *)NP_BOOT_STAGE,3166988,&elf,&patch_set,
            (uintptr_t)np_packet_fast,(uintptr_t)np_packet_fast_template,
            (uintptr_t)np_packet_fast_end);
        if(error){trace("PSXCORE rejected=%s\n",error);goto rejected;}
        trace("PSXCORE polygon-packet-fastpath=1 packet=exact fallback=native\n");
    }
#endif
#if NP_CD_COMMAND_ORDER
    if(native) {
        const uintptr_t begin=(uintptr_t)np_cd_order_begin,end=(uintptr_t)np_cd_order_end;
        const uintptr_t command=(uintptr_t)np_cd_order_command,ack=(uintptr_t)np_cd_order_after_ack;
        if(begin!=0xec800 || end<=begin || end>0xf0000 ||
           command<begin || command>=end || ack<begin || ack>=end ||
           ((begin|end|command|ack)&3)) {
            error="cd-order-layout";goto rejected;
        }
        if(queue_word(0x4f9f24,0x20fa30,(uint32_t)command) ||
           queue_word(0x20fb18,0x0808398e,0x08000000u|((uint32_t)ack>>2)) ||
           queue_word(0x20fb4c,0x0808398e,0x08000000u|((uint32_t)ack>>2))) {
            error="cd-order-plan";goto rejected;
        }
        trace("PSXCORE cd-command-order=1 admission=after-irq-ack queue=8\n");
    }
#endif
    error=np_patch_apply((void *)NP_BOOT_STAGE,3166988,&elf,&patch_set);
    if(error){trace("PSXCORE rejected=%s\n",error);goto rejected;}
    trace("PSXCORE patches=verified count=%u capacity=%u\n",patch_set.count,NP_PATCH_LIMIT);
    trace("PSXCORE gte-orgb=rgb555 saturation=0..31 irgb=rgb555-to-ir128 scope=runtime\n");
    trace("PSXCORE gte-intpl=signed-ir sf=1 lm=0 scope=runtime\n");
    trace("PSXCORE core-corrections=%u texture-window=repeat scope=runtime\n",
        (unsigned)NP_RUNTIME_CORRECTION_REVISION);
    np_firmware_license_enabled=license;
    np_asset_trace_enabled=assets;
    np_disc_probe_enabled=file_launch && np_file_probe_only?1:verify?2:probe;
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
    if(repair)trace("PSXCORE compatibility=native-readback-descriptor restore-before-start=yes file=unmodified\n");
    fflush(stdout);
    if(image) {
        np_startup_done();np_startup_begin(10,"Reset IOP using verified POPS image");
        if(file_launch){np_startup_operation("Close storage service before POPS reset");np_file_storage_exit();np_startup_done();}
        np_startup_operation("Reset IOP using verified POPS image");
        trace("PSXCORE iop=image-reset-begin\n");
        int reset=SifIopRebootBuffer((void *)NP_BOOT_IOP,265233);
        trace("PSXCORE iop=image-reset-submitted result=%d\n",reset);
        if(reset!=1){np_startup_context("SifIopRebootBuffer result=%d",reset);np_startup_error("pops-iop-reset-submit");goto failed;}
        np_startup_done();np_startup_operation("Synchronize POPS IOP reset");
        unsigned i;for(i=0;i<300 && !SifIopSync();++i)usleep(10000);
        if(i==300){np_startup_error("pops-iop-reset-sync-timeout");goto failed;}
        np_startup_done();np_startup_operation("Reinitialize RPC after POPS reset");
        SifInitRpc(0);
        np_startup_done();np_startup_operation("Enable module loading after POPS reset");
        trace("PSXCORE iop=image-synchronized\n");
        int patched=sbv_patch_enable_lmb();
        trace("PSXCORE iop=lmb-patch result=%d\n",patched);
        if(patched<0){np_startup_context("sbv_patch_enable_lmb result=%d",patched);np_startup_error("pops-lmb-patch");goto failed;}
        np_startup_done();
    }
    trace("PSXCORE iop=service-probe-begin\n");
    np_startup_operation("Initialize POPS loadfile service");
    int load=SifLoadFileInit();
    np_startup_context("SifLoadFileInit result=%d",load);
    if(load<0){np_startup_error("pops-loadfile-init");goto failed;}
    np_startup_done();np_startup_operation("Initialize POPS IOP heap service");
    int heap=SifInitIopHeap();
    np_startup_context("SifInitIopHeap result=%d",heap);
    if(heap<0){np_startup_error("pops-iop-heap-init");goto failed;}
    np_startup_done();np_startup_operation("Probe POPS IOP heap allocation");
    void *allocation=heap>=0?SifAllocIopHeap(64):NULL;
    trace("PSXCORE iop=services loadfile=%d heap=%d allocation=%s\n",load,heap,allocation?"passed":"FAILED");
    if(!allocation){np_startup_error("pops-iop-heap-allocation");goto failed;}
    SifFreeIopHeap(allocation);
    np_startup_done();
    if(file_launch) {
        np_startup_begin(11,"Reload selected storage after POPS reset");
        if(np_file_storage_init_pops()){trace("PSXCORE rejected=file-storage-post-reset\n");goto failed;}
        trace("PSXCORE file-storage=reopen-begin\n");
        np_startup_operation("Rediscover VCD/save volumes and recheck cards");
        error=np_file_reopen(&file_request);
        if(!error){
            np_startup_done();np_startup_begin(12,"Start file bridge and check save readiness");
            trace("PSXCORE file-storage=reopen-complete bridge-begin\n");
            error=np_file_bridge_start(&file_request);
        }
        if(error){trace("PSXCORE rejected=%s\n",error);goto failed;}
        trace("PSXCORE file-storage=ready native-hdd-modules=skipped disc-reader=original\n");
    }
#if NP_IGR_RETURN
    /* Install only after the complete POPS plan and storage preparation pass.
     * This disjoint low region survives every validated POPS PT_LOAD range. */
    if(native) {
        memcpy((void *)NP_IGR_LOADER_ENTRY,np_igr_loader_blob,size_np_igr_loader_blob);
        memset((void *)(NP_IGR_LOADER_ENTRY+size_np_igr_loader_blob),0,
            NP_IGR_LOADER_END-NP_IGR_LOADER_ENTRY-size_np_igr_loader_blob);
    }
#endif
    trace("PSXCORE runtime=transfer entry=00200008 argc=1 source=external-file-unmodified\n");
    np_startup_done();np_startup_begin(13,"Transfer to POPS attempted; game boot unconfirmed");
    fflush(stdout);TerminateLibrary();np_runtime_transfer(plan);
rejected:
    trace(file_launch?"PSXCORE runtime=rejected before-pops-transfer=yes\n":"PSXCORE runtime=rejected before-iop-change=yes\n");goto wait;
failed:
    trace("PSXCORE runtime=preparation-failed\n");
wait:
    np_startup_stop("runtime-preparation-failed");
    for(;;)nopdelay();
    return 0;
}
