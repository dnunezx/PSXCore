/* Original POPS pfs-path adapter. Selected VCD is always read-only.
 * Uses the SDK filesystem rather than copied POPS/IOPCD implementation. */
#ifdef NP_HOST_TEST
#include "filebridge_stub.h"
#else
#include "irx_imports.h"
#endif
#include "psxcore_file.h"
#include "native.h"
#include <errno.h>
#include <stdint.h>
#include <string.h>
IRX_ID("psxcore_file",1,0);
static char root[8],disc[NP_FILE_PATH_BYTES],key[33];
static int lock=-1;
static unsigned writers,flushes,replacements;
static struct block_device *drive;
static u64 drive_sectors;
static volatile int removed;
/* BDM's callback has no device identity. Conservatively latch every unmount;
 * reconnection requires a fresh launch and full image verification. */
static void storage_event(int mounted){if(!mounted)removed=1;}

/* Never reuse an unrelated disk after removal. Filesystem handles additionally
 * reject a disconnected/reconnected volume. ATA0 only in this contract. */
static int connected(void) {
    if(removed)return 0;
    struct block_device *list[20]={0};
    bdm_get_bd(list,20);
    for(unsigned i=0;i<20;++i)
        if(list[i]==drive && drive && !strcmp(drive->name,"ata") && drive->devNr==0 &&
           drive->parNr==0 && drive->sectorCount==drive_sectors && drive->sectorSize==512)return 1;
    return 0;
}
static int enter(void) {
    if(lock<0 || WaitSema(lock)<0)return -EIO;
    if(!connected()){SignalSema(lock);return -ENODEV;}
    return 0;
}
static int leave(int result){SignalSema(lock);return result;}
static int path(unsigned unit,const char *name,char out[NP_FILE_PATH_BYTES+8]) {
    char relative[NP_FILE_PATH_BYTES];
    if(np_file_route(unit,name,disc,key,relative,sizeof(relative)))return -EACCES;
    size_t r=strlen(root),n=strlen(relative);
    if(r+n>=NP_FILE_PATH_BYTES+8)return -ENAMETOOLONG;
    memcpy(out,root,r);memcpy(out+r,relative,n+1);return 0;
}
static int valid_card(const char *name) {
    iox_stat_t st;unsigned char h[128];
    int result=iomanX_getstat(name,&st);
    if(result<0)return result;
    if(!FIO_S_ISREG(st.mode) || st.hisize || st.size!=NP_FILE_CARD_BYTES)return -EINVAL;
    int fd=iomanX_open(name,FIO_O_RDONLY,0);
    if(fd<0)return fd;
    result=iomanX_read(fd,h,sizeof(h));
    int closed=iomanX_close(fd);
    if(result!=sizeof(h) || closed<0)return -EIO;
    return np_file_card_header_valid(h,sizeof(h))?0:-EINVAL;
}
static int missing(int error){return error==-ENOENT || error==-4; /* FatFs FR_NO_FILE */}
/* f_close synchronizes modified file metadata. BDM flush callbacks return void;
 * completion does not prove device error reporting or power-loss durability. */
static int flush(void) {
    if(writers || !connected() || !drive->flush)return -EIO;
    drive->flush(drive);
    if(!connected())return -ENODEV;
    ++flushes;return 0;
}
static int init(iop_device_t *d){(void)d;return 0;}
static int deny(void){return -EACCES;}
static int deny64(void){return -EACCES;}
static int bridge_open(iop_file_t *f,const char *name,int flags,int mode) {
    char target[NP_FILE_PATH_BYTES+8];int e=path(f->unit,name,target);
    if(e)return e;
    int writing=(flags&FIO_O_RDWR)!=FIO_O_RDONLY;
    if(writing) {
        if(f->unit!=1 || (strcmp(name,"/ps1emu/card0.bak") && strcmp(name,"/ps1emu/card1.bak")) ||
           flags!=(FIO_O_WRONLY|FIO_O_CREAT|FIO_O_TRUNC))return -EROFS;
    } else if(flags!=FIO_O_RDONLY)return -EROFS;
    e=enter();if(e)return e;
    if(writing && writers)return leave(-EBUSY);
    int fd=iomanX_open(target,flags,mode);
    if(fd>=0){f->privdata=(void *)(uintptr_t)(fd+1);if(writing)++writers;}
    return leave(fd<0?fd:0);
}
static int bridge_close(iop_file_t *f) {
    /* Close even if removed, so the underlying filesystem releases its handle. */
    if(lock<0 || WaitSema(lock)<0)return -EIO;
    int result=iomanX_close((int)(uintptr_t)f->privdata-1);
    f->privdata=NULL;
    if((f->mode&FIO_O_RDWR)!=FIO_O_RDONLY && writers)--writers;
    if(!connected() && result>=0)result=-ENODEV;
    return leave(result);
}
static int bridge_read(iop_file_t *f,void *p,int n) {
    if(n<0)return -EINVAL;
    int e=enter();if(e)return e;
    return leave(iomanX_read((int)(uintptr_t)f->privdata-1,p,n));
}
static int bridge_write(iop_file_t *f,void *p,int n) {
    if(f->unit!=1 || (f->mode&FIO_O_RDWR)==FIO_O_RDONLY || n<0)return -EROFS;
    int e=enter();if(e)return e;
    return leave(iomanX_write((int)(uintptr_t)f->privdata-1,p,n));
}
static s64 bridge_seek64(iop_file_t *f,s64 offset,int whence) {
    if(whence<0 || whence>2)return -EINVAL;
    int e=enter();if(e)return e;
    s64 result=iomanX_lseek64((int)(uintptr_t)f->privdata-1,offset,whence);
    SignalSema(lock);return result;
}
static int bridge_seek(iop_file_t *f,int offset,int whence){return (int)bridge_seek64(f,offset,whence);}
static int bridge_stat(iop_file_t *f,const char *name,iox_stat_t *st) {
    if(!st)return -EINVAL;
    char target[NP_FILE_PATH_BYTES+8];
    int directory=f->unit==1 && name && !strcmp(name,"/ps1emu");
    int e;
    if(directory) {
        /* Route through a validated card name, then stat its actual parent.
         * The bootstrap has already verified ownership and both cards there.
         * No other directory alias or mkdir operation is admitted. */
        e=path(1,"/ps1emu/card0",target);
        if(!e)target[strlen(target)-sizeof("/card0")+1]=0;
    } else e=path(f->unit,name,target);
    if(e)return e;
    e=enter();if(e)return e;
    iox_stat_t result;memset(&result,0,sizeof(result));
    e=iomanX_getstat(target,&result);
    if(e>=0 && directory && !FIO_S_ISDIR(result.mode))e=-ENOTDIR;
    if(e>=0)memcpy(st,&result,sizeof(result));
    return leave(e);
}
static int bridge_mount(iop_file_t *f,const char *name,const char *device,int flags,void *args,int length) {
    (void)flags;(void)args;(void)length;
    if(!name || *name || !device || f->unit<0 || f->unit>1 ||
       strcmp(device,f->unit?"hdd0:PP.PSXC.FILE":"hdd0:PP.FILE"))return -EINVAL;
    return connected()?0:-ENODEV;
}
static int bridge_sync(iop_file_t *f,const char *name,int flags) {
    if(f->unit!=1 || !name || *name || flags)return -EINVAL;
    int e=enter();if(e)return e;return leave(flush());
}
static int bridge_rename(iop_file_t *f,const char *old,const char *newname) {
    char source[NP_FILE_PATH_BYTES+8],target[NP_FILE_PATH_BYTES+8],previous[NP_FILE_PATH_BYTES+16];
    if(f->unit!=1 || !old || !newname ||
       !((!strcmp(old,"/ps1emu/card0.bak") && !strcmp(newname,"/ps1emu/card0")) ||
         (!strcmp(old,"/ps1emu/card1.bak") && !strcmp(newname,"/ps1emu/card1"))))return -EACCES;
    if(path(1,old,source) || path(1,newname,target))return -EACCES;
    size_t n=strlen(target);memcpy(previous,target,n);memcpy(previous+n,".prev",6);
    int e=enter();if(e)return e;
    if(writers)return leave(-EBUSY);
    e=valid_card(source);if(e)return leave(e);
    e=valid_card(target);if(e)return leave(e);
    /* FatFs cannot replace an existing destination. Retain the last complete
     * canonical card; never delete it to emulate POSIX replacement. */
    e=valid_card(previous);
    if(!e){e=iomanX_remove(previous);if(e<0)return leave(e);}
    else if(!missing(e))return leave(e);
    e=iomanX_rename(target,previous);if(e<0)return leave(e);
    e=flush();if(!e)e=iomanX_rename(source,target);
    if(e<0) {
        int restored=iomanX_rename(previous,target);
        printf("PSXCORE file-save=replace-failed result=%d rollback=%d\n",e,restored);
        return leave(e);
    }
    ++replacements;return leave(0);
}
static int bridge_devctl(iop_file_t *f,const char *name,int cmd,void *arg,unsigned arglen,void *buffer,unsigned size) {
    (void)arg;
    if(f->unit!=1 || !name || *name || arglen)return -EINVAL;
    if((unsigned)cmd==NP_FILE_CTL_FLUSH)return bridge_sync(f,name,0);
    if((unsigned)cmd!=NP_FILE_CTL_STATUS || !buffer || size!=16)return -EINVAL;
    int e=enter();if(e)return e;
    uint32_t status[4]={1,writers,flushes,replacements};memcpy(buffer,status,sizeof(status));
    return leave(0);
}
/* Explicitly deny every operation outside the disc/card whitelist. */
static iop_device_ops_t ops={
    .init=init,.deinit=init,.format=(void *)deny,.open=bridge_open,.close=bridge_close,
    .read=bridge_read,.write=bridge_write,.lseek=bridge_seek,.ioctl=(void *)deny,
    .remove=(void *)deny,.mkdir=(void *)deny,.rmdir=(void *)deny,
    .dopen=(void *)deny,.dclose=(void *)deny,.dread=(void *)deny,
    .getstat=bridge_stat,.chstat=(void *)deny,.rename=bridge_rename,.chdir=(void *)deny,
    .sync=bridge_sync,.mount=bridge_mount,.umount=(void *)deny,.lseek64=bridge_seek64,
    .devctl=bridge_devctl,.symlink=(void *)deny,.readlink=(void *)deny,.ioctl2=(void *)deny64,
};
static iop_device_t device={"pfs",IOP_DT_FS|IOP_DT_FSEXT,1,"PSXCore file bridge",&ops};
int _start(int argc,char **argv) {
    /* Loader supplies module name followed by three bounded, NUL-separated args. */
    if(argc!=4 || strlen(argv[1])!=6 || strncmp(argv[1],"mass",4) ||
       argv[1][4]<'0' || argv[1][4]>'9' || argv[1][5]!=':' ||
       strlen(argv[2])>=sizeof(disc) || strlen(argv[3])>=sizeof(key))return MODULE_NO_RESIDENT_END;
    memcpy(root,argv[1],7);memcpy(disc,argv[2],strlen(argv[2])+1);memcpy(key,argv[3],strlen(argv[3])+1);
    char test[NP_FILE_PATH_BYTES];
    if(np_file_route(0,"/disc/disc0",disc,key,test,sizeof(test)) ||
       np_file_route(1,"/ps1emu/card0",disc,key,test,sizeof(test)))return MODULE_NO_RESIDENT_END;
    struct block_device *list[20]={0};bdm_get_bd(list,20);
    for(unsigned i=0;i<20;++i)if(list[i] && !strcmp(list[i]->name,"ata") && !list[i]->devNr && !list[i]->parNr) {
        if(drive)return MODULE_NO_RESIDENT_END;
        drive=list[i];drive_sectors=drive->sectorCount;
    }
    if(!drive || !drive->flush || drive->sectorSize!=512)return MODULE_NO_RESIDENT_END;
    iop_sema_t sema={0};sema.initial=1;sema.max=1;lock=CreateSema(&sema);
    if(lock<0)return MODULE_NO_RESIDENT_END;
    if(iomanX_AddDrv(&device)!=0){DeleteSema(lock);lock=-1;return MODULE_NO_RESIDENT_END;}
    if(np_file_native_register(&device)!=0) {
        iomanX_DelDrv("pfs");DeleteSema(lock);lock=-1;return MODULE_NO_RESIDENT_END;
    }
    bdm_RegisterCallback(storage_event);
    printf("PSXCORE file-bridge=ready root=%s disc=%s saves=%s native-ioman=1.4 flush-errors=limited\n",root,disc,key);
    return MODULE_RESIDENT_END;
}
