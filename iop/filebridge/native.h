/* Native IOP ioman 1.4 driver ABI. This is not SDK ioman 1.1's legacy ABI.
 * In particular, open/mkdir take modes and getstat uses the extended 64-byte
 * stat format. Keep these types separate from the SDK's aligned iomanX types. */
#ifndef PSXCORE_FILE_NATIVE_H
#define PSXCORE_FILE_NATIVE_H
#include <stdint.h>
#include <stddef.h>
#include "psxcore_nativegate.h"

typedef struct NpNativeFile {
    int mode,unit;
    NpNativeDevice *device;
    void *privdata;
} NpNativeFile;
typedef struct {
    uint32_t mode,attr,size;
    unsigned char ctime[8],atime[8],mtime[8];
    uint32_t hisize,private_data[6];
} NpNativeStat;
typedef struct {
    NpNativeStat stat;
    char name[256];
    void *privdata;
} NpNativeDirent;
typedef struct {
    int (*init)(NpNativeDevice *),(*deinit)(NpNativeDevice *);
    int (*format)(NpNativeFile *,const char *,const char *,void *,int);
    int (*open)(NpNativeFile *,const char *,int,int),(*close)(NpNativeFile *);
    int (*read)(NpNativeFile *,void *,int),(*write)(NpNativeFile *,void *,int);
    int (*lseek)(NpNativeFile *,int,int),(*ioctl)(NpNativeFile *,int,void *);
    int (*remove)(NpNativeFile *,const char *),(*mkdir)(NpNativeFile *,const char *,int);
    int (*rmdir)(NpNativeFile *,const char *),(*dopen)(NpNativeFile *,const char *);
    int (*dclose)(NpNativeFile *),(*dread)(NpNativeFile *,NpNativeDirent *);
    int (*getstat)(NpNativeFile *,const char *,NpNativeStat *);
    int (*chstat)(NpNativeFile *,const char *,NpNativeStat *,unsigned);
    int (*rename)(NpNativeFile *,const char *,const char *),(*chdir)(NpNativeFile *,const char *);
    int (*sync)(NpNativeFile *,const char *,int);
    int (*mount)(NpNativeFile *,const char *,const char *,int,void *,int);
    int (*umount)(NpNativeFile *,const char *);
    int64_t (*lseek64)(NpNativeFile *,int64_t,int);
    int (*devctl)(NpNativeFile *,const char *,int,void *,unsigned,void *,unsigned);
    int (*symlink)(NpNativeFile *,const char *,const char *);
    int (*readlink)(NpNativeFile *,const char *,char *,unsigned);
    int (*ioctl2)(NpNativeFile *,int,void *,unsigned,void *,unsigned);
} NpNativeOps;
struct NpNativeDevice {
    const char *name;
    unsigned type,version;
    const char *desc;
    NpNativeOps *ops;
};
_Static_assert(sizeof(NpNativeStat)==64,"native stat ABI");
_Static_assert(offsetof(NpNativeStat,hisize)==36,"native stat size offset");
#ifdef _IOP
_Static_assert(sizeof(NpNativeFile)==16,"native file ABI");
_Static_assert(sizeof(NpNativeDevice)==20,"native device ABI");
_Static_assert(offsetof(NpNativeOps,mount)==80,"native mount slot");
_Static_assert(offsetof(NpNativeOps,lseek64)==88,"native seek64 slot");
_Static_assert(sizeof(NpNativeOps)==108,"native operations ABI");
#endif
int np_file_native_register(iop_device_t *bridge);
#endif
