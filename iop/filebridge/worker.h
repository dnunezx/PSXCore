/* Synchronous borrowed requests. The caller retains all buffers until done. */
#ifndef PSXCORE_FILE_WORKER_H
#define PSXCORE_FILE_WORKER_H
#include <stdint.h>
#include "psxcore_nativegate.h"
typedef struct NpNativeFile NpNativeFile;
enum NpFileOperation {
    NP_FILE_OPEN,NP_FILE_CLOSE,NP_FILE_READ,NP_FILE_WRITE,NP_FILE_SEEK,
    NP_FILE_SEEK64,NP_FILE_STAT,NP_FILE_MOUNT,NP_FILE_SYNC,NP_FILE_RENAME,NP_FILE_DEVCTL,
    NP_FILE_BIND,
    NP_FILE_SDK=0x100,
};
typedef struct {
    enum NpFileOperation operation;
    void *file; /* Typed by the operation's native/SDK ABI; never reinterpreted. */
    const char *name,*other;
    void *buffer,*arg;
    int64_t offset,result;
    int flags,mode,count;
    unsigned arg_size,buffer_size;
} NpFileWork;
#ifdef _IOP
_Static_assert(sizeof(NpFileWork)<=64,"bounded native caller request");
#endif
int np_file_worker_start(void);
/* Only before either device becomes reachable; never cancel active requests. */
int np_file_worker_stop(void);
int64_t np_file_worker_call(NpFileWork *request);
int64_t np_file_native_execute(const NpFileWork *request);
int64_t np_file_sdk_execute(const NpFileWork *request);
#endif
