/* Standalone backend entry points, separate from any frontend. */
#ifndef PSXCORE_FILE_STORAGE_H
#define PSXCORE_FILE_STORAGE_H
#include "psxcore_file.h"
int np_file_storage_init(int reset);
int np_file_storage_init_pops(void);
const char *np_file_prepare(int argc,char **argv,NpFileLaunchRequest *out);
const char *np_file_reopen(const NpFileLaunchRequest *request,int provision);
const char *np_file_bridge_start(const NpFileLaunchRequest *request);
const char *np_file_runtime_path(const NpFileLaunchRequest *request,const char *name,char *out,size_t size);
void np_file_storage_exit(void);
int np_file_read_asset(const NpFileLaunchRequest *request,const char *name,void *target,
    unsigned bytes,const unsigned char hash[32]);
extern int np_file_probe_only;
#endif
