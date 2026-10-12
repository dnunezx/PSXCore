/* Bootstrap/private-bridge access to POPStarter's native mass filesystem.
 * No native table hooks, driver replacement, disk fingerprints or RPC server.
 * Each private descriptor owns a separate native descriptor. */
#ifdef NP_HOST_TEST
#include "massproxy_stub.h"
#else
#include "irx_imports.h"
#endif
#include "psxcore_file.h"
#include <errno.h>
#include <limits.h>
#include <stdint.h>
IRX_ID("psxcore_massproxy",1,0);
static int handle(const iop_file_t *f){return (int)(uintptr_t)f->privdata-1;}
static int target(const iop_file_t *f,const char *name,char out[NP_FILE_PATH_BYTES+8]) {
 if(!f || !name || f->unit<0 || f->unit>9 || strlen(name)>=NP_FILE_PATH_BYTES)return -EINVAL;
 sprintf(out,"mass%d:%s",f->unit,name);return 0;
}
static int init(iop_device_t *d){(void)d;return 0;}
static int deny(void){return -EACCES;}
static int open_file(iop_file_t *f,const char *name,int flags,int mode) {
 char path[NP_FILE_PATH_BYTES+8];int e=target(f,name,path);if(e)return e;
 int fd=np_mass_open(path,flags,mode);f->privdata=fd<0?NULL:(void *)(uintptr_t)(fd+1);
 return fd<0?fd:0;
}
static int close_file(iop_file_t *f){int e=np_mass_close(handle(f));f->privdata=NULL;return e;}
static int read_file(iop_file_t *f,void *p,int n){return np_mass_read(handle(f),p,n);}
static int write_file(iop_file_t *f,void *p,int n){return np_mass_write(handle(f),p,n);}
static int seek_file(iop_file_t *f,int off,int whence){return np_mass_seek(handle(f),off,whence);}
/* Supported VCD/card sizes are <2 GiB; native mass driver supplies seek32. */
static int64_t seek64_file(iop_file_t *f,int64_t off,int whence) {
 if(off<INT_MIN || off>INT_MAX)return -EOVERFLOW;
 return np_mass_seek(handle(f),(int)off,whence);
}
static int dopen_file(iop_file_t *f,const char *name) {
 char path[NP_FILE_PATH_BYTES+8];int e=target(f,name,path);if(e)return e;
 int fd=np_mass_dopen(path);f->privdata=fd<0?NULL:(void *)(uintptr_t)(fd+1);return fd<0?fd:0;
}
static int dclose_file(iop_file_t *f){int e=np_mass_dclose(handle(f));f->privdata=NULL;return e;}
static int stat_file(iop_file_t *f,const char *name,iox_stat_t *out) {
 char path[NP_FILE_PATH_BYTES+8];int e=target(f,name,path);if(e)return e;
 if(!out)return -EINVAL;
 NpNativeStat st={0};e=np_mass_stat(path,&st);if(e<0)return e;
 memset(out,0,sizeof(*out));out->mode=st.mode;out->attr=st.attr;out->size=st.size;out->hisize=st.hisize;
 memcpy(out->ctime,st.ctime,8);memcpy(out->atime,st.atime,8);memcpy(out->mtime,st.mtime,8);
 out->private_0=st.private_data[0];out->private_1=st.private_data[1];out->private_2=st.private_data[2];
 out->private_3=st.private_data[3];out->private_4=st.private_data[4];out->private_5=st.private_data[5];return e;
}
static int remove_file(iop_file_t *f,const char *name) {
 char path[NP_FILE_PATH_BYTES+8];int e=target(f,name,path);return e?e:np_mass_remove(path);
}
static int rename_file(iop_file_t *f,const char *old,const char *newname) {
 char from[NP_FILE_PATH_BYTES+8],to[NP_FILE_PATH_BYTES+8];int e=target(f,old,from);
 if(!e)e=target(f,newname,to);
 return e?e:np_mass_rename(from,to);
}
static int ioctl2_file(iop_file_t *f,int cmd,void *in,unsigned n,void *out,unsigned size) {
 return np_mass_ioctl2(handle(f),cmd,in,n,out,size);
}
static iop_device_ops_t ops={
 .init=init,.deinit=init,.format=(void *)deny,.open=open_file,.close=close_file,
 .read=read_file,.write=write_file,.lseek=seek_file,.ioctl=(void *)deny,
 .remove=remove_file,.mkdir=(void *)deny,.rmdir=(void *)deny,.dopen=dopen_file,
 .dclose=dclose_file,.dread=(void *)deny,.getstat=stat_file,.chstat=(void *)deny,
 .rename=rename_file,.chdir=(void *)deny,.sync=(void *)deny,.mount=(void *)deny,
 .umount=(void *)deny,.lseek64=seek64_file,.devctl=(void *)deny,.symlink=(void *)deny,
 .readlink=(void *)deny,.ioctl2=ioctl2_file,
};
static iop_device_t device={"mass",IOP_DT_FS|IOP_DT_FSEXT,1,"PSXCore native mass access",&ops};
int _start(int argc,char **argv) {
 (void)argc;(void)argv;
 int e=iomanX_AddDrv(&device);
 printf("PSXCORE mass-access=native registry=private result=%d\n",e);
 return e?MODULE_NO_RESIDENT_END:MODULE_RESIDENT_END;
}
