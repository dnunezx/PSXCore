/* Native POPS reopen transport; reuses the existing RPC and sector semaphore. */
#if NP_CD_SWAP
#include "cdreopen.h"
#define CODE __attribute__((section("._psxcore_cd_reopen.text")))
#if NP_CD_SWAP
#define NP_CD_TOC_CODE CODE
#include "psxcore_cdtoc.h"
#endif
#define DATA __attribute__((section("._psxcore_cd_reopen.data"),aligned(64)))
#define WORD(a) (*(volatile uint32_t *)(uintptr_t)(a))
#define HALF(a) (*(volatile uint16_t *)(uintptr_t)(a))
#define BYTE(a) (*(volatile uint8_t *)(uintptr_t)(a))
NpCdReopen np_cd_reopen DATA={0};
static char path[128] DATA="pfs0:/disc/disc0";
static int32_t reply[16] DATA={0};
static uint8_t toc[1024] DATA={0};
typedef int (*Rpc)(void *,int,int,const void *,int,void *,int,void *,void *);
static CODE int rpc(int command,const void *send,int sent,void *receive,int received) {
    return ((Rpc)(uintptr_t)0x230330u)((void *)0x5a91c0u,command,0,send,sent,receive,received,0,0);
}
#if NP_CD_SWAP
static CODE int reopen(unsigned slot,const unsigned char *expected,uint32_t sectors) {
    uint32_t last=0,tracks=0;
    if(slot>3 || (expected && np_cd_toc_geometry(expected,1024,sectors,&last,&tracks)))return -1;
#else
CODE int np_cd_reopen_same_disc(void) {
#endif
    uint32_t status;
    __asm__ volatile("mfc0 %0,$12":"=r"(status));
    if((status&0x30006u)!=0x30000u || !(HALF(0x59c200u)&0x1000u) ||
       WORD(0x502f14u)!=1 || !WORD(0x5a91e4u) || np_cd_reopen.active)return -1;
    if(WORD(0x502ea0u) || WORD(0x502898u)!=UINT32_MAX)return 0;
    int sema=(int)WORD(0x502f0cu);
    if(sema<=0)return -1;
    if(((int (*)(int))(uintptr_t)0x22e870u)(sema)<0)return 0;
    /* Recheck after taking the existing sector/prefetch serialization lease.
     * Main-thread exclusion remains a caller prerequisite, not a sampled flag. */
    int result=0;
    if(WORD(0x502898u)!=UINT32_MAX || WORD(0x502ea0u))goto release;
#if NP_CD_SWAP
    path[sizeof("pfs0:/disc/disc0")-2]=(char)('0'+slot);
#endif
    np_cd_reopen.active=1;
    if(np_cd_reopen.attempts!=UINT32_MAX)++np_cd_reopen.attempts;
    WORD(0x59f680u)=0;WORD(0x59f684u)=0;
    for(unsigned i=0;i<16;++i)reply[i]=0;
    result=-2;
    if(rpc(0x435052,reply,16,0,0)<0)goto finished;
    reply[0]=-1;
    result=-3;
    if(rpc(0x435053,path,128,reply,16)<0 || reply[0]!=1024)goto finished;
    for(unsigned i=0;i<1024;++i)toc[i]=0;
    result=-4;
    if(rpc(0x435055,0,0,toc,1024)<0)goto finished;
    /* Expected TOC is prevalidated resident caller data for replacement;
     * same-disc calls retain the already installed native TOC. */
    result=-5;
    for(unsigned i=0;i<1024;++i)if(toc[i]!=
#if NP_CD_SWAP
        (expected?expected[i]:BYTE(0x5a8dc0u+i))
#else
        BYTE(0x5a8dc0u+i)
#endif
    )goto finished;
    result=1;
finished:
    np_cd_reopen.active=0;
    np_cd_reopen.result=result;
release:
    if(((int (*)(int))(uintptr_t)0x22e840u)(sema)<0) {
        np_cd_reopen.result=-6;return -1;
    }
    if(result==1) {
#if NP_CD_SWAP
        /* Publish validated replacement geometry only after RPC and lease
         * release succeed. No PS1/event producer can run until caller returns. */
        if(expected) {
            for(unsigned i=0;i<1024;++i)BYTE(0x5a8dc0u+i)=toc[i];
            WORD(0x50289cu)=last;
        }
#endif
        HALF(0x59c200u)=0x0200u;
        if(np_cd_reopen.successes!=UINT32_MAX)++np_cd_reopen.successes;
        return 1;
    }
    return result? -1:0;
}
#if NP_CD_SWAP
CODE int np_cd_reopen_same_disc(void) {return reopen(0,0,0);}
CODE int np_cd_reopen_target(unsigned slot,const unsigned char expected[1024],uint32_t sectors) {
    if(!expected)return -1;
    return reopen(slot,expected,sectors);
}
#endif
#endif
