/* Original POPS pfs-path adapter. Selected VCD is always read-only.
 * Uses the SDK filesystem rather than copied POPS/IOPCD implementation. */
#ifdef NP_HOST_TEST
#include "filebridge_stub.h"
#else
#include "irx_imports.h"
#endif
#include "psxcore_file.h"
#include "native.h"
#include "worker.h"
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#ifndef NP_HOST_TEST
#include <usbhdfsd-common.h>
#endif
IRX_ID("psxcore_file",1,0);
static char root[8],save_root[8],disc[NP_FILE_PATH_BYTES],cards[2][NP_FILE_PATH_BYTES];
static unsigned char disc_volume[16],save_volume[16];
static char expected_save_driver[8];static unsigned expected_save_device;
static int lock=-1;
static unsigned writers,flushes,replacements;
/* A failed save write/close cannot later become a successful replacement. */
static int save_failed;
typedef struct {
    struct block_device *drive;
    u64 sectors,offset;
    char driver[8];unsigned device;
} DiskBinding;
static DiskBinding bindings[2];
static volatile int removed;
/* BDM's callback has no device identity. Conservatively latch every unmount;
 * reconnection requires a fresh launch and bounded image verification. */
static void storage_event(int mounted){if(!mounted)removed=1;}

/* Never reuse an unrelated disk after removal. Filesystem handles additionally
 * reject a disconnected/reconnected volume. Bind using the selected file's
 * metadata, never mass enumeration or the first physical drive. */
static int connected(void) {
    if(removed)return 0;
    struct block_device *list[20]={0};
    bdm_get_bd(list,20);
    for(unsigned unit=0;unit<2;++unit) {
        DiskBinding *b=&bindings[unit];int found=0;
        for(unsigned i=0;i<20;++i) {
            struct block_device *d=list[i];
            if(d && d==b->drive && d->name && !strcmp(d->name,b->driver) && d->devNr==b->device &&
               !d->parNr && d->sectorOffset==b->offset && d->sectorCount==b->sectors && d->sectorSize==512){found=1;break;}
        }
        if(!found)return 0;
    }
    return 1;
}
static int enter(void) {
    if(lock<0 || WaitSema(lock)<0)return -EIO;
    if(!connected()){SignalSema(lock);return -ENODEV;}
    return 0;
}
static int leave(int result){SignalSema(lock);return result;}
static int path(unsigned unit,const char *name,char out[NP_FILE_PATH_BYTES+8]) {
    char relative[NP_FILE_PATH_BYTES];
    if(np_file_route(unit,name,disc,cards,relative,sizeof(relative)))return -EACCES;
    const char *selected=unit==1?save_root:root;
    size_t r=strlen(selected),n=strlen(relative);
    if(r+n>=NP_FILE_PATH_BYTES+8)return -ENAMETOOLONG;
    memcpy(out,selected,r);memcpy(out+r,relative,n+1);return 0;
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
    struct block_device *drive=bindings[1].drive;
    if(writers || save_failed || !connected() || !drive->flush)return -EIO;
    drive->flush(drive);
    if(!connected())return -ENODEV;
    ++flushes;return 0;
}
static int init(iop_device_t *d){(void)d;return 0;}
static int deny(void){return -EACCES;}
static int deny64(void){return -EACCES;}
/* Track backing write capability per descriptor, independently of requested
 * native mode: POPS initializes existing canonical cards with O_RDWR but only
 * reads them. The backing handle remains read-only; saves use .bak instead. */
static int backing_fd(const iop_file_t *f){return (int)((uintptr_t)f->privdata>>1)-1;}
static int writable_handle(const iop_file_t *f){return ((uintptr_t)f->privdata&1)!=0;}
static int execute_bridge_open(iop_file_t *f,const char *name,int flags,int mode) {
    char target[NP_FILE_PATH_BYTES+8];int e=path(f->unit,name,target);
    if(e)return e;
    int writing=(flags&FIO_O_RDWR)!=FIO_O_RDONLY;
    int canonical=f->unit==1 && (!strcmp(name,"/ps1emu/card0") || !strcmp(name,"/ps1emu/card1"));
    if(canonical && flags==FIO_O_RDWR) {
        writing=0;flags=FIO_O_RDONLY;
    } else if(writing) {
        if(f->unit!=1 || (strcmp(name,"/ps1emu/card0.bak") && strcmp(name,"/ps1emu/card1.bak")) ||
           flags!=(FIO_O_WRONLY|FIO_O_CREAT|FIO_O_TRUNC))return -EROFS;
    } else if(flags!=FIO_O_RDONLY)return -EROFS;
    e=enter();if(e)return e;
    if(writing && save_failed)return leave(-EIO);
    if(writing && writers)return leave(-EBUSY);
    int fd=iomanX_open(target,flags,mode);
    if(fd>=0) {
        if(fd>INT_MAX/2-1){iomanX_close(fd);return leave(-EOVERFLOW);}
        f->privdata=(void *)(((uintptr_t)(fd+1)<<1)|(unsigned)writing);
        if(writing)++writers;
    }
    return leave(fd<0?fd:0);
}
static int execute_bridge_close(iop_file_t *f) {
    /* Close even if removed, so the underlying filesystem releases its handle. */
    if(lock<0 || WaitSema(lock)<0)return -EIO;
    int writing=writable_handle(f);
    int result=iomanX_close(backing_fd(f));
    f->privdata=NULL;
    if(writing && writers)--writers;
    if(writing && result<0)save_failed=1;
    if(!connected() && result>=0)result=-ENODEV;
    return leave(result);
}
static int execute_bridge_read(iop_file_t *f,void *p,int n) {
    if(n<0)return -EINVAL;
    int e=enter();if(e)return e;
    return leave(iomanX_read(backing_fd(f),p,n));
}
static int execute_bridge_write(iop_file_t *f,void *p,int n) {
    if(f->unit!=1 || !writable_handle(f) || n<0)return -EROFS;
    int e=enter();if(e)return e;
    if(save_failed)return leave(-EIO);
    int result=iomanX_write(backing_fd(f),p,n);
    if(result!=n)save_failed=1;
    if((n && !result) || result>n)result=-EIO;
    return leave(result);
}
static s64 execute_bridge_seek64(iop_file_t *f,s64 offset,int whence) {
    if(whence<0 || whence>2)return -EINVAL;
    int e=enter();if(e)return e;
    s64 result=iomanX_lseek64(backing_fd(f),offset,whence);
    SignalSema(lock);return result;
}
static int execute_bridge_seek(iop_file_t *f,int offset,int whence){return (int)execute_bridge_seek64(f,offset,whence);}
static int execute_bridge_stat(iop_file_t *f,const char *name,iox_stat_t *st) {
    if(!st)return -EINVAL;
    char target[NP_FILE_PATH_BYTES+8];
    int directory=f->unit==1 && name && !strcmp(name,"/ps1emu");
    int e;
    if(directory) {
        /* Route through a validated card name, then stat its actual parent.
         * The bootstrap has already verified both selected cards.
         * No other directory alias or mkdir operation is admitted. */
        e=path(1,"/ps1emu/card0",target);
        if(!e) {
            size_t n=strlen(target);
            while(n && target[n-1]!='/')--n;
            target[n-1]=0;
        }
    } else e=path(f->unit,name,target);
    if(e)return e;
    e=enter();if(e)return e;
    iox_stat_t result;memset(&result,0,sizeof(result));
    e=iomanX_getstat(target,&result);
    if(e>=0 && directory && !FIO_S_ISDIR(result.mode))e=-ENOTDIR;
    if(e>=0)memcpy(st,&result,sizeof(result));
    return leave(e);
}
static int execute_bridge_mount(iop_file_t *f,const char *name,const char *device,int flags,void *args,int length) {
    (void)flags;(void)args;(void)length;
    if(!name || *name || !device || f->unit<0 || f->unit>1 ||
       strcmp(device,f->unit?NP_SONY_SAVE_PARTITION:"hdd0:PP.FILE"))return -EINVAL;
    return connected()?0:-ENODEV;
}
static int execute_bridge_sync(iop_file_t *f,const char *name,int flags) {
    if(f->unit!=1 || !name || *name || flags)return -EINVAL;
    int e=enter();if(e)return e;return leave(flush());
}
static int execute_bridge_rename(iop_file_t *f,const char *old,const char *newname) {
    char source[NP_FILE_PATH_BYTES+8],target[NP_FILE_PATH_BYTES+8],previous[NP_FILE_PATH_BYTES+16];
    if(f->unit!=1 || !old || !newname ||
       !((!strcmp(old,"/ps1emu/card0.bak") && !strcmp(newname,"/ps1emu/card0")) ||
         (!strcmp(old,"/ps1emu/card1.bak") && !strcmp(newname,"/ps1emu/card1"))))return -EACCES;
    if(path(1,old,source) || path(1,newname,target))return -EACCES;
    size_t n=strlen(target);memcpy(previous,target,n);memcpy(previous+n,".prev",6);
    int e=enter();if(e)return e;
    if(writers)return leave(-EBUSY);
    if(save_failed)return leave(-EIO);
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
static int execute_bridge_devctl(iop_file_t *f,const char *name,int cmd,void *arg,unsigned arglen,void *buffer,unsigned size) {
    (void)arg;
    if(f->unit!=1 || !name || *name || arglen)return -EINVAL;
    if((unsigned)cmd==NP_FILE_CTL_FLUSH)return execute_bridge_sync(f,name,0);
    if((unsigned)cmd!=NP_FILE_CTL_STATUS || !buffer || size!=16)return -EINVAL;
    int e=enter();if(e)return e;
    uint32_t status[4]={save_failed?0u:1u,writers,flushes,replacements};memcpy(buffer,status,sizeof(status));
    return leave(0);
}
/* Native IOMAN remains responsible for IOPCD's imports. Queue both published
 * device ABIs; reaching the private SDK registry must not consume a
 * native caller's small stack in path routing or backing filesystem work. */
static int bind_device(const char *expected_driver,unsigned expected_device);
int64_t np_file_sdk_execute(const NpFileWork *r) {
    iop_file_t *f=r->file;
    switch((unsigned)r->operation&~NP_FILE_SDK) {
    case NP_FILE_OPEN:return execute_bridge_open(f,r->name,r->flags,r->mode);
    case NP_FILE_CLOSE:return execute_bridge_close(f);
    case NP_FILE_READ:return execute_bridge_read(f,r->buffer,r->count);
    case NP_FILE_WRITE:return execute_bridge_write(f,r->buffer,r->count);
    case NP_FILE_SEEK:return execute_bridge_seek(f,(int)r->offset,r->flags);
    case NP_FILE_SEEK64:return execute_bridge_seek64(f,r->offset,r->flags);
    case NP_FILE_STAT:return execute_bridge_stat(f,r->name,r->buffer);
    case NP_FILE_MOUNT:return execute_bridge_mount(f,r->name,r->other,r->flags,r->arg,r->count);
    case NP_FILE_SYNC:return execute_bridge_sync(f,r->name,r->flags);
    case NP_FILE_RENAME:return execute_bridge_rename(f,r->name,r->other);
    case NP_FILE_DEVCTL:return execute_bridge_devctl(f,r->name,r->flags,r->arg,r->arg_size,r->buffer,r->buffer_size);
    case NP_FILE_BIND:return bind_device(r->name,(unsigned)r->flags);
    default:return -EINVAL;
    }
}
static int bridge_open(iop_file_t *f,const char *name,int flags,int mode) {
    NpFileWork r={.operation=NP_FILE_SDK|NP_FILE_OPEN,.file=f,.name=name,.flags=flags,.mode=mode};
    return (int)np_file_worker_call(&r);
}
static int bridge_close(iop_file_t *f) {
    NpFileWork r={.operation=NP_FILE_SDK|NP_FILE_CLOSE,.file=f};return (int)np_file_worker_call(&r);
}
static int bridge_read(iop_file_t *f,void *p,int n) {
    NpFileWork r={.operation=NP_FILE_SDK|NP_FILE_READ,.file=f,.buffer=p,.count=n};
    return (int)np_file_worker_call(&r);
}
static int bridge_write(iop_file_t *f,void *p,int n) {
    NpFileWork r={.operation=NP_FILE_SDK|NP_FILE_WRITE,.file=f,.buffer=p,.count=n};
    return (int)np_file_worker_call(&r);
}
static int bridge_seek(iop_file_t *f,int offset,int whence) {
    NpFileWork r={.operation=NP_FILE_SDK|NP_FILE_SEEK,.file=f,.offset=offset,.flags=whence};
    return (int)np_file_worker_call(&r);
}
static s64 bridge_seek64(iop_file_t *f,s64 offset,int whence) {
    NpFileWork r={.operation=NP_FILE_SDK|NP_FILE_SEEK64,.file=f,.offset=offset,.flags=whence};
    return np_file_worker_call(&r);
}
static int bridge_stat(iop_file_t *f,const char *name,iox_stat_t *out) {
    NpFileWork r={.operation=NP_FILE_SDK|NP_FILE_STAT,.file=f,.name=name,.buffer=out};
    return (int)np_file_worker_call(&r);
}
static int bridge_mount(iop_file_t *f,const char *name,const char *dev,int flags,void *args,int n) {
    NpFileWork r={.operation=NP_FILE_SDK|NP_FILE_MOUNT,.file=f,.name=name,.other=dev,.flags=flags,.arg=args,.count=n};
    return (int)np_file_worker_call(&r);
}
static int bridge_sync(iop_file_t *f,const char *name,int flags) {
    NpFileWork r={.operation=NP_FILE_SDK|NP_FILE_SYNC,.file=f,.name=name,.flags=flags};
    return (int)np_file_worker_call(&r);
}
static int bridge_rename(iop_file_t *f,const char *old,const char *newname) {
    NpFileWork r={.operation=NP_FILE_SDK|NP_FILE_RENAME,.file=f,.name=old,.other=newname};
    return (int)np_file_worker_call(&r);
}
static int bridge_devctl(iop_file_t *f,const char *name,int cmd,void *arg,unsigned n,void *buf,unsigned size) {
    NpFileWork r={.operation=NP_FILE_SDK|NP_FILE_DEVCTL,.file=f,.name=name,.flags=cmd,
        .arg=arg,.arg_size=n,.buffer=buf,.buffer_size=size};
    return (int)np_file_worker_call(&r);
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
static int reject_start(int registered) {
    /* Keep code resident if a live device/thread could still reference it.
     * Return 2 so the EE loader rejects this incomplete startup. */
    if(registered && iomanX_DelDrv("pfs")<0)return MODULE_REMOVABLE_END;
    if(np_file_worker_stop()<0)return MODULE_REMOVABLE_END;
    if(lock>=0){DeleteSema(lock);lock=-1;}
    return MODULE_NO_RESIDENT_END;
}
/* Startup filesystem work also belongs on the dedicated worker stack. */
static int bind_path(const char *target,const char *expected_driver,unsigned expected_device,DiskBinding *out) {
    /* The native POPSLoader ATA BDM names its block device "mass" while the
     * frontend/backend contract remains "ata". USB continues to report usb. */
    const char *native_driver=!strcmp(expected_driver,"ata")?"mass":expected_driver;
    int fd=iomanX_open(target,FIO_O_RDONLY,0);
    if(fd<0)return fd;
    char actual_driver[8]={0};unsigned actual_device=~0u;u64 lba=0;
    int identity=iomanX_ioctl2(fd,USBMASS_IOCTL_GET_DRIVERNAME,NULL,0,actual_driver,sizeof(actual_driver));
    if(identity>=0)identity=iomanX_ioctl2(fd,USBMASS_IOCTL_GET_DEVICE_NUMBER,NULL,0,&actual_device,sizeof(actual_device));
    if(identity>=0)identity=iomanX_ioctl2(fd,USBMASS_IOCTL_GET_LBA,NULL,0,&lba,sizeof(lba));
    int closed=iomanX_close(fd);
    if(identity<0 || closed<0 || !memchr(actual_driver,0,sizeof(actual_driver)) ||
       strcmp(actual_driver,native_driver) || actual_device!=expected_device)return -ENODEV;
    memset(out,0,sizeof(*out));
    struct block_device *list[20]={0};bdm_get_bd(list,20);
    for(unsigned i=0;i<20;++i)if(list[i] && list[i]->name && !strcmp(list[i]->name,actual_driver) &&
        list[i]->devNr==actual_device && !list[i]->parNr && lba>=list[i]->sectorOffset &&
        lba-list[i]->sectorOffset<list[i]->sectorCount) {
        if(out->drive)return -ENODEV;
        out->drive=list[i];out->sectors=list[i]->sectorCount;
    }
    struct block_device *drive=out->drive;
    if(!drive || !drive->flush || drive->sectorSize!=512)return -ENODEV;
    out->offset=drive->sectorOffset;memcpy(out->driver,actual_driver,sizeof(out->driver));out->device=actual_device;
    return 0;
}
static int marker_matches(const char *selected,const unsigned char expected[16]) {
    if(!np_file_volume_id_present(expected))return 0;
    char target[64];size_t n=strlen(selected);
    memcpy(target,selected,n);memcpy(target+n,NP_FILE_VOLUME_MARKER,sizeof(NP_FILE_VOLUME_MARKER));
    iox_stat_t st;if(iomanX_getstat(target,&st)<0 || !FIO_S_ISREG(st.mode) || st.hisize || st.size!=sizeof(NpFileVolumeMarker))return -ENODEV;
    int fd=iomanX_open(target,FIO_O_RDONLY,0);if(fd<0)return fd;
    NpFileVolumeMarker marker;unsigned char actual[16],extra;
    int nread=iomanX_read(fd,&marker,sizeof(marker));
    int trailing=nread==sizeof(marker)?iomanX_read(fd,&extra,1):-1;
    int closed=iomanX_close(fd);
    if(nread!=sizeof(marker) || trailing || closed<0 || np_file_volume_marker_check(&marker,sizeof(marker),actual) || memcmp(actual,expected,16))return -ENODEV;
    return 0;
}
static int bind_device(const char *expected_driver,unsigned expected_device) {
    if(marker_matches(root,disc_volume) || marker_matches(save_root,save_volume))return -ENODEV;
    char target[NP_FILE_PATH_BYTES+8];size_t rn=strlen(root),dn=strlen(disc);
    memcpy(target,root,rn);memcpy(target+rn,disc,dn+1);
    int e=bind_path(target,expected_driver,expected_device,&bindings[0]);if(e)return e;
    if(!strcmp(root,save_root))bindings[1]=bindings[0];
    else {
        e=path(1,"/ps1emu/card0",target);if(e)return e;
        e=bind_path(target,expected_save_driver,expected_save_device,&bindings[1]);if(e)return e;
    }
    removed=0;save_failed=0;return 0;
}
static int root_valid(const char *selected) {
    return strlen(selected)==6 && !strncmp(selected,"mass",4) && selected[4]>='0' && selected[4]<='9' && selected[5]==':';
}
int _start(int argc,char **argv) {
    /* Only the frontend-selected shared pair contract is supported. */
    if(argc!=12)return MODULE_NO_RESIDENT_END;
    for(unsigned i=1;i<12;++i)if(!argv[i])return MODULE_NO_RESIDENT_END;
    if(!root_valid(argv[1]) || !root_valid(argv[7]) || strlen(argv[2])>=sizeof(disc) ||
       !np_file_card_path_valid(argv[3]) || !np_file_card_path_valid(argv[4]))return MODULE_NO_RESIDENT_END;
    memcpy(root,argv[1],7);strcpy(disc,argv[2]);strcpy(cards[0],argv[3]);strcpy(cards[1],argv[4]);
    char test[NP_FILE_PATH_BYTES];
    if(np_file_route(0,"/disc/disc0",disc,cards,test,sizeof(test)) ||
       np_file_route(1,"/ps1emu/card0",disc,cards,test,sizeof(test)) ||
       np_file_route(1,"/ps1emu/card1",disc,cards,test,sizeof(test)))return MODULE_NO_RESIDENT_END;
    const char *expected_driver=argv[5];
    if((strcmp(expected_driver,"ata") && strcmp(expected_driver,"usb")) ||
       (argv[6][0]!='0' && argv[6][0]!='1') || argv[6][1])return MODULE_NO_RESIDENT_END;
    unsigned expected_device=(unsigned)(argv[6][0]-'0');
    if(!strcmp(expected_driver,"ata") && expected_device)return MODULE_NO_RESIDENT_END;
    if((strcmp(argv[8],"ata") && strcmp(argv[8],"usb")) ||
       (argv[9][0]!='0' && argv[9][0]!='1') || argv[9][1])return MODULE_NO_RESIDENT_END;
    memcpy(save_root,argv[7],7);strcpy(expected_save_driver,argv[8]);expected_save_device=(unsigned)(argv[9][0]-'0');
    if(!strcmp(expected_save_driver,"ata") && expected_save_device)return MODULE_NO_RESIDENT_END;
    memset(disc_volume,0,16);memset(save_volume,0,16);memset(bindings,0,sizeof(bindings));
    if(strcmp(argv[10],"-") || strcmp(argv[11],"-")) {
        if(!np_file_volume_id_parse(argv[10],disc_volume) || !np_file_volume_id_parse(argv[11],save_volume))return MODULE_NO_RESIDENT_END;
    }
    int same=!strcmp(root,save_root);
    if(same && (strcmp(expected_driver,expected_save_driver) || expected_device!=expected_save_device || memcmp(disc_volume,save_volume,16)))return MODULE_NO_RESIDENT_END;
    if(!same && (!np_file_volume_id_present(disc_volume) || !np_file_volume_id_present(save_volume) || !memcmp(disc_volume,save_volume,16)))return MODULE_NO_RESIDENT_END;
    iop_sema_t sema={0};sema.initial=1;sema.max=1;lock=CreateSema(&sema);
    if(lock<0)return MODULE_NO_RESIDENT_END;
    if(np_file_worker_start()<0)return reject_start(0);
    NpFileWork binding={.operation=NP_FILE_SDK|NP_FILE_BIND,.file=&device,.name=expected_driver,.flags=(int)expected_device};
    if(np_file_worker_call(&binding)<0)return reject_start(0);
    if(iomanX_AddDrv(&device)!=0)return reject_start(0);
    if(np_file_native_register(&device)!=0) {
        return reject_start(1);
    }
    bdm_RegisterCallback(storage_event);
    printf("PSXCORE file-bridge=ready root=%s disc=%s saves-root=%s card0=%s card1=%s native-ioman=1.4 flush-errors=limited\n",root,disc,save_root,cards[0],cards[1]);
    return MODULE_RESIDENT_END;
}
