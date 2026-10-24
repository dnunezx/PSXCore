/* Original standalone ATA/USB FAT32/exFAT backend. Frontend-independent. */
#include "file_storage.h"
#include "startup_diag.h"
#include "psxcore_hash.h"
#include "psxcore_boot.h"
#include <limits.h>
#ifdef NP_HOST_TEST
#include "file_storage_stub.h"
#else
#include <kernel.h>
#include <iopcontrol.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <sifrpc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ee_regs.h>
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>
#include <usbhdfsd-common.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* Startup diagnostics must survive the POPS IOP reset. stdio may still use
 * the discarded FILEIO server before the replacement modules are ready. */
static void storage_trace(const char *format,...) {
    va_list args;va_start(args,format);
#ifdef NP_HOST_TEST
    vprintf(format,args);
#else
    char text[256];
    int n=vsnprintf(text,sizeof(text),format,args);
    if(n>=(int)sizeof(text))n=(int)sizeof(text)-1;
    for(int i=0;i<n;++i)*R_EE_SIO_TXFIFO=(unsigned char)text[i];
#endif
    va_end(args);
}

#define MODULE(name) extern unsigned char name##_irx[];extern unsigned int size_##name##_irx
MODULE(iomanX);MODULE(fileXio);MODULE(ps2dev9);MODULE(bdm);MODULE(bdmfs_fatfs);
MODULE(ata_bd);MODULE(usbd);MODULE(usbmass_bd);MODULE(psxcore_nativegate);MODULE(psxcore_filebridge);
MODULE(poweroff);
MODULE(psxcore_powerbutton);
MODULE(psxcore_iomanx);
MODULE(bdma_usb_fs);MODULE(bdma_usb_bd);MODULE(bdma_ata_fs);MODULE(bdma_ata_bd);MODULE(psxcore_massproxy);
static char mounted[8],save_mounted[8];
static uint32_t selected_backend=NP_FILE_BACKEND_ATA;
/* Zero inherits the game backend, preserving initial/post-reset defaults. */
static uint32_t selected_save_backend;
static int native_storage;
int np_file_probe_only;
/* The POPSLoader ATA transport reports its BDM name as "mass". The
 * bootstrap SDK ATA transport reports "ata"; keep the two sessions apart. */
static const char *storage_driver(uint32_t backend) {
    return native_storage && backend==NP_FILE_BACKEND_ATA?"mass":np_file_backend_driver(backend);
}
static int load(const char *name,const unsigned char *bytes,unsigned size,const char *args,unsigned length) {
    int result=-1;
    np_startup_operationf("Load IOP module: %s",name);
    storage_trace("PSXCORE file-module=%s phase=begin\n",name);
    int id=SifExecModuleBuffer((void *)bytes,size,length,args,&result);
    storage_trace("PSXCORE file-module=%s id=%d result=%d\n",name,id,result);
    np_startup_context("Module %s: ID=%d result=%d",name,id,result);
    if(id<0 || result!=0)np_startup_error("storage-module-load");else np_startup_done();
    return id<0 || result!=0?-1:0;
}
#define LOAD(name) do { if(load(#name,name##_irx,size_##name##_irx,NULL,0))return -1; } while(0)
static int storage_init(int reset,int pops);
int np_file_storage_init_pops(void) {
    /* Called only after the final, verified POPS IOP reset. The initial
     * storage environment is discarded and must not supply this pointer. */
    LOAD(psxcore_nativegate);
    return storage_init(0,1);
}
int np_file_storage_init(int reset) {
    return storage_init(reset,0);
}
static int storage_init(int reset,int pops) {
    native_storage=pops;
    uint32_t save_backend=selected_save_backend?selected_save_backend:selected_backend;
    if((selected_backend!=NP_FILE_BACKEND_ATA && selected_backend!=NP_FILE_BACKEND_USB) ||
       (save_backend!=NP_FILE_BACKEND_ATA && save_backend!=NP_FILE_BACKEND_USB))return -1;
    if(reset) {
        unsigned i;
        np_startup_begin(3,"Submit initial IOP reset");
        for(i=0;i<300 && !SifIopReset("",0);++i)usleep(10000);
        if(i==300){np_startup_error("storage-iop-reset-submit-timeout");return -1;}
        np_startup_done();np_startup_operation("Synchronize initial IOP reset");
        for(i=0;i<300 && !SifIopSync();++i)usleep(10000);
        if(i==300){np_startup_error("storage-iop-reset-sync-timeout");return -1;}
        np_startup_done();np_startup_operation("Reinitialize RPC after initial reset");
        SifInitRpc(0);
        np_startup_done();
    }
    if(reset) {
        np_startup_begin(4,"Enable initial IOP module loading");
        int patched=sbv_patch_enable_lmb();
        storage_trace("PSXCORE file-storage=lmb-patch result=%d\n",patched);
        np_startup_context("sbv_patch_enable_lmb result=%d",patched);
        if(patched<0){np_startup_error("storage-lmb-patch");return -1;}
        np_startup_done();
    }
    if(pops) {
        /* The POPS image reset discards the launcher's power-button module.
         * Install its CDVD interrupt chain before native storage is exposed. */
        LOAD(poweroff);
        /* Unchanged POPSLoader BDMA pairs: native mass registration first,
         * backend second. Never apply the SDK legacy-IOMAN takeover here. */
        if(selected_backend!=save_backend) {
            np_startup_error("native-storage-mixed-backend-unimplemented");return -1;
        }
        if(selected_backend==NP_FILE_BACKEND_USB){LOAD(bdma_usb_fs);LOAD(bdma_usb_bd);}
        else {LOAD(bdma_ata_fs);LOAD(bdma_ata_bd);LOAD(psxcore_powerbutton);}
        LOAD(psxcore_iomanx);LOAD(psxcore_massproxy);LOAD(fileXio);
    } else {
        LOAD(iomanX);LOAD(fileXio);LOAD(bdm);LOAD(bdmfs_fatfs);
        if(selected_backend==NP_FILE_BACKEND_ATA || save_backend==NP_FILE_BACKEND_ATA){LOAD(ps2dev9);LOAD(ata_bd);}
        if(selected_backend==NP_FILE_BACKEND_USB || save_backend==NP_FILE_BACKEND_USB){LOAD(usbd);LOAD(usbmass_bd);}
    }
    storage_trace("PSXCORE file-storage=filexio-init-begin\n");
    if(reset)np_startup_begin(5,"Initialize fileXio storage service");
    else np_startup_operation("Initialize fileXio after POPS reset");
    int initialized=fileXioInit();
    storage_trace("PSXCORE file-storage=filexio-init result=%d\n",initialized);
    np_startup_context("fileXioInit result=%d",initialized);
    if(initialized<0){np_startup_error("storage-filexio-init");return -1;}
    np_startup_done();np_startup_operation("Discover mounted storage volumes");
    for(unsigned i=0;i<50;++i) {
        for(unsigned unit=0;unit<10;++unit) {
            char root[9];snprintf(root,sizeof(root),"mass%u:/",unit);
            int fd=fileXioDopen(root);
            if(fd>=0){int e=fileXioDclose(fd);storage_trace("PSXCORE file-storage=mounted unit=%u close=%d\n",unit,e);
                np_startup_context("Mounted mass%u: close=%d",unit,e);
                if(e<0)np_startup_error("storage-volume-close");else np_startup_done();return e<0?-1:0;}
        }
        usleep(100000);
    }
    np_startup_error("storage-volume-discovery-timeout");return -1;
}
void np_file_storage_exit(void){fileXioExit();}
static int join(char *out,size_t size,const char *root,const char *relative) {
    int n=snprintf(out,size,"%s%s",root,relative);
    return n<0 || (size_t)n>=size?-1:0;
}
static int absent(int result){return result==-2 || result==-4;}
static int read_all(int fd,void *buffer,unsigned count) {
    unsigned done=0;
    while(done<count) {
        int n=fileXioRead(fd,(unsigned char *)buffer+done,count-done);
        if(n<=0 || (unsigned)n>count-done)return -1;
        done+=(unsigned)n;
    }
    return 0;
}
static int read_at(int fd,uint32_t offset,void *buffer,unsigned count) {
    return fileXioLseek64(fd,offset,SEEK_SET)!=offset || read_all(fd,buffer,count)?-1:0;
}
static uint32_t le32(const unsigned char *p){return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint32_t be32(const unsigned char *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static int iso_sector(int fd,uint64_t bytes,uint32_t lba,unsigned char data[2048]) {
    uint64_t offset=NP_VCD_DATA_OFFSET+(uint64_t)lba*2352;
    unsigned char raw[2352];
    if(offset>bytes || sizeof(raw)>bytes-offset || read_at(fd,(uint32_t)offset,raw,sizeof(raw)))return -1;
    static const unsigned char sync[12]={0,255,255,255,255,255,255,255,255,255,255,0};
    if(memcmp(raw,sync,12))return -1;
    unsigned start=raw[15]==1?16:raw[15]==2?24:0;
    if(!start || (start==24 && ((raw[18]&0x20) || memcmp(raw+16,raw+20,4))))return -1;
    memcpy(data,raw+start,2048);return 0;
}
/* Serial comes from bounded ISO9660 SYSTEM.CNF content, never the filename. */
static void serial(int fd,uint64_t bytes,char out[16]) {
    unsigned char data[2048],text[4097];out[0]=0;
    if(iso_sector(fd,bytes,16,data) || data[0]!=1 || memcmp(data+1,"CD001",5) || data[6]!=1)return;
    const unsigned char *record=data+156;
    if(record[0]<34 || !(record[25]&2) || le32(record+2)!=be32(record+6) || le32(record+10)!=be32(record+14))return;
    uint32_t extent=le32(record+2),size=le32(record+10);
    if(!size || size>65536)return;
    for(unsigned block=0;block<(size+2047)/2048;++block) {
        if(iso_sector(fd,bytes,extent+block,data))return;
        unsigned limit=size-block*2048;if(limit>2048)limit=2048;
        for(unsigned offset=0;offset<limit;) {
            record=data+offset;unsigned n=record[0];if(!n)break;
            if(n<34 || n>limit-offset || record[32]>n-33)return;
            if(record[32]==12 && !memcmp(record+33,"SYSTEM.CNF;1",12) && !(record[25]&2) &&
               le32(record+2)==be32(record+6) && le32(record+10)==be32(record+14)) {
                uint32_t cnf=le32(record+2),length=le32(record+10);
                if(!length || length>4096)return;
                for(unsigned i=0;i<(length+2047)/2048;++i) {
                    unsigned char sector[2048];if(iso_sector(fd,bytes,cnf+i,sector))return;
                    unsigned take=length-i*2048;if(take>2048)take=2048;
                    memcpy(text+i*2048,sector,take);
                }
                text[length]=0;
                for(unsigned p=0;p+4<length;++p) {
                    if(p && text[p-1]!='\n' && text[p-1]!='\r')continue;
                    while(p<length && (text[p]==' ' || text[p]=='\t'))++p;
                    if(p+4>=length || memcmp(text+p,"BOOT",4))continue;
                    p+=4;while(p<length && (text[p]==' ' || text[p]=='\t'))++p;
                    if(p>=length || text[p++]!='=')return;
                    while(p<length && (text[p]==' ' || text[p]=='\t'))++p;
                    if(p+6>length || memcmp(text+p,"cdrom:",6))return;
                    p+=6;while(p<length && (text[p]=='\\' || text[p]=='/'))++p;
                    unsigned start=p;
                    while(p<length && text[p]!=';' && text[p]!='\r' && text[p]!='\n' && text[p]!=' ') {
                        if(text[p]=='/' || text[p]=='\\')start=p+1;
                        ++p;
                    }
                    if(p-start==11) {
                        char candidate[16]={0};memcpy(candidate,text+start,11);
                        int valid=candidate[4]=='_' && candidate[8]=='.';
                        for(unsigned i=0;i<11;++i)if(i!=4 && i!=8)
                            valid&=i<4?(candidate[i]>='A'&&candidate[i]<='Z'):(candidate[i]>='0'&&candidate[i]<='9');
                        if(valid)memcpy(out,candidate,16);
                    }
                    return;
                }
                return;
            }
            offset+=n;
        }
    }
}
/* Fixed-size header and bounded ISO metadata only. No full-image hash or
 * per-game metadata sidecar is required before or after the IOP reset. */
#if NP_STARTUP_DIAGNOSTICS
static char inspect_context[137];
#endif
static const char *inspect(const char *path,NpFileLaunchRequest *r,int initial) {
    iox_stat_t before,after;unsigned char head[NP_FILE_HEADER_HASH_BYTES];
    memset(&before,0,sizeof(before));
#if NP_STARTUP_DIAGNOSTICS
    snprintf(inspect_context,sizeof(inspect_context),"%.68s",path);
#endif
    np_startup_context("%.68s",path);
    int status=fileXioGetStat(path,&before);
    if(status<0 || !FIO_S_ISREG(before.mode)) {
#if NP_STARTUP_DIAGNOSTICS
        snprintf(inspect_context,sizeof(inspect_context),"%.68s\nGetStat=%d mode=%08x",path,status,(unsigned)before.mode);
#endif
        np_startup_context("%s",inspect_context);
        return status<0?"file-image-stat":"file-image-not-regular";
    }
    uint64_t bytes=((uint64_t)before.hisize<<32)|before.size;
    if(bytes>INT32_MAX)return "file-image-size";
    if(!initial && bytes!=r->image_bytes)return "file-image-changed";
    int fd=fileXioOpen(path,FIO_O_RDONLY,0);if(fd<0)return "file-image-open";
    const char *error=NULL;
    if(read_all(fd,head,sizeof(head)))error="file-image-header-read";
    if(!error)error=np_vcd_check(head,sizeof(head),bytes,NULL);
    char driver[8]={0};uint32_t device=UINT32_MAX;
    if(!error && (fileXioIoctl2(fd,USBMASS_IOCTL_GET_DRIVERNAME,NULL,0,driver,sizeof(driver))<0 ||
        fileXioIoctl2(fd,USBMASS_IOCTL_GET_DEVICE_NUMBER,NULL,0,&device,sizeof(device))<0 ||
        !memchr(driver,0,sizeof(driver)) || strcmp(driver,storage_driver(r->disc.backend)) ||
        device!=r->disc.device))error="file-image-device";
    char actual_serial[16]={0};unsigned char actual_header[20];
    if(!error) {
        NpSha1 header;np_sha1_init(&header);np_sha1_update(&header,head,sizeof(head));np_sha1_final(&header,actual_header);
        serial(fd,bytes,actual_serial);
        if(!initial && (memcmp(actual_header,r->header_sha1,20) || strcmp(actual_serial,r->serial)))error="file-image-changed";
    }
    if(fileXioClose(fd)<0 && !error)error="file-image-close";
    if(!error && (fileXioGetStat(path,&after)<0 || after.size!=before.size || after.hisize!=before.hisize ||
        memcmp(after.mtime,before.mtime,sizeof(before.mtime))))error="file-image-changed";
    if(!error && initial) {
        r->image_bytes=bytes;memcpy(r->header_sha1,actual_header,20);memcpy(r->serial,actual_serial,16);
    }
    return error;
}
static int card(const char *path) {
    iox_stat_t st;unsigned char head[128];int e=fileXioGetStat(path,&st);
    if(e<0)return e;
    if(!FIO_S_ISREG(st.mode) || st.hisize || st.size!=NP_FILE_CARD_BYTES)return -1;
    int fd=fileXioOpen(path,FIO_O_RDONLY,0);if(fd<0)return fd;
    e=read_all(fd,head,sizeof(head));int closed=fileXioClose(fd);
    return e || closed<0 || !np_file_card_header_valid(head,sizeof(head))?-1:0;
}
static int root_valid(const char *root) {
    return root && strlen(root)==6 && !strncmp(root,"mass",4) && root[4]>='0' && root[4]<='9' && root[5]==':';
}
/* The pinned SDK supports these two metadata ioctls on directory handles:
 * FIL and DIR start with FFOBJID, and only obj.fs->pdrv is consulted. */
static const char *root_metadata(const char *root,NpFileEndpoint *endpoint) {
    if(!root_valid(root))return "file-volume-root";
    char path[9],driver[8]={0};uint32_t device=UINT32_MAX;
    snprintf(path,sizeof(path),"%s/",root);
    int fd=fileXioDopen(path);if(fd<0)return "file-volume-open";
    int e=fileXioIoctl2(fd,USBMASS_IOCTL_GET_DRIVERNAME,NULL,0,driver,sizeof(driver));
    if(e>=0)e=fileXioIoctl2(fd,USBMASS_IOCTL_GET_DEVICE_NUMBER,NULL,0,&device,sizeof(device));
    int closed=fileXioDclose(fd);
    np_startup_context("%s: driver=%.7s device=%u; wanted %s device=%u",root,driver,
        (unsigned)device,storage_driver(endpoint->backend),(unsigned)endpoint->device);
    if(e<0 || closed<0 || !memchr(driver,0,sizeof(driver)) ||
       strcmp(driver,storage_driver(endpoint->backend)) || device>1)return "file-volume-device";
    endpoint->device=device;
    return NULL;
}
static const char *root_check(const char *root,const NpFileEndpoint *endpoint) {
    NpFileEndpoint actual=*endpoint;
    const char *error=root_metadata(root,&actual);
    return error?error:actual.device!=endpoint->device?"file-volume-device":NULL;
}
static const char *volume_read(const char *root,unsigned char id[16]) {
    char path[64];iox_stat_t st;NpFileVolumeMarker marker;unsigned char extra;
    memset(id,0,16);snprintf(path,sizeof(path),"%s/POPS",root);
    int parent=fileXioGetStat(path,&st);
    if(absent(parent))return "file-volume-id-missing";
    if(parent<0 || !FIO_S_ISDIR(st.mode))return "file-volume-directory";
    if(join(path,sizeof(path),root,NP_FILE_VOLUME_MARKER))return "file-volume-path";
    int e=fileXioGetStat(path,&st);
    if(absent(e))return "file-volume-id-missing";
    if(e<0 || !FIO_S_ISREG(st.mode) || st.hisize || st.size!=sizeof(marker))return "file-volume-marker-stat";
    int fd=fileXioOpen(path,FIO_O_RDONLY,0);if(fd<0)return "file-volume-marker-open";
    e=read_all(fd,&marker,sizeof(marker));if(!e && fileXioRead(fd,&extra,1)!=0)e=-1;
    int closed=fileXioClose(fd);if(e || closed<0)return "file-volume-marker-read";
    return np_file_volume_marker_check(&marker,sizeof(marker),id);
}
static const char *volume_check(const char *root,unsigned char id[16]) {
    unsigned char actual[16];const char *error=volume_read(root,actual);
    if(error) {
        if(!strcmp(error,"file-volume-id-missing") && !np_file_volume_id_present(id))return NULL;
        return error;
    }
    if(np_file_volume_id_present(id) && memcmp(id,actual,16))return "file-volume-id-mismatch";
    memcpy(id,actual,16);return NULL;
}
/* massN belongs to the frontend's previous IOP session. Resolve the initial
 * read by the explicit endpoint and optional volume ID before inspecting it.
 * Never use a different physical device merely because the filename exists. */
static const char *select_initial_once(NpFileLaunchRequest *r,char selected[NP_FILE_PATH_BYTES+8]) {
    unsigned matches=0,endpoints=0;
    NpFileLaunchRequest chosen;
    char chosen_path[NP_FILE_PATH_BYTES+8]={0};
    const char *failure=NULL;
#if NP_STARTUP_DIAGNOSTICS
    char failure_context[137]={0};
#endif
    for(unsigned i=0;i<10;++i) {
        char root[8],path[NP_FILE_PATH_BYTES+8];
        snprintf(root,sizeof(root),"mass%u:",i);
        if(root_check(root,&r->disc))continue;
        ++endpoints;
        NpFileLaunchRequest candidate=*r;
        /* A supplied ID disambiguates partitions before touching their VCDs. */
        if(np_file_volume_id_present(r->disc_volume)) {
            const char *error=volume_check(root,candidate.disc_volume);
            if(error){
                failure=error;
#if NP_STARTUP_DIAGNOSTICS
                snprintf(failure_context,sizeof(failure_context),"Volume %s did not match selected ID",root);
#endif
                continue;
            }
        }
        if(join(path,sizeof(path),root,r->disc_path))return "file-image-path";
        const char *error=inspect(path,&candidate,1);
        if(error) {
            /* Preserve a substantive failure over another absent filename. */
            if(!failure || !strcmp(failure,"file-image-stat")) {
                failure=error;
#if NP_STARTUP_DIAGNOSTICS
                memcpy(failure_context,inspect_context,sizeof(failure_context));
#endif
            }
            continue;
        }
        error=volume_check(root,candidate.disc_volume);
        if(error)return error;
        if(++matches>1)return "file-image-volume-ambiguous";
        chosen=candidate;memcpy(chosen_path,path,strlen(path)+1);
    }
    if(matches!=1) {
        if(!endpoints)return "file-image-endpoint-not-found";
#if NP_STARTUP_DIAGNOSTICS
        if(failure_context[0])np_startup_context("%s",failure_context);
#endif
        return failure?failure:"file-image-not-found-or-changed";
    }
    *r=chosen;memcpy(selected,chosen_path,strlen(chosen_path)+1);
    np_startup_context("%.68s\nResolved after storage reset",selected);
    return NULL;
}
static const char *select_initial(NpFileLaunchRequest *r,char selected[NP_FILE_PATH_BYTES+8]) {
    const char *error=NULL;
    for(unsigned attempt=0;attempt<50;++attempt) {
        error=select_initial_once(r,selected);
        if(!error || (strcmp(error,"file-image-endpoint-not-found") && strcmp(error,"file-image-stat")))return error;
        /* Discovery of any mass volume does not mean the selected one is ready.
         * This bounds retries only; synchronous filesystem calls may block. */
        if(attempt+1<50)usleep(100000);
    }
    return error;
}
static const char *cards(const NpFileLaunchRequest *r) {
    if(np_file_probe_only)return NULL;
    char path[NP_FILE_PATH_BYTES+16];iox_stat_t st;
    for(unsigned i=0;i<2;++i) {
        if(join(path,sizeof(path),save_mounted,r->cards[i]))return "file-save-path";
        int e=card(path);
        if(e)return absent(e)?"file-save-card-missing":"file-save-card-invalid";
        static const char *const suffix[]={".bak",".new"};
        for(unsigned j=0;j<2;++j) {
            snprintf(path,sizeof(path),"%s%s%s",save_mounted,r->cards[i],suffix[j]);
            if(!absent(fileXioGetStat(path,&st)))return "file-save-recovery-required";
        }
        snprintf(path,sizeof(path),"%s%s.prev",save_mounted,r->cards[i]);e=card(path);
        if(e && !absent(e))return "file-save-previous-invalid";
    }
    return NULL;
}
static const char *storage_contract(const NpFileLaunchRequest *r) {
    const char *error=np_file_launch_check(r,sizeof(*r),NULL);if(error)return error;
    const NpFileLaunchRequest *f=r;
    if((f->disc.backend!=NP_FILE_BACKEND_ATA && f->disc.backend!=NP_FILE_BACKEND_USB) ||
       (f->saves.backend!=NP_FILE_BACKEND_ATA && f->saves.backend!=NP_FILE_BACKEND_USB))return "file-storage-backend-unimplemented";
    uint32_t save_backend=selected_save_backend?selected_save_backend:selected_backend;
    if(f->disc.backend!=selected_backend || f->saves.backend!=save_backend)return "file-storage-backend-mismatch";
    if((f->disc.required|f->saves.required)&~NP_FILE_SAVE_CAPS)return "file-storage-capabilities-unavailable";
    return NULL;
}
static const char *save_discover(const NpFileLaunchRequest *r) {
    save_mounted[0]=0;
    if(!np_file_volume_id_present(r->save_volume)) {
        if(r->disc.backend!=r->saves.backend || r->disc.device!=r->saves.device)return "file-launch-volume-identities";
        memcpy(save_mounted,mounted,sizeof(save_mounted));return NULL;
    }
    unsigned matches=0;
    for(unsigned i=0;i<10;++i) {
        char root[8];unsigned char id[16];snprintf(root,sizeof(root),"mass%u:",i);
        if(root_check(root,&r->saves) ||
           volume_read(root,id) || memcmp(id,r->save_volume,16))continue;
        if(++matches>1){save_mounted[0]=0;return "file-save-volume-ambiguous";}
        memcpy(save_mounted,root,sizeof(save_mounted));
    }
    return matches==1?NULL:"file-save-volume-not-found-or-changed";
}
static const char *locate(NpFileLaunchRequest *r,const char *checked_path) {
    mounted[0]=save_mounted[0]=0;
    const char *error=storage_contract(r);if(error)return error;
    NpFileLaunchRequest *f=r;
    unsigned matches=0,identity_matches=0;unsigned char selected_id[16]={0};
    for(unsigned i=0;i<10;++i) {
        char root[8],path[NP_FILE_PATH_BYTES+8];unsigned char id[16];snprintf(root,sizeof(root),"mass%u:",i);
        if(root_check(root,&f->disc))continue;
        const char *marker_error=volume_read(root,id);
        if(np_file_volume_id_present(r->disc_volume) && (marker_error || memcmp(id,r->disc_volume,16)))continue;
        if(np_file_volume_id_present(r->disc_volume) && ++identity_matches>1){mounted[0]=0;return "file-image-volume-ambiguous";}
        if(join(path,sizeof(path),root,f->disc_path))return "file-image-path";
        /* The selected path was already inspected in this storage session.
         * Other mounts and all post-reset discovery still get checked. */
        if(!checked_path || strcmp(path,checked_path)) {
            error=inspect(path,f,0);if(error)continue;
        }
        if(marker_error && strcmp(marker_error,"file-volume-id-missing")){mounted[0]=0;return marker_error;}
        if(++matches>1){mounted[0]=0;return "file-image-volume-ambiguous";}
        memcpy(mounted,root,sizeof(mounted));memcpy(selected_id,id,16);
    }
    if(matches!=1)return "file-image-not-found-or-changed";
    /* Unbound requests acquire an existing marker on first verification. */
    if(!np_file_volume_id_present(r->disc_volume) && np_file_volume_id_present(selected_id)) {
        memcpy(r->disc_volume,selected_id,16);memcpy(r->save_volume,selected_id,16);
    }
    error=save_discover(r);
    if(error){mounted[0]=save_mounted[0]=0;return error;}
    storage_trace("PSXCORE file-image=header-checked full-scan=no root=%s saves-root=%s bytes=%u serial=%s volume-id=%s\n",mounted,save_mounted,
        (unsigned)f->image_bytes,f->serial[0]?f->serial:"unidentified",np_file_volume_id_present(r->disc_volume)?"verified":"unbound");
    return NULL;
}
#if NP_CD_SWAP
/* Binary frontend output only; legacy single-disc requests keep their ABI. */
#include "psxcore_discset.h"
#if NP_CD_SWAP
#include "cdswap.h"
#include "cdswap_runtime.h"
static const char *prepare_swap(const NpFileDiscSet *set) {
    memset(&np_cd_swap,0,sizeof(np_cd_swap));
    memset(np_cd_swap_tocs,0,sizeof(np_cd_swap_tocs));
    if(set->count<2)return "disc-swap-count";
    for(unsigned slot=0;slot<set->count;++slot) {
        const NpFileDiscEntry *entry=np_file_disc_entry(set,slot);
        char path[NP_FILE_PATH_BYTES+8];unsigned char header[NP_FILE_HEADER_HASH_BYTES];
        if(!entry || join(path,sizeof(path),mounted,entry->path))return "disc-swap-path";
        int fd=fileXioOpen(path,FIO_O_RDONLY,0);if(fd<0)return "disc-swap-open";
        int error=read_all(fd,header,sizeof(header));int closed=fileXioClose(fd);
        if(error || closed<0 || np_file_disc_entry_check(entry,header,sizeof(header)))return "disc-swap-header";
        memcpy(np_cd_swap_tocs[slot],header,1024);np_cd_swap.sectors[slot]=entry->sectors;
    }
    np_cd_swap.magic=NP_CD_SWAP_MAGIC;np_cd_swap.version=1;np_cd_swap.bytes=sizeof(np_cd_swap);
    np_cd_swap.count=set->count;np_cd_swap.ready=1;
    return NULL;
}
#endif
static char disc_set_path[NP_FILE_PATH_BYTES];
static NpFileDiscSet launch_disc_set;
#endif
/* POPStarter selects a VCD path in the newly initialized native mass stack.
 * Recheck the existing measured VCD identity, not the old USB location.
 * Exactly one match is required; saves keep their path/marker constraints. */
static const char *reopen_once(NpFileLaunchRequest *r) {
    NpFileLaunchRequest chosen;unsigned matches=0;char checked[NP_FILE_PATH_BYTES+8]={0};
    const char *failure="file-image-not-found-or-changed";
    for(unsigned unit=0;unit<10;++unit) {
        char root[8],path[NP_FILE_PATH_BYTES+8];snprintf(root,sizeof(root),"mass%u:",unit);
        NpFileLaunchRequest candidate=*r;
        if(root_metadata(root,&candidate.disc))continue;
        if(volume_check(root,candidate.disc_volume))continue;
        if(join(path,sizeof(path),root,r->disc_path))return "file-image-path";
        const char *error=inspect(path,&candidate,0);
        if(error){if(strcmp(error,"file-image-stat"))failure=error;continue;}
        if(++matches>1)return "file-image-volume-ambiguous";
        if(r->disc.backend==r->saves.backend && r->disc.device==r->saves.device &&
           !memcmp(r->disc_volume,r->save_volume,16))candidate.saves.device=candidate.disc.device;
        chosen=candidate;strcpy(checked,path);
    }
    if(matches!=1)return failure;
    if(np_file_volume_id_present(r->save_volume) &&
       memcmp(r->disc_volume,r->save_volume,16)) {
        unsigned saves=0;
        for(unsigned unit=0;unit<10;++unit) {
            char root[8];unsigned char id[16];snprintf(root,sizeof(root),"mass%u:",unit);
            NpFileEndpoint endpoint=chosen.saves;
            if(root_metadata(root,&endpoint) || volume_read(root,id) || memcmp(id,r->save_volume,16))continue;
            if(++saves>1)return "file-save-volume-ambiguous";
            chosen.saves=endpoint;
        }
        if(saves!=1)return "file-save-volume-not-found-or-changed";
    }
    const char *error=locate(&chosen,checked);
    if(!error)error=cards(&chosen);
    if(!error)*r=chosen;
    return error;
}
const char *np_file_reopen(NpFileLaunchRequest *r) {
    np_startup_operation("Open selected VCD through native mass storage");
    mounted[0]=save_mounted[0]=0;const char *error=NULL;
    for(unsigned attempt=0;attempt<50;++attempt) {
        error=reopen_once(r);
        if(!error || strcmp(error,"file-image-not-found-or-changed"))break;
        if(attempt+1<50)usleep(100000);
    }
    if(error)mounted[0]=save_mounted[0]=0;
    else {
        char path[NP_FILE_PATH_BYTES+8];snprintf(path,sizeof(path),"%s%s",mounted,r->disc_path);
        np_startup_selection(np_file_backend_driver(r->disc.backend),r->disc.device,
            np_file_backend_driver(r->saves.backend),r->saves.device,r->environment,path);
    }
    return error;
}
static uint32_t backend_number(const char *name) {
    return !strcmp(name,"ata")?NP_FILE_BACKEND_ATA:!strcmp(name,"usb")?NP_FILE_BACKEND_USB:0;
}
static int device_number(const char *number,uint32_t *out) {
    if((number[0]!='0' && number[0]!='1') || number[1])return 0;
    *out=(uint32_t)(number[0]-'0');return 1;
}
int np_file_disc_set_selected(void) {
#if NP_CD_SWAP
    return disc_set_path[0]!=0;
#else
    return 0;
#endif
}
const char *np_file_prepare(int argc,char **argv,NpFileLaunchRequest *r) {
#if NP_CD_SWAP
    disc_set_path[0]=0;
#endif
    np_file_probe_only=0;mounted[0]=save_mounted[0]=0;
    np_file_launch_init(r);selected_backend=NP_FILE_BACKEND_ATA;selected_save_backend=0;
    const char *selected=NULL,*request_path=NULL,*save_root=NULL;
    char resolved_selected[NP_FILE_PATH_BYTES+8];
    int overrides=0,backend_seen=0,device_seen=0,save_backend_seen=0,save_device_seen=0;
    int disc_id_seen=0,save_id_seen=0,card_seen[2]={0};
    for(int i=1;i<argc;++i) {
        if(!strcmp(argv[i],"--file") && i+1<argc && !selected && !request_path)selected=argv[++i];
#if NP_CD_SWAP
        else if(!strcmp(argv[i],"--disc-set") && i+1<argc && !disc_set_path[0]) {
            const char *p=argv[++i];
            if(!np_file_path_valid(p,sizeof(disc_set_path),1))return "disc-set-path";
            strcpy(disc_set_path,p);
        }
#endif
        else if(!strcmp(argv[i],"--file-request") && i+1<argc && !selected && !request_path)request_path=argv[++i];
        else if(!strcmp(argv[i],"--probe-synthetic"))np_file_probe_only=1;
        else if((!strcmp(argv[i],"--card0") || !strcmp(argv[i],"--card1")) && i+1<argc) {
            unsigned slot=argv[i][6]-'0';
            if(card_seen[slot]++)return "file-launch-arguments";
            const char *path=argv[++i];if(!np_file_card_path_valid(path))return "file-launch-card-paths";
            strcpy(r->cards[slot],path);overrides=1;
        } else if(!strcmp(argv[i],"--backend") && i+1<argc && !backend_seen) {
            backend_seen=1;selected_backend=backend_number(argv[++i]);if(!selected_backend)return "file-storage-backend-unimplemented";
        } else if(!strcmp(argv[i],"--save-backend") && i+1<argc && !save_backend_seen) {
            save_backend_seen=1;selected_save_backend=backend_number(argv[++i]);if(!selected_save_backend)return "file-storage-backend-unimplemented";
        } else if(!strcmp(argv[i],"--device") && i+1<argc && !device_seen) {
            device_seen=1;overrides=1;if(!device_number(argv[++i],&r->disc.device))return "file-endpoint-device";
        } else if(!strcmp(argv[i],"--save-device") && i+1<argc && !save_device_seen) {
            save_device_seen=1;overrides=1;if(!device_number(argv[++i],&r->saves.device))return "file-endpoint-device";
        } else if(!strcmp(argv[i],"--save-volume") && i+1<argc && !save_root) {
            save_root=argv[++i];overrides=1;if(!root_valid(save_root))return "file-volume-root";
        } else if(!strcmp(argv[i],"--volume-id") && i+1<argc && !disc_id_seen) {
            disc_id_seen=1;overrides=1;if(!np_file_volume_id_parse(argv[++i],r->disc_volume))return "file-volume-id-format";
        } else if(!strcmp(argv[i],"--save-volume-id") && i+1<argc && !save_id_seen) {
            save_id_seen=1;overrides=1;if(!np_file_volume_id_parse(argv[++i],r->save_volume))return "file-volume-id-format";
        } else if(!strcmp(argv[i],"--license")){r->options.mode=NP_COMPAT_CUSTOM;r->options.enable|=NP_FIX_LICENSE;overrides=1;}
        else if(!strcmp(argv[i],"--adaptive-audio") || !strcmp(argv[i],"--no-adaptive-audio")) {
            r->options.mode=NP_COMPAT_CUSTOM;
            if(!strcmp(argv[i],"--adaptive-audio")) {
                r->options.enable|=NP_FIX_ADAPTIVE_AUDIO;
                r->options.disable&=~NP_FIX_ADAPTIVE_AUDIO;
            } else {
                r->options.disable|=NP_FIX_ADAPTIVE_AUDIO;
                r->options.enable&=~NP_FIX_ADAPTIVE_AUDIO;
            }
            overrides=1;
        }
        else if(!strcmp(argv[i],"--code-validation") || !strcmp(argv[i],"--no-code-validation")) {
            r->options.mode=NP_COMPAT_CUSTOM;
            if(!strcmp(argv[i],"--code-validation")) {
                r->options.enable|=NP_FIX_CODE_VALIDATION;
                r->options.disable&=~NP_FIX_CODE_VALIDATION;
            } else {
                r->options.disable|=NP_FIX_CODE_VALIDATION;
                r->options.enable&=~NP_FIX_CODE_VALIDATION;
            }
            overrides=1;
        }
        else if(!strcmp(argv[i],"--hdtv-fix") || !strcmp(argv[i],"--no-hdtv-fix")) {
            r->options.mode=NP_COMPAT_CUSTOM;
            if(!strcmp(argv[i],"--hdtv-fix")) {
                r->options.enable|=NP_FIX_HDTV;
                r->options.disable&=~NP_FIX_HDTV;
            } else {
                r->options.disable|=NP_FIX_HDTV;
                r->options.enable&=~NP_FIX_HDTV;
            }
            overrides=1;
        }
        else return "file-launch-arguments";
    }
    r->disc.backend=selected_backend;r->saves.backend=selected_save_backend?selected_save_backend:selected_backend;
    if(!save_device_seen && r->saves.backend==r->disc.backend)r->saves.device=r->disc.device;
    if((!selected && !request_path) || (request_path && overrides))return "file-launch-arguments";
    if(!np_file_card_pair_valid(r->cards))return "file-launch-card-paths";
    if(np_file_endpoint_check(&r->disc,0) || np_file_endpoint_check(&r->saves,1))return "file-endpoint-device";
    np_startup_selection(np_file_backend_driver(r->disc.backend),r->disc.device,
        np_file_backend_driver(r->saves.backend),r->saves.device,r->environment,selected?selected:request_path);
    np_startup_done();
    if(np_file_storage_init(1))return "file-storage-init";
    np_startup_begin(6,"Locate VCD, validate identity and memory cards");
    if(request_path) {
        int fd=fileXioOpen(request_path,FIO_O_RDONLY,0);if(fd<0)return "file-request-open";
        unsigned char extra;int e=read_all(fd,r,sizeof(*r));
        if(!e && fileXioRead(fd,&extra,1)!=0)e=-1;
        int closed=fileXioClose(fd);if(e || closed<0)return "file-request-read";
        const char *error=np_file_launch_check(r,sizeof(*r),NULL);if(error)return error;
        np_startup_selection(np_file_backend_driver(r->disc.backend),r->disc.device,
            np_file_backend_driver(r->saves.backend),r->saves.device,r->environment,r->disc_path);
    } else {
        if(strlen(selected)<8 || strncmp(selected,"mass",4) || selected[4]<'0' || selected[4]>'9' || selected[5]!=':' ||
           strlen(selected+6)>=sizeof(r->disc_path) || !np_file_path_valid(selected+6,sizeof(r->disc_path),0))return "file-image-path";
        memcpy(r->disc_path,selected+6,strlen(selected+6)+1);
        np_startup_operation("Resolve selected VCD after storage reset");
        np_startup_context("%s",selected);
        const char *error=select_initial(r,resolved_selected);if(error)return error;
        selected=resolved_selected;
        np_startup_selection(np_file_backend_driver(r->disc.backend),r->disc.device,
            np_file_backend_driver(r->saves.backend),r->saves.device,r->environment,selected);
        np_startup_done();np_startup_operation("Validate disc and save volume identities");
        char disc_root[8]={0};memcpy(disc_root,selected,6);
        error=volume_check(disc_root,r->disc_volume);if(error)return error;
        if(save_root) {
            if(!np_file_volume_id_present(r->disc_volume))return "file-launch-volume-identities";
            /* Explicit marker IDs survive remounts; save_discover below resolves
             * that volume rather than trusting the old --save-volume hint. */
            if(!np_file_volume_id_present(r->save_volume)) {
                error=root_check(save_root,&r->saves);if(error)return error;
                error=volume_check(save_root,r->save_volume);if(error)return error;
            }
            if(!np_file_volume_id_present(r->save_volume))return "file-launch-volume-identities";
        } else if(!save_id_seen)memcpy(r->save_volume,r->disc_volume,16);
    }
    np_startup_done();np_startup_operation("Resolve selected storage volumes");
    const char *error=locate(r,selected);
    if(!error){np_startup_done();np_startup_operation("Check selected memory cards");error=cards(r);}
    if(error)mounted[0]=save_mounted[0]=0;
    else np_startup_done();
    return error;
}
const char *np_file_runtime_path(const NpFileLaunchRequest *r,const char *name,char *out,size_t size) {
    int n=snprintf(out,size,"%s%s/%s",mounted,r->runtime_root,name);
    return !mounted[0] || n<0 || (size_t)n>=size?"file-runtime-path":NULL;
}
int np_file_read_asset(const NpFileLaunchRequest *r,const char *name,void *target,unsigned bytes,const unsigned char expected[32]) {
    char path[160];iox_stat_t st;
    if(np_file_runtime_path(r,name,path,sizeof(path))){np_startup_error("asset-path");return -1;}
    np_startup_operationf("Check asset size: %s",name);
    np_startup_context("%s",path);
    int status=fileXioGetStat(path,&st);
    if(status<0){np_startup_context("%s stat=%d",path,status);np_startup_error("asset-stat");return -1;}
    if(st.hisize || st.size!=bytes){np_startup_context("%s size=%u:%u expected=%u",name,(unsigned)st.hisize,(unsigned)st.size,bytes);np_startup_error("asset-size");return -1;}
    np_startup_done();np_startup_operationf("Open asset: %s",name);
    int fd=fileXioOpen(path,FIO_O_RDONLY,0);
    if(fd<0){np_startup_context("%s open=%d",path,fd);np_startup_error("asset-open");return -1;}
    np_startup_done();np_startup_operationf("Read asset: %s",name);
    unsigned done=0;int result=0;
    while(done<bytes) {
        unsigned chunk=bytes-done;if(chunk>65536)chunk=65536;
        np_startup_context("%s read=%u/%u",name,done,bytes);
        result=fileXioRead(fd,(unsigned char *)target+done,chunk);
        if(result<=0 || (unsigned)result>chunk)break;
        done+=(unsigned)result;
    }
    if(done!=bytes){np_startup_context("%s read=%u/%u result=%d",name,done,bytes,result);np_startup_error("asset-read");}
    else {np_startup_done();np_startup_operationf("Close asset: %s",name);}
    int closed=fileXioClose(fd);
    if(done!=bytes)return -1;
    if(closed<0){np_startup_context("%s close=%d",name,closed);np_startup_error("asset-close");return -1;}
    np_startup_done();np_startup_operationf("Verify SHA-256: %s",name);
    unsigned char hash[32];np_boot_sha256(target,bytes,hash);
    if(memcmp(hash,expected,32)){np_startup_error("asset-hash");return -1;}
    return 0;
}
const char *np_file_bridge_start(const NpFileLaunchRequest *r) {
    const char *error=storage_contract(r);if(error)return error;
    if(!mounted[0] || !save_mounted[0])return "file-bridge-volume-unverified";
    error=cards(r);if(error)return error;
    const NpFileLaunchRequest *f=r;
    char args[3*NP_FILE_PATH_BYTES+160];unsigned n=0;
    char number[2]={(char)('0'+f->disc.device),0},save_number[2]={(char)('0'+f->saves.device),0},disc_id[33]="-",save_id[33]="-";
    if(np_file_volume_id_present(r->disc_volume))np_file_volume_id_text(r->disc_volume,disc_id);
    if(np_file_volume_id_present(r->save_volume))np_file_volume_id_text(r->save_volume,save_id);
    const char *values[11]={mounted,f->disc_path,f->cards[0],f->cards[1],np_file_backend_driver(f->disc.backend),number,
        save_mounted,np_file_backend_driver(f->saves.backend),save_number,disc_id,save_id};
    for(unsigned i=0;i<11;++i){unsigned length=strlen(values[i])+1;if(length>sizeof(args)-n)return "file-bridge-arguments";memcpy(args+n,values[i],length);n+=length;}
    if(load("psxcore_filebridge",psxcore_filebridge_irx,size_psxcore_filebridge_irx,args,n))return "file-bridge-load";
#if NP_CD_SWAP
    if(disc_set_path[0]) {
        char path[NP_FILE_PATH_BYTES+8];unsigned char binding[32],extra;
        if(join(path,sizeof(path),mounted,disc_set_path))return "disc-set-path";
        int fd=fileXioOpen(path,FIO_O_RDONLY,0);if(fd<0)return "disc-set-open";
        int read_error=read_all(fd,&launch_disc_set,sizeof(launch_disc_set));
        if(!read_error && fileXioRead(fd,&extra,1)!=0)read_error=-1;
        int closed=fileXioClose(fd);if(read_error || closed<0)return "disc-set-read";
        np_file_disc_save_binding(r->cards,r->saves.backend,r->saves.device,r->save_volume,binding);
        error=np_file_disc_set_check(&launch_disc_set,sizeof(launch_disc_set),r->disc_path,r->disc_volume,binding);
        if(error)return error;
        if(fileXioDevctl("pfs0:",NP_FILE_CTL_DISC_SET,&launch_disc_set,sizeof(launch_disc_set),NULL,0)<0)return "disc-set-register";
#if NP_CD_SWAP
        error=prepare_swap(&launch_disc_set);if(error)return error;
        storage_trace("PSXCORE disc-swap=manual targets=%u armed=%u shortcut=L2+R2+Select+direction\n",np_cd_swap.count,np_cd_swap_runtime.enabled);
#endif
        storage_trace("PSXCORE disc-set=registered count=%u initial=%u shared-cards=unchanged\n",launch_disc_set.count,launch_disc_set.initial);
    }
#endif
    if(np_file_probe_only)return NULL;
    uint32_t state[4];
    np_startup_operation("Check bridge save flush and readiness");
    if(fileXioDevctl("pfs1:",NP_FILE_CTL_FLUSH,NULL,0,NULL,0)<0 ||
       fileXioDevctl("pfs1:",NP_FILE_CTL_STATUS,NULL,0,state,sizeof(state))<0 || state[0]!=1 || state[1])return "file-bridge-flush";
    storage_trace("PSXCORE file-saves=ready mode=sony-shared card0=%s card1=%s flush-callback=completed durability=unverified\n",f->cards[0],f->cards[1]);
    return NULL;
}
