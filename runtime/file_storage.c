/* Original standalone ATA/USB FAT32/exFAT backend. Frontend-independent. */
#include "file_storage.h"
#include "psxcore_hash.h"
#include "psxcore_boot.h"
#include "psxcore_shared_store.h"
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
MODULE(ata_bd);MODULE(usbd);MODULE(usbmass_bd);MODULE(psxcore_nativegate);MODULE(psxcore_filebridge);
static char mounted[8],save_mounted[8];
static uint32_t selected_backend=NP_FILE_BACKEND_ATA;
/* Zero inherits the game backend, preserving initial/post-reset defaults. */
static uint32_t selected_save_backend;
int np_file_probe_only;
int np_file_management_only;
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
    uint32_t save_backend=selected_save_backend?selected_save_backend:selected_backend;
    if((selected_backend!=NP_FILE_BACKEND_ATA && selected_backend!=NP_FILE_BACKEND_USB) ||
       (save_backend!=NP_FILE_BACKEND_ATA && save_backend!=NP_FILE_BACKEND_USB))return -1;
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
    LOAD(iomanX);LOAD(fileXio);LOAD(bdm);LOAD(bdmfs_fatfs);
    if(selected_backend==NP_FILE_BACKEND_ATA || save_backend==NP_FILE_BACKEND_ATA){LOAD(ps2dev9);LOAD(ata_bd);}
    if(selected_backend==NP_FILE_BACKEND_USB || save_backend==NP_FILE_BACKEND_USB){LOAD(usbd);LOAD(usbmass_bd);}
    if(fileXioInit()<0)return -1;
    for(unsigned i=0;i<50;++i) {
        for(unsigned unit=0;unit<10;++unit) {
            char root[9];snprintf(root,sizeof(root),"mass%u:/",unit);
            int fd=fileXioDopen(root);
            if(fd>=0){int e=fileXioDclose(fd);return e<0?-1:0;}
        }
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
static const char *measure(const char *path,NpFileLaunchRequestV4 *r,int identify) {
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
        !memchr(driver,0,sizeof(driver)) || strcmp(driver,np_file_backend_driver(r->disc.backend)) ||
        device!=r->disc.device))error="file-image-device";
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
static int root_valid(const char *root) {
    return root && strlen(root)==6 && !strncmp(root,"mass",4) && root[4]>='0' && root[4]<='9' && root[5]==':';
}
/* The pinned SDK supports these two metadata ioctls on directory handles:
 * FIL and DIR start with FFOBJID, and only obj.fs->pdrv is consulted. */
static const char *root_check(const char *root,const NpFileEndpoint *endpoint) {
    if(!root_valid(root))return "file-volume-root";
    char path[9],driver[8]={0};uint32_t device=UINT32_MAX;
    snprintf(path,sizeof(path),"%s/",root);
    int fd=fileXioDopen(path);if(fd<0)return "file-volume-open";
    int e=fileXioIoctl2(fd,USBMASS_IOCTL_GET_DRIVERNAME,NULL,0,driver,sizeof(driver));
    if(e>=0)e=fileXioIoctl2(fd,USBMASS_IOCTL_GET_DEVICE_NUMBER,NULL,0,&device,sizeof(device));
    int closed=fileXioDclose(fd);
    if(e<0 || closed<0 || !memchr(driver,0,sizeof(driver)) ||
       strcmp(driver,np_file_backend_driver(endpoint->backend)) || device!=endpoint->device)return "file-volume-device";
    return NULL;
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
static const char *volume_setup(const char *root,unsigned char id[16],int provision) {
    if(provision) {
        if(!np_file_volume_id_present(id))return "file-volume-id-required";
        char path[64];snprintf(path,sizeof(path),"%s/POPS",root);
        if(directory(path,1))return "file-volume-directory";
    }
    unsigned char actual[16];const char *error=volume_read(root,actual);
    if(error && strcmp(error,"file-volume-id-missing"))return error;
    if(error) {
        if(!np_file_volume_id_present(id))return provision?"file-volume-id-required":NULL;
        if(!provision)return error;
        char path[64];snprintf(path,sizeof(path),"%s/POPS",root);
        if(directory(path,1) || join(path,sizeof(path),root,NP_FILE_VOLUME_MARKER))return "file-volume-directory";
        NpFileVolumeMarker marker;np_file_volume_marker_init(&marker,id);
        if(new_file(path,&marker,sizeof(marker),0))return "file-volume-marker-create";
        error=volume_read(root,actual);if(error)return error;
    }
    if(np_file_volume_id_present(id) && memcmp(id,actual,16))return "file-volume-id-mismatch";
    memcpy(id,actual,16);return NULL;
}
static const char *saves(const NpFileLaunchRequestV5 *request,int provision) {
    const NpFileLaunchRequestV4 *r=&request->launch;
    if(r->save_mode==NP_SAVE_SHARED) {
        if(np_file_probe_only || provision)return "file-shared-operation-conflict";
        char base[96];NpSharedSet set;
        snprintf(base,sizeof(base),"%s/POPS/SAVES/%s",save_mounted,r->save_key);
        const char *error=np_shared_store_read(base,&set);
        return error?error:np_shared_match(&set,NP_SHARED_FILE,r->save_key,request->save_volume,r->disc_sha256);
    }
    if(np_file_probe_only)return NULL;
    char base[96],path[128],owner[66],actual[66];iox_stat_t st;
    snprintf(base,sizeof(base),"%s/POPS",save_mounted);if(directory(base,provision))return "file-save-directory";
    snprintf(base,sizeof(base),"%s/POPS/SAVES",save_mounted);if(directory(base,provision))return "file-save-directory";
    snprintf(base,sizeof(base),"%s/POPS/SAVES/%s",save_mounted,r->save_key);if(directory(base,provision))return "file-save-directory";
    static const char *const shared_files[]={"shared.set","shared.set.new","shared.set.prev","shared.set.bak"};
    for(unsigned i=0;i<4;++i){snprintf(path,sizeof(path),"%s/%s",base,shared_files[i]);if(!absent(fileXioGetStat(path,&st)))return "file-save-shared-conflict";}
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
static const char *storage_contract(const NpFileLaunchRequestV5 *r) {
    const char *error=np_file_launch_check_v5(r,sizeof(*r),NULL);if(error)return error;
    const NpFileLaunchRequestV4 *f=&r->launch;
    if((f->disc.backend!=NP_FILE_BACKEND_ATA && f->disc.backend!=NP_FILE_BACKEND_USB) ||
       (f->saves.backend!=NP_FILE_BACKEND_ATA && f->saves.backend!=NP_FILE_BACKEND_USB))return "file-storage-backend-unimplemented";
    uint32_t save_backend=selected_save_backend?selected_save_backend:selected_backend;
    if(f->disc.backend!=selected_backend || f->saves.backend!=save_backend)return "file-storage-backend-mismatch";
    if((f->disc.required|f->saves.required)&~NP_FILE_SAVE_CAPS)return "file-storage-capabilities-unavailable";
    return NULL;
}
static const char *save_discover(const NpFileLaunchRequestV5 *r) {
    save_mounted[0]=0;
    if(!np_file_volume_id_present(r->save_volume)) {
        if(r->launch.disc.backend!=r->launch.saves.backend || r->launch.disc.device!=r->launch.saves.device)return "file-launch-volume-identities";
        memcpy(save_mounted,mounted,sizeof(save_mounted));return NULL;
    }
    unsigned matches=0;
    for(unsigned i=0;i<10;++i) {
        char root[8];unsigned char id[16];snprintf(root,sizeof(root),"mass%u:",i);
        if(root_check(root,&r->launch.saves) || volume_read(root,id) || memcmp(id,r->save_volume,16))continue;
        if(++matches>1){save_mounted[0]=0;return "file-save-volume-ambiguous";}
        memcpy(save_mounted,root,sizeof(save_mounted));
    }
    return matches==1?NULL:"file-save-volume-not-found-or-changed";
}
static const char *locate(NpFileLaunchRequestV5 *r) {
    mounted[0]=save_mounted[0]=0;
    const char *error=storage_contract(r);if(error)return error;
    NpFileLaunchRequestV4 *f=&r->launch;
    unsigned matches=0,identity_matches=0;unsigned char selected_id[16]={0};
    for(unsigned i=0;i<10;++i) {
        char root[8],path[NP_FILE_PATH_BYTES+8];unsigned char id[16];snprintf(root,sizeof(root),"mass%u:",i);
        if(root_check(root,&f->disc))continue;
        const char *marker_error=volume_read(root,id);
        if(np_file_volume_id_present(r->disc_volume) && (marker_error || memcmp(id,r->disc_volume,16)))continue;
        if(np_file_volume_id_present(r->disc_volume) && ++identity_matches>1){mounted[0]=0;return "file-image-volume-ambiguous";}
        if(join(path,sizeof(path),root,f->disc_path))return "file-image-path";
        NpFileLaunchRequestV4 found;np_file_launch_init_v4(&found);found.disc=f->disc;found.saves=f->saves;
        error=measure(path,&found,1);if(error)continue;
        if(found.image_bytes!=f->image_bytes || memcmp(found.disc_sha256,f->disc_sha256,32) ||
           memcmp(found.header_sha1,f->header_sha1,20) || strcmp(found.serial,f->serial))continue;
        if(marker_error && strcmp(marker_error,"file-volume-id-missing")){mounted[0]=0;return marker_error;}
        if(++matches>1){mounted[0]=0;return "file-image-volume-ambiguous";}
        memcpy(mounted,root,sizeof(mounted));memcpy(selected_id,id,16);
    }
    if(matches!=1)return "file-image-not-found-or-changed";
    /* Legacy requests acquire an existing marker on first verification. */
    if(!np_file_volume_id_present(r->disc_volume) && np_file_volume_id_present(selected_id)) {
        memcpy(r->disc_volume,selected_id,16);memcpy(r->save_volume,selected_id,16);
    }
    error=save_discover(r);
    if(error){mounted[0]=save_mounted[0]=0;return error;}
    printf("PSXCORE file-image=verified root=%s saves-root=%s bytes=%u serial=%s volume-id=%s\n",mounted,save_mounted,
        (unsigned)f->image_bytes,f->serial[0]?f->serial:"unidentified",np_file_volume_id_present(r->disc_volume)?"verified":"legacy-unbound");
    return NULL;
}
const char *np_file_reopen(NpFileLaunchRequestV5 *r,int provision) {
    const char *error=locate(r);
    if(!error)error=saves(r,provision);
    if(error)mounted[0]=save_mounted[0]=0;
    return error;
}
static const char *manage_shared(const NpFileLaunchRequestV5 *r,const char *name,int enroll,int copy) {
    const NpFileLaunchRequestV4 *f=&r->launch;
    char base[96];snprintf(base,sizeof(base),"%s/POPS/SAVES/%s",save_mounted,f->save_key);
    if(name) {
        NpSharedSet set;const char *error=np_shared_init(&set,NP_SHARED_FILE,f->save_key,name,r->save_volume);
        if(!error)error=np_shared_member(&set,f->disc_sha256,1);
        if(error)return error;
        char source[96];source[0]=0;
        if(copy) {
            NpFileLaunchRequestV5 private=*r;private.launch.save_mode=NP_SAVE_ISOLATED;
            for(unsigned i=0;i<16;++i)snprintf(private.launch.save_key+i*2,3,"%02x",f->disc_sha256[i]);
            error=saves(&private,0);if(error)return error;
            snprintf(source,sizeof(source),"%s/POPS/SAVES/%s",save_mounted,private.launch.save_key);
        }
        char parent[64];snprintf(parent,sizeof(parent),"%s/POPS/SAVES",save_mounted);
        if(directory(parent,1) || directory(base,1))return "file-save-directory";
        return np_shared_store_create(base,&set,copy?source:NULL);
    }
    NpSharedSet current,next;const char *error=np_shared_store_read(base,&current);if(error)return error;
    if(current.storage!=NP_SHARED_FILE || strcmp(current.key,f->save_key) || memcmp(current.volume,r->save_volume,16))return "shared-set-identity";
    next=current;error=np_shared_member(&next,f->disc_sha256,enroll);
    return error?error:np_shared_store_update(base,&current,&next);
}
static uint32_t backend_number(const char *name) {
    return !strcmp(name,"ata")?NP_FILE_BACKEND_ATA:!strcmp(name,"usb")?NP_FILE_BACKEND_USB:0;
}
static int device_number(const char *number,uint32_t *out) {
    if((number[0]!='0' && number[0]!='1') || number[1])return 0;
    *out=(uint32_t)(number[0]-'0');return 1;
}
const char *np_file_prepare(int argc,char **argv,NpFileLaunchRequestV5 *r) {
    np_file_probe_only=np_file_management_only=0;mounted[0]=save_mounted[0]=0;
    np_file_launch_init_v5(r);selected_backend=NP_FILE_BACKEND_ATA;selected_save_backend=0;
    NpFileLaunchRequestV4 *f=&r->launch;
    const char *selected=NULL,*request_path=NULL,*save_root=NULL;
    const char *shared_key=NULL,*shared_name=NULL;int membership=0,copy_private=0;
    int provision=0,overrides=0,backend_seen=0,device_seen=0,save_backend_seen=0,save_device_seen=0;
    int disc_id_seen=0,save_id_seen=0,init_disc_id=0,init_save_id=0;
    for(int i=1;i<argc;++i) {
        if(!strcmp(argv[i],"--file") && i+1<argc && !selected && !request_path)selected=argv[++i];
        else if(!strcmp(argv[i],"--file-request") && i+1<argc && !selected && !request_path)request_path=argv[++i];
        else if(!strcmp(argv[i],"--init-cards"))provision=1;
        else if(!strcmp(argv[i],"--probe-synthetic"))np_file_probe_only=1;
        else if(!strcmp(argv[i],"--shared-set") && i+1<argc && !shared_key) {
            shared_key=argv[++i];overrides=1;
            if(!np_shared_key_valid(shared_key,NP_SHARED_FILE))return "shared-set-format";
        } else if(!strcmp(argv[i],"--create-shared") && i+1<argc && !shared_name) {
            shared_name=argv[++i];np_file_management_only=1;overrides=1;
        } else if(!strcmp(argv[i],"--copy-private") && !copy_private){copy_private=1;overrides=1;}
        else if(!strcmp(argv[i],"--enroll-shared") && !membership){membership=1;np_file_management_only=1;overrides=1;}
        else if(!strcmp(argv[i],"--unenroll-shared") && !membership){membership=-1;np_file_management_only=1;overrides=1;}
        else if(!strcmp(argv[i],"--backend") && i+1<argc && !backend_seen) {
            backend_seen=1;selected_backend=backend_number(argv[++i]);if(!selected_backend)return "file-storage-backend-unimplemented";
        } else if(!strcmp(argv[i],"--save-backend") && i+1<argc && !save_backend_seen) {
            save_backend_seen=1;selected_save_backend=backend_number(argv[++i]);if(!selected_save_backend)return "file-storage-backend-unimplemented";
        } else if(!strcmp(argv[i],"--device") && i+1<argc && !device_seen) {
            device_seen=1;overrides=1;if(!device_number(argv[++i],&f->disc.device))return "file-endpoint-device";
        } else if(!strcmp(argv[i],"--save-device") && i+1<argc && !save_device_seen) {
            save_device_seen=1;overrides=1;if(!device_number(argv[++i],&f->saves.device))return "file-endpoint-device";
        } else if(!strcmp(argv[i],"--save-volume") && i+1<argc && !save_root) {
            save_root=argv[++i];overrides=1;if(!root_valid(save_root))return "file-volume-root";
        } else if(!strcmp(argv[i],"--volume-id") && i+1<argc && !disc_id_seen) {
            disc_id_seen=1;overrides=1;if(!np_file_volume_id_parse(argv[++i],r->disc_volume))return "file-volume-id-format";
        } else if(!strcmp(argv[i],"--save-volume-id") && i+1<argc && !save_id_seen) {
            save_id_seen=1;overrides=1;if(!np_file_volume_id_parse(argv[++i],r->save_volume))return "file-volume-id-format";
        } else if(!strcmp(argv[i],"--init-volume-id") && !init_disc_id){init_disc_id=1;overrides=1;}
        else if(!strcmp(argv[i],"--init-save-volume-id") && !init_save_id){init_save_id=1;overrides=1;}
        else if(!strcmp(argv[i],"--pcsx2")){f->environment=NP_ENV_PCSX2;overrides=1;}
        else if(!strcmp(argv[i],"--license")){f->options.mode=NP_COMPAT_CUSTOM;f->options.enable|=NP_FIX_LICENSE;overrides=1;}
        else return "file-launch-arguments";
    }
    f->disc.backend=selected_backend;f->saves.backend=selected_save_backend?selected_save_backend:selected_backend;
    if(!save_device_seen && f->saves.backend==f->disc.backend)f->saves.device=f->disc.device;
    if((!selected && !request_path) || ((provision||init_disc_id||init_save_id) && np_file_probe_only) ||
       (request_path && overrides) || (init_disc_id && !disc_id_seen) || (init_save_id && (!save_root||!save_id_seen)))return "file-launch-arguments";
    if((np_file_management_only && !shared_key) || (shared_name && membership) || (copy_private && !shared_name) ||
       (shared_key && (provision || np_file_probe_only)))return "file-shared-operation-conflict";
    if(np_file_endpoint_check(&f->disc,0) || np_file_endpoint_check(&f->saves,1))return "file-endpoint-device";
    if(np_file_storage_init(1))return "file-storage-init";
    if(request_path) {
        int fd=fileXioOpen(request_path,FIO_O_RDONLY,0);if(fd<0)return "file-request-open";
        unsigned char bytes[sizeof(*r)],extra;size_t size=0;int e=read_all(fd,bytes,8);
        if(!e) {
            unsigned version=bytes[4]|(unsigned)bytes[5]<<8;
            size=version==NP_FILE_VERSION?sizeof(NpFileLaunchRequest):version==NP_FILE_VERSION_4?sizeof(NpFileLaunchRequestV4):version==NP_FILE_VERSION_5?sizeof(*r):0;
            if(!size || read_all(fd,bytes+8,(unsigned)size-8) || fileXioRead(fd,&extra,1)!=0)e=-1;
        }
        int closed=fileXioClose(fd);if(e || closed<0)return "file-request-read";
        const char *error=np_file_launch_decode_v5(bytes,size,r,NULL);if(error)return error;
    } else {
        if(strlen(selected)<8 || strncmp(selected,"mass",4) || selected[4]<'0' || selected[4]>'9' || selected[5]!=':' ||
           strlen(selected+6)>=sizeof(f->disc_path) || !np_file_path_valid(selected+6,sizeof(f->disc_path),0))return "file-image-path";
        memcpy(f->disc_path,selected+6,strlen(selected+6)+1);
        const char *error=measure(selected,f,1);if(error)return error;
        for(unsigned i=0;i<16;++i)snprintf(f->save_key+i*2,3,"%02x",f->disc_sha256[i]);
        char disc_root[8]={0};memcpy(disc_root,selected,6);
        error=volume_setup(disc_root,r->disc_volume,init_disc_id);if(error)return error;
        if(save_root) {
            error=root_check(save_root,&f->saves);if(error)return error;
            if(!np_file_volume_id_present(r->disc_volume))return "file-launch-volume-identities";
            error=volume_setup(save_root,r->save_volume,init_save_id);if(error)return error;
            if(!np_file_volume_id_present(r->save_volume))return "file-launch-volume-identities";
        } else if(!save_id_seen)memcpy(r->save_volume,r->disc_volume,16);
        if(shared_key){strcpy(f->save_key,shared_key);f->save_mode=NP_SAVE_SHARED;}
    }
    if(np_file_management_only) {
        const char *error=locate(r);
        if(!error)error=manage_shared(r,shared_name,membership>0,copy_private);
        if(error){mounted[0]=save_mounted[0]=0;return error;}
        printf("PSXCORE shared-set=%s operation=%s cards=%s execution=disabled\n",f->save_key,
            shared_name?"created":membership>0?"enrolled":"unenrolled",shared_name?(copy_private?"copied-private":"blank"):"preserved");
        return NULL;
    }
    return np_file_reopen(r,provision);
}
const char *np_file_runtime_path(const NpFileLaunchRequestV5 *r,const char *name,char *out,size_t size) {
    int n=snprintf(out,size,"%s%s/%s",mounted,r->launch.runtime_root,name);
    return !mounted[0] || n<0 || (size_t)n>=size?"file-runtime-path":NULL;
}
int np_file_read_asset(const NpFileLaunchRequestV5 *r,const char *name,void *target,unsigned bytes,const unsigned char expected[32]) {
    char path[160];iox_stat_t st;
    if(np_file_runtime_path(r,name,path,sizeof(path)) || fileXioGetStat(path,&st)<0 || st.hisize || st.size!=bytes)return -1;
    int fd=fileXioOpen(path,FIO_O_RDONLY,0);if(fd<0)return -1;
    int e=read_all(fd,target,bytes);int closed=fileXioClose(fd);if(e || closed<0)return -1;
    unsigned char hash[32];np_boot_sha256(target,bytes,hash);return memcmp(hash,expected,32)?-1:0;
}
const char *np_file_bridge_start(const NpFileLaunchRequestV5 *r) {
    const char *error=storage_contract(r);if(error)return error;
    if(!mounted[0] || !save_mounted[0])return "file-bridge-volume-unverified";
    if(np_file_management_only)return "file-bridge-management-only";
    error=saves(r,0);if(error)return error;
    const NpFileLaunchRequestV4 *f=&r->launch;
    char args[NP_FILE_PATH_BYTES+160];unsigned n=0;
    char number[2]={(char)('0'+f->disc.device),0},save_number[2]={(char)('0'+f->saves.device),0},disc_id[33]="-",save_id[33]="-";
    if(np_file_volume_id_present(r->disc_volume))np_file_volume_id_text(r->disc_volume,disc_id);
    if(np_file_volume_id_present(r->save_volume))np_file_volume_id_text(r->save_volume,save_id);
    const char *values[10]={mounted,f->disc_path,f->save_key,np_file_backend_driver(f->disc.backend),number,
        save_mounted,np_file_backend_driver(f->saves.backend),save_number,disc_id,save_id};
    for(unsigned i=0;i<10;++i){unsigned length=strlen(values[i])+1;if(length>sizeof(args)-n)return "file-bridge-arguments";memcpy(args+n,values[i],length);n+=length;}
    if(load("psxcore_filebridge",psxcore_filebridge_irx,size_psxcore_filebridge_irx,args,n))return "file-bridge-load";
    if(np_file_probe_only)return NULL;
    uint32_t state[4];
    if(fileXioDevctl("pfs1:",NP_FILE_CTL_FLUSH,NULL,0,NULL,0)<0 ||
       fileXioDevctl("pfs1:",NP_FILE_CTL_STATUS,NULL,0,state,sizeof(state))<0 || state[0]!=1 || state[1])return "file-bridge-flush";
    printf("PSXCORE file-saves=ready mode=%s flush-callback=completed device-errors=limited durability=unverified\n",
        f->save_mode==NP_SAVE_SHARED?"shared-explicit":"isolated");
    return NULL;
}
