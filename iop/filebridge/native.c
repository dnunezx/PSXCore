/* Original driver adapter: register in native ioman, retain the SDK bridge's
 * whitelist, locking, descriptor ownership and save replacement policy.
 * No library hooks, export replacement or changes to native services. */
#ifdef NP_HOST_TEST
#include "filebridge_stub.h"
#else
#include "irx_imports.h"
#endif
#include "native.h"
#include "worker.h"
#include <errno.h>
#include <string.h>

static iop_device_t *backing;
/* Both extended ABIs use FIO flags/mode bits. Do not apply legacy ioman 1.1
 * mode conversion here. Translate the objects field by field, never cast. */
static iop_file_t sdk_file(const NpNativeFile *f) {
    iop_file_t result={.mode=f->mode,.unit=f->unit,.device=backing,.privdata=f->privdata};
    return result;
}
static int native_init(NpNativeDevice *d){(void)d;return 0;}
static int native_deinit(NpNativeDevice *d){(void)d;return -EACCES;}
static int execute_open(NpNativeFile *f,const char *name,int flags,int mode) {
    iop_file_t sdk=sdk_file(f);sdk.mode=flags;sdk.privdata=NULL;
    int e=backing->ops->open(&sdk,name,flags,mode);
    f->privdata=e<0?NULL:sdk.privdata;
    return e;
}
static int execute_close(NpNativeFile *f) {
    iop_file_t sdk=sdk_file(f);
    int e=backing->ops->close(&sdk);f->privdata=sdk.privdata;return e;
}
static int execute_read(NpNativeFile *f,void *p,int n) {
    iop_file_t sdk=sdk_file(f);return backing->ops->read(&sdk,p,n);
}
static int execute_write(NpNativeFile *f,void *p,int n) {
    iop_file_t sdk=sdk_file(f);return backing->ops->write(&sdk,p,n);
}
static int execute_seek(NpNativeFile *f,int offset,int whence) {
    iop_file_t sdk=sdk_file(f);return backing->ops->lseek(&sdk,offset,whence);
}
static int64_t execute_seek64(NpNativeFile *f,int64_t offset,int whence) {
    iop_file_t sdk=sdk_file(f);return backing->ops->lseek64(&sdk,offset,whence);
}
static int execute_stat(NpNativeFile *f,const char *name,NpNativeStat *out) {
    if(!out)return -EINVAL;
    iop_file_t sdk=sdk_file(f);iox_stat_t st;
    memset(&st,0,sizeof(st));
    int e=backing->ops->getstat(&sdk,name,&st);if(e<0)return e;
    NpNativeStat result={0};
    result.mode=st.mode;result.attr=st.attr;result.size=st.size;result.hisize=st.hisize;
    memcpy(result.ctime,st.ctime,8);memcpy(result.atime,st.atime,8);memcpy(result.mtime,st.mtime,8);
    result.private_data[0]=st.private_0;result.private_data[1]=st.private_1;
    result.private_data[2]=st.private_2;result.private_data[3]=st.private_3;
    result.private_data[4]=st.private_4;result.private_data[5]=st.private_5;
    memcpy(out,&result,sizeof(result));return e;
}
static int execute_mount(NpNativeFile *f,const char *name,const char *dev,int flags,void *args,int n) {
    iop_file_t sdk=sdk_file(f);return backing->ops->mount(&sdk,name,dev,flags,args,n);
}
static int execute_sync(NpNativeFile *f,const char *name,int flags) {
    iop_file_t sdk=sdk_file(f);return backing->ops->sync(&sdk,name,flags);
}
static int execute_rename(NpNativeFile *f,const char *old,const char *newname) {
    iop_file_t sdk=sdk_file(f);return backing->ops->rename(&sdk,old,newname);
}
static int execute_devctl(NpNativeFile *f,const char *name,int cmd,void *arg,unsigned n,void *buf,unsigned size) {
    iop_file_t sdk=sdk_file(f);return backing->ops->devctl(&sdk,name,cmd,arg,n,buf,size);
}
/* All ABI translation and SDK callbacks below execute on the worker stack.
 * Native callers retain only this bounded request and the wait transport. */
int64_t np_file_native_execute(const NpFileWork *r) {
    switch(r->operation) {
    case NP_FILE_OPEN:return execute_open(r->file,r->name,r->flags,r->mode);
    case NP_FILE_CLOSE:return execute_close(r->file);
    case NP_FILE_READ:return execute_read(r->file,r->buffer,r->count);
    case NP_FILE_WRITE:return execute_write(r->file,r->buffer,r->count);
    case NP_FILE_SEEK:return execute_seek(r->file,(int)r->offset,r->flags);
    case NP_FILE_SEEK64:return execute_seek64(r->file,r->offset,r->flags);
    case NP_FILE_STAT:return execute_stat(r->file,r->name,r->buffer);
    case NP_FILE_MOUNT:return execute_mount(r->file,r->name,r->other,r->flags,r->arg,r->count);
    case NP_FILE_SYNC:return execute_sync(r->file,r->name,r->flags);
    case NP_FILE_RENAME:return execute_rename(r->file,r->name,r->other);
    case NP_FILE_DEVCTL:return execute_devctl(r->file,r->name,r->flags,r->arg,r->arg_size,r->buffer,r->buffer_size);
    default:return -EINVAL;
    }
}
static int native_open(NpNativeFile *f,const char *name,int flags,int mode) {
    NpFileWork r={.operation=NP_FILE_OPEN,.file=f,.name=name,.flags=flags,.mode=mode};
    return (int)np_file_worker_call(&r);
}
static int native_close(NpNativeFile *f) {
    NpFileWork r={.operation=NP_FILE_CLOSE,.file=f};return (int)np_file_worker_call(&r);
}
static int native_read(NpNativeFile *f,void *p,int n) {
    NpFileWork r={.operation=NP_FILE_READ,.file=f,.buffer=p,.count=n};
    return (int)np_file_worker_call(&r);
}
static int native_write(NpNativeFile *f,void *p,int n) {
    NpFileWork r={.operation=NP_FILE_WRITE,.file=f,.buffer=p,.count=n};
    return (int)np_file_worker_call(&r);
}
static int native_seek(NpNativeFile *f,int offset,int whence) {
    NpFileWork r={.operation=NP_FILE_SEEK,.file=f,.offset=offset,.flags=whence};
    return (int)np_file_worker_call(&r);
}
static int64_t native_seek64(NpNativeFile *f,int64_t offset,int whence) {
    NpFileWork r={.operation=NP_FILE_SEEK64,.file=f,.offset=offset,.flags=whence};
    return np_file_worker_call(&r);
}
static int native_stat(NpNativeFile *f,const char *name,NpNativeStat *out) {
    NpFileWork r={.operation=NP_FILE_STAT,.file=f,.name=name,.buffer=out};
    return (int)np_file_worker_call(&r);
}
static int native_mount(NpNativeFile *f,const char *name,const char *dev,int flags,void *args,int n) {
    NpFileWork r={.operation=NP_FILE_MOUNT,.file=f,.name=name,.other=dev,.flags=flags,.arg=args,.count=n};
    return (int)np_file_worker_call(&r);
}
static int native_sync(NpNativeFile *f,const char *name,int flags) {
    NpFileWork r={.operation=NP_FILE_SYNC,.file=f,.name=name,.flags=flags};
    return (int)np_file_worker_call(&r);
}
static int native_rename(NpNativeFile *f,const char *old,const char *newname) {
    NpFileWork r={.operation=NP_FILE_RENAME,.file=f,.name=old,.other=newname};
    return (int)np_file_worker_call(&r);
}
static int native_devctl(NpNativeFile *f,const char *name,int cmd,void *arg,unsigned n,void *buf,unsigned size) {
    NpFileWork r={.operation=NP_FILE_DEVCTL,.file=f,.name=name,.flags=cmd,
        .arg=arg,.arg_size=n,.buffer=buf,.buffer_size=size};
    return (int)np_file_worker_call(&r);
}
/* Unsupported callbacks have their actual ABI signatures, including 64-bit
 * returns where applicable; no function-pointer casts or uninitialized slots. */
static int deny_format(NpNativeFile *f,const char *name,const char *dev,void *arg,int n) {
    (void)f;(void)name;(void)dev;(void)arg;(void)n;return -EACCES;
}
static int deny_ioctl(NpNativeFile *f,int cmd,void *arg){(void)f;(void)cmd;(void)arg;return -EACCES;}
static int deny_path(NpNativeFile *f,const char *name){(void)f;(void)name;return -EACCES;}
static int deny_mkdir(NpNativeFile *f,const char *name,int mode){(void)f;(void)name;(void)mode;return -EACCES;}
static int deny_close(NpNativeFile *f){(void)f;return -EACCES;}
static int deny_dread(NpNativeFile *f,NpNativeDirent *dir){(void)f;(void)dir;return -EACCES;}
static int deny_chstat(NpNativeFile *f,const char *name,NpNativeStat *st,unsigned mask) {
    (void)f;(void)name;(void)st;(void)mask;return -EACCES;
}
static int deny_symlink(NpNativeFile *f,const char *old,const char *newname) {
    (void)f;(void)old;(void)newname;return -EACCES;
}
static int deny_readlink(NpNativeFile *f,const char *name,char *buf,unsigned n) {
    (void)f;(void)name;(void)buf;(void)n;return -EACCES;
}
static int deny_ioctl2(NpNativeFile *f,int cmd,void *arg,unsigned n,void *buf,unsigned size) {
    (void)f;(void)cmd;(void)arg;(void)n;(void)buf;(void)size;return -EACCES;
}
static NpNativeOps native_ops={
    .init=native_init,.deinit=native_deinit,.format=deny_format,.open=native_open,.close=native_close,
    .read=native_read,.write=native_write,.lseek=native_seek,.ioctl=deny_ioctl,
    .remove=deny_path,.mkdir=deny_mkdir,.rmdir=deny_path,.dopen=deny_path,.dclose=deny_close,.dread=deny_dread,
    .getstat=native_stat,.chstat=deny_chstat,.rename=native_rename,.chdir=deny_path,
    .sync=native_sync,.mount=native_mount,.umount=deny_path,.lseek64=native_seek64,.devctl=native_devctl,
    .symlink=deny_symlink,.readlink=deny_readlink,.ioctl2=deny_ioctl2,
};
static NpNativeDevice native_device={"pfs",IOP_DT_FS|IOP_DT_FSEXT,1,"PSXCore native file bridge",&native_ops};
int np_file_native_register(iop_device_t *bridge) {
    backing=bridge;
    int e=np_file_native_adddrv(&native_device);
    if(e)backing=NULL;
    return e;
}
