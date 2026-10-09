/* Original standalone ATA/FAT32/exFAT backend. Frontend-independent. */
#include "file_storage.h"
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
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>
#include <usbhdfsd-common.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODULE(name) extern unsigned char name##_irx[];extern unsigned int size_##name##_irx
MODULE(iomanX);MODULE(fileXio);MODULE(ps2dev9);MODULE(bdm);MODULE(bdmfs_fatfs);
MODULE(ata_bd);MODULE(psxcore_nativegate);MODULE(psxcore_filebridge);
static char mounted[8];
int np_file_probe_only;
static int load(const char *name,const unsigned char *bytes,unsigned size,const char *args,unsigned length) {
    int result=-1;
    int id=SifExecModuleBuffer((void *)bytes,size,length,args,&result);
    printf("PSXCORE file-module=%s id=%d result=%d\n",name,id,result);
    return id<0 || result!=0?-1:0;
}
#define LOAD(name) do { if(load(#name,name##_irx,size_##name##_irx,NULL,0))return -1; } while(0)
int np_file_storage_init_pops(void) {
    /* Called only after the final, verified POPS IOP reset. The initial
     * storage environment is discarded and must not supply this pointer. */
    LOAD(psxcore_nativegate);
    return np_file_storage_init(0);
}
int np_file_storage_init(int reset) {
    if(reset) {
        unsigned i;
        for(i=0;i<300 && !SifIopReset("",0);++i)usleep(10000);
        if(i==300)return -1;
        for(i=0;i<300 && !SifIopSync();++i)usleep(10000);
        if(i==300)return -1;
        SifInitRpc(0);
    }
    if(reset) {
        int patched=sbv_patch_enable_lmb();
        printf("PSXCORE file-storage=lmb-patch result=%d\n",patched);
        if(patched<0)return -1;
    }
    LOAD(iomanX);LOAD(fileXio);LOAD(ps2dev9);LOAD(bdm);LOAD(bdmfs_fatfs);LOAD(ata_bd);
    if(fileXioInit()<0)return -1;
    for(unsigned i=0;i<50;++i) {
        int fd=fileXioDopen("mass0:/");
        if(fd>=0){int e=fileXioDclose(fd);return e<0?-1:0;}
        usleep(100000);
    }
    return -1;
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
static const char *measure(const char *path,NpFileLaunchRequest *r,int identify) {
    iox_stat_t before,after;unsigned char head[NP_VCD_HEADER_BYTES];
    if(fileXioGetStat(path,&before)<0 || !FIO_S_ISREG(before.mode))return "file-image-stat";
    uint64_t bytes=((uint64_t)before.hisize<<32)|before.size;
    if(bytes>INT32_MAX)return "file-image-size";
    int fd=fileXioOpen(path,FIO_O_RDONLY,0);if(fd<0)return "file-image-open";
    const char *error=NULL;
    if(read_all(fd,head,sizeof(head)))error="file-image-header-read";
    if(!error)error=np_vcd_check(head,sizeof(head),bytes,NULL);
    char driver[8]={0};uint32_t device=UINT32_MAX;
    if(!error && (fileXioIoctl2(fd,USBMASS_IOCTL_GET_DRIVERNAME,NULL,0,driver,sizeof(driver))<0 ||
        fileXioIoctl2(fd,USBMASS_IOCTL_GET_DEVICE_NUMBER,NULL,0,&device,sizeof(device))<0 ||
        strcmp(driver,"ata") || device))error="file-image-device";
    unsigned char *buffer=!error?malloc(32768):NULL;
    if(!error && !buffer)error="file-image-memory";
    if(!error && fileXioLseek64(fd,0,SEEK_SET)!=0)error="file-image-seek";
    NpSha256 hash;NpSha1 header;np_sha256_init(&hash);np_sha1_init(&header);
    uint64_t done=0,next_progress=16u*1024*1024;
    while(!error && done<bytes) {
        unsigned take=bytes-done>32768?32768:(unsigned)(bytes-done);
        int n=fileXioRead(fd,buffer,take);
        if(n<=0 || (unsigned)n>take){error="file-image-read";break;}
        np_sha256_update(&hash,buffer,n);
        if(done<262144){unsigned h=262144-done;if(h>(unsigned)n)h=n;np_sha1_update(&header,buffer,h);}
        done+=(unsigned)n;
        if(done>=next_progress){printf("PSXCORE file-verify=%u/%u MiB\n",(unsigned)(done>>20),(unsigned)(bytes>>20));next_progress+=16u*1024*1024;}
    }
    if(!error && fileXioRead(fd,buffer,1)!=0)error="file-image-changed";
    if(!error && identify)serial(fd,bytes,r->serial);
    if(fileXioClose(fd)<0 && !error)error="file-image-close";
    free(buffer);
    if(!error && (fileXioGetStat(path,&after)<0 || after.size!=before.size || after.hisize!=before.hisize ||
        memcmp(after.mtime,before.mtime,sizeof(before.mtime))))error="file-image-changed";
    if(!error){r->image_bytes=bytes;np_sha256_final(&hash,r->disc_sha256);np_sha1_final(&header,r->header_sha1);}
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
static int write_all(int fd,const void *data,unsigned size) {
    unsigned done=0;
    while(done<size){int n=fileXioWrite(fd,(const unsigned char *)data+done,size-done);if(n<=0 || (unsigned)n>size-done)return -1;done+=n;}
    return 0;
}
static int new_file(const char *path,const void *bytes,unsigned size,int blank) {
    char temp[128];iox_stat_t st;
    if(snprintf(temp,sizeof(temp),"%s.new",path)>=(int)sizeof(temp))return -1;
    if(!absent(fileXioGetStat(path,&st)) || !absent(fileXioGetStat(temp,&st)))return -1;
    int fd=fileXioOpen(temp,FIO_O_WRONLY|FIO_O_CREAT,0666);if(fd<0)return -1;
    int e=0;
    if(blank) {
        unsigned char frame[128];
        for(unsigned i=0;i<1024 && !e;++i){np_file_card_frame(i,frame);e=write_all(fd,frame,sizeof(frame));}
    } else e=write_all(fd,bytes,size);
    int closed=fileXioClose(fd);
    if(e || closed<0)return -1; /* Preserve incomplete .new for explicit recovery. */
    if(blank && card(temp))return -1;
    return fileXioRename(temp,path)<0?-1:0;
}
static int directory(const char *path,int provision) {
    iox_stat_t st;int e=fileXioGetStat(path,&st);
    if(e>=0)return FIO_S_ISDIR(st.mode)?0:-1;
    return absent(e) && provision && fileXioMkdir(path,0777)>=0?0:-1;
}
static const char *saves(const NpFileLaunchRequest *r,int provision) {
    if(np_file_probe_only)return NULL;
    char base[96],path[128],owner[66],actual[66];iox_stat_t st;
    snprintf(base,sizeof(base),"%s/POPS",mounted);if(directory(base,provision))return "file-save-directory";
    snprintf(base,sizeof(base),"%s/POPS/SAVES",mounted);if(directory(base,provision))return "file-save-directory";
    snprintf(base,sizeof(base),"%s/POPS/SAVES/%s",mounted,r->save_key);if(directory(base,provision))return "file-save-directory";
    for(unsigned i=0;i<32;++i)snprintf(owner+i*2,3,"%02x",r->disc_sha256[i]);
    owner[64]='\n';owner[65]=0;
    snprintf(path,sizeof(path),"%s/owner.sha256",base);
    int e=fileXioGetStat(path,&st);
    if(absent(e) && provision) {
        for(unsigned i=0;i<2;++i) {
            char current[128];snprintf(current,sizeof(current),"%s/card%u",base,i);
            if(!absent(fileXioGetStat(current,&st)))return "file-save-owner-missing";
        }
        if(new_file(path,owner,65,0))return "file-save-owner-create";
        e=fileXioGetStat(path,&st);
    }
    if(e<0 || st.hisize || st.size!=65)return "file-save-owner";
    int fd=fileXioOpen(path,FIO_O_RDONLY,0);if(fd<0)return "file-save-owner";
    e=read_all(fd,actual,65);int closed=fileXioClose(fd);
    if(e || closed<0 || memcmp(actual,owner,65))return "file-save-owner-mismatch";
    for(unsigned i=0;i<2;++i) {
        snprintf(path,sizeof(path),"%s/card%u",base,i);
        e=card(path);
        if(absent(e) && provision){if(new_file(path,NULL,0,1))return "file-save-card-create";e=card(path);}
        if(e)return "file-save-card-invalid";
        static const char *const suffix[]={".bak",".new"};
        for(unsigned j=0;j<2;++j){snprintf(path,sizeof(path),"%s/card%u%s",base,i,suffix[j]);if(!absent(fileXioGetStat(path,&st)))return "file-save-recovery-required";}
        snprintf(path,sizeof(path),"%s/card%u.prev",base,i);e=card(path);
        if(e && !absent(e))return "file-save-previous-invalid";
    }
    return NULL;
}
const char *np_file_reopen(const NpFileLaunchRequest *r,int provision) {
    const char *error=np_file_launch_check(r,sizeof(*r),NULL);if(error)return error;
    unsigned matches=0;mounted[0]=0;
    for(unsigned i=0;i<10;++i) {
        char root[8],path[NP_FILE_PATH_BYTES+8];snprintf(root,sizeof(root),"mass%u:",i);
        if(join(path,sizeof(path),root,r->disc_path))return "file-image-path";
        NpFileLaunchRequest found;np_file_launch_init(&found);
        error=measure(path,&found,1);
        if(error)continue;
        if(found.image_bytes!=r->image_bytes || memcmp(found.disc_sha256,r->disc_sha256,32) ||
           memcmp(found.header_sha1,r->header_sha1,20) || strcmp(found.serial,r->serial))continue;
        if(++matches>1)return "file-image-volume-ambiguous";
        memcpy(mounted,root,sizeof(mounted));
    }
    if(matches!=1)return "file-image-not-found-or-changed";
    printf("PSXCORE file-image=verified root=%s bytes=%u serial=%s\n",mounted,(unsigned)r->image_bytes,r->serial[0]?r->serial:"unidentified");
    return saves(r,provision);
}
const char *np_file_prepare(int argc,char **argv,NpFileLaunchRequest *r) {
    np_file_probe_only=0;
    np_file_launch_init(r);const char *selected=NULL,*request_path=NULL;int provision=0,overrides=0;
    for(int i=1;i<argc;++i) {
        if(!strcmp(argv[i],"--file") && i+1<argc && !selected && !request_path)selected=argv[++i];
        else if(!strcmp(argv[i],"--file-request") && i+1<argc && !selected && !request_path)request_path=argv[++i];
        else if(!strcmp(argv[i],"--init-cards"))provision=1;
        else if(!strcmp(argv[i],"--probe-synthetic"))np_file_probe_only=1;
        else if(!strcmp(argv[i],"--pcsx2") && !request_path){r->environment=NP_ENV_PCSX2;overrides=1;}
        else if(!strcmp(argv[i],"--license") && !request_path){r->options.mode=NP_COMPAT_CUSTOM;r->options.enable|=NP_FIX_LICENSE;overrides=1;}
        else return "file-launch-arguments";
    }
    if((!selected && !request_path) || (provision && np_file_probe_only) || (request_path && overrides))return "file-launch-arguments";
    if(np_file_storage_init(1))return "file-storage-init";
    if(request_path) {
        int fd=fileXioOpen(request_path,FIO_O_RDONLY,0);if(fd<0)return "file-request-open";
        int e=read_all(fd,r,sizeof(*r));unsigned char extra;
        if(!e && fileXioRead(fd,&extra,1)!=0)e=-1;
        int closed=fileXioClose(fd);if(e || closed<0)return "file-request-read";
    } else {
        if(strlen(selected)<8 || strncmp(selected,"mass",4) || selected[4]<'0' || selected[4]>'9' || selected[5]!=':' ||
           strlen(selected+6)>=sizeof(r->disc_path) || !np_file_path_valid(selected+6,sizeof(r->disc_path),0))return "file-image-path";
        memcpy(r->disc_path,selected+6,strlen(selected+6)+1);
        const char *error=measure(selected,r,1);if(error)return error;
        for(unsigned i=0;i<16;++i)snprintf(r->save_key+i*2,3,"%02x",r->disc_sha256[i]);
    }
    return np_file_reopen(r,provision);
}
const char *np_file_runtime_path(const NpFileLaunchRequest *r,const char *name,char *out,size_t size) {
    int n=snprintf(out,size,"%s%s/%s",mounted,r->runtime_root,name);
    return !mounted[0] || n<0 || (size_t)n>=size?"file-runtime-path":NULL;
}
int np_file_read_asset(const NpFileLaunchRequest *r,const char *name,void *target,unsigned bytes,const unsigned char expected[32]) {
    char path[160];iox_stat_t st;
    if(np_file_runtime_path(r,name,path,sizeof(path)) || fileXioGetStat(path,&st)<0 || st.hisize || st.size!=bytes)return -1;
    int fd=fileXioOpen(path,FIO_O_RDONLY,0);if(fd<0)return -1;
    int e=read_all(fd,target,bytes);int closed=fileXioClose(fd);if(e || closed<0)return -1;
    unsigned char hash[32];np_boot_sha256(target,bytes,hash);return memcmp(hash,expected,32)?-1:0;
}
const char *np_file_bridge_start(const NpFileLaunchRequest *r) {
    char args[NP_FILE_PATH_BYTES+48];unsigned n=0;
    const char *values[3]={mounted,r->disc_path,r->save_key};
    for(unsigned i=0;i<3;++i){unsigned length=strlen(values[i])+1;memcpy(args+n,values[i],length);n+=length;}
    if(load("psxcore_filebridge",psxcore_filebridge_irx,size_psxcore_filebridge_irx,args,n))return "file-bridge-load";
    if(np_file_probe_only)return NULL;
    uint32_t state[4];
    if(fileXioDevctl("pfs1:",NP_FILE_CTL_FLUSH,NULL,0,NULL,0)<0 ||
       fileXioDevctl("pfs1:",NP_FILE_CTL_STATUS,NULL,0,state,sizeof(state))<0 || state[0]!=1 || state[1])return "file-bridge-flush";
    printf("PSXCORE file-saves=ready isolated=yes flush-callback=completed device-errors=limited durability=unverified\n");
    return NULL;
}
