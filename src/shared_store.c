#include "psxcore_shared_store.h"
#include "psxcore_file.h"
#include "psxcore_hash.h"
#ifdef NP_HOST_TEST
#include "file_storage_stub.h"
#else
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>
#endif
#include <stdio.h>
#include <string.h>

static int absent(int e){return e==-2 || e==-4;}
static int path(char out[256],const char *base,const char *name) {
    return !base || snprintf(out,256,"%s/%s",base,name)>=256;
}
static int missing(const char *base,const char *name) {
    char p[256];iox_stat_t st;return !path(p,base,name) && absent(fileXioGetStat(p,&st));
}
static int read_all(int fd,void *b,unsigned size) {
    unsigned done=0;
    while(done<size){int n=fileXioRead(fd,(unsigned char *)b+done,size-done);if(n<=0 || (unsigned)n>size-done)return -1;done+=(unsigned)n;}
    return 0;
}
static int write_all(int fd,const void *b,unsigned size) {
    unsigned done=0;
    while(done<size){int n=fileXioWrite(fd,(const unsigned char *)b+done,size-done);if(n<=0 || (unsigned)n>size-done)return -1;done+=(unsigned)n;}
    return 0;
}
static int card_hash(const char *base,const char *name,unsigned char hash[32]) {
    char p[256];iox_stat_t st;
    if(path(p,base,name) || fileXioGetStat(p,&st)<0 || !FIO_S_ISREG(st.mode) || st.hisize || st.size!=NP_FILE_CARD_BYTES)return -1;
    int fd=fileXioOpen(p,FIO_O_RDONLY,0);if(fd<0)return -1;
    unsigned char b[4096];NpSha256 h;np_sha256_init(&h);int e=0;
    for(unsigned i=0;i<NP_FILE_CARD_BYTES/sizeof(b) && !e;++i) {
        e=read_all(fd,b,sizeof(b));
        if(!e && !i && !np_file_card_header_valid(b,sizeof(b)))e=-1;
        if(!e)np_sha256_update(&h,b,sizeof(b));
    }
    if(!e && fileXioRead(fd,b,1)!=0)e=-1;
    if(fileXioClose(fd)<0)e=-1;
    if(!e)np_sha256_final(&h,hash);
    return e;
}
static const char *recovery(const char *base) {
    const char *names[]={"shared.set.new","shared.set.prev","shared.set.bak"};
    for(unsigned i=0;i<3;++i)if(!missing(base,names[i]))return "shared-set-recovery-required";
    if(!missing(base,"owner.sha256") || !missing(base,"owner.sha256.new"))return "shared-set-private-conflict";
    for(unsigned slot=0;slot<2;++slot) {
        char name[24];const char *suffix[]={".new",".bak"};
        for(unsigned j=0;j<2;++j){snprintf(name,sizeof(name),"card%u%s",slot,suffix[j]);if(!missing(base,name))return "shared-card-recovery-required";}
        snprintf(name,sizeof(name),"card%u.prev",slot);unsigned char hash[32];
        if(!missing(base,name) && card_hash(base,name,hash))return "shared-card-previous-invalid";
    }
    return NULL;
}
static const char *read_set(const char *base,const char *name,NpSharedSet *s) {
    char p[256];iox_stat_t st;memset(s,0,sizeof(*s));
    if(path(p,base,name) || fileXioGetStat(p,&st)<0 || !FIO_S_ISREG(st.mode) || st.hisize || st.size!=sizeof(*s))return "shared-set-stat";
    int fd=fileXioOpen(p,FIO_O_RDONLY,0);if(fd<0)return "shared-set-open";
    unsigned char extra;int e=read_all(fd,s,sizeof(*s));if(!e && fileXioRead(fd,&extra,1)!=0)e=-1;
    int closed=fileXioClose(fd);if(e || closed<0){memset(s,0,sizeof(*s));return "shared-set-read";}
    const char *error=np_shared_check(s,sizeof(*s));if(error)memset(s,0,sizeof(*s));return error;
}
const char *np_shared_store_read(const char *base,NpSharedSet *s) {
    if(!s)return "shared-set-output";
    memset(s,0,sizeof(*s));const char *error=recovery(base);
    if(!error)error=read_set(base,"shared.set",s);
    for(unsigned slot=0;slot<2 && !error;++slot) {
        char name[16];unsigned char hash[32];snprintf(name,sizeof(name),"card%u",slot);
        if(card_hash(base,name,hash))error="shared-card-invalid";
    }
    if(error)memset(s,0,sizeof(*s));
    return error;
}
static int stage_set(const char *base,const NpSharedSet *s) {
    char p[256];if(!missing(base,"shared.set.new") || path(p,base,"shared.set.new"))return -1;
    int fd=fileXioOpen(p,FIO_O_WRONLY|FIO_O_CREAT,0666);if(fd<0)return -1;
    int e=write_all(fd,s,sizeof(*s));if(fileXioClose(fd)<0)e=-1;
    NpSharedSet check;
    return e || read_set(base,"shared.set.new",&check) || memcmp(s,&check,sizeof(*s))?-1:0;
}
static int rename_file(const char *base,const char *old,const char *next) {
    char a[256],b[256];return path(a,base,old) || path(b,base,next) || fileXioRename(a,b)<0?-1:0;
}
const char *np_shared_store_create(const char *base,const NpSharedSet *s,const char *source) {
    const char *error=np_shared_check(s,sizeof(*s));if(!error)error=recovery(base);if(error)return error;
    if(!missing(base,"shared.set"))return "shared-set-exists";
    unsigned char original[2][32];
    for(unsigned slot=0;slot<2;++slot) {
        char name[16];snprintf(name,sizeof(name),"card%u",slot);
        if(!missing(base,name))return "shared-card-exists";
        snprintf(name,sizeof(name),"card%u.prev",slot);if(!missing(base,name))return "shared-card-exists";
        if(source) {
            snprintf(name,sizeof(name),"card%u",slot);
            if(card_hash(source,name,original[slot]))return "shared-copy-source-invalid";
            snprintf(name,sizeof(name),"card%u.new",slot);if(!missing(source,name))return "shared-copy-source-recovery";
            snprintf(name,sizeof(name),"card%u.bak",slot);if(!missing(source,name))return "shared-copy-source-recovery";
        }
    }
    for(unsigned slot=0;slot<2;++slot) {
        char name[20],p[256],source_path[256];snprintf(name,sizeof(name),"card%u.new",slot);
        if(path(p,base,name))return "shared-card-path";
        int in=-1;if(source){snprintf(name,sizeof(name),"card%u",slot);if(path(source_path,source,name))return "shared-card-path";in=fileXioOpen(source_path,FIO_O_RDONLY,0);if(in<0)return "shared-copy-source-open";}
        int out=fileXioOpen(p,FIO_O_WRONLY|FIO_O_CREAT,0666);
        if(out<0){if(in>=0)fileXioClose(in);return "shared-card-create";}
        int e=0;unsigned char b[4096];
        for(unsigned i=0;i<NP_FILE_CARD_BYTES/sizeof(b) && !e;++i) {
            if(in>=0)e=read_all(in,b,sizeof(b));
            else for(unsigned j=0;j<sizeof(b)/128;++j)np_file_card_frame(i*(sizeof(b)/128)+j,b+j*128);
            if(!e)e=write_all(out,b,sizeof(b));
        }
        if(in>=0 && fileXioClose(in)<0)e=-1;
        if(fileXioClose(out)<0)e=-1;
        unsigned char copied[32];snprintf(name,sizeof(name),"card%u.new",slot);
        if(e || card_hash(base,name,copied) || (source && memcmp(copied,original[slot],32)))return "shared-card-verify";
        char final[16];snprintf(final,sizeof(final),"card%u",slot);
        if(rename_file(base,name,final))return "shared-card-commit";
    }
    if(stage_set(base,s) || rename_file(base,"shared.set.new","shared.set"))return "shared-set-commit";
    NpSharedSet check;return np_shared_store_read(base,&check);
}
const char *np_shared_store_update(const char *base,const NpSharedSet *expected,const NpSharedSet *next) {
    const char *error=np_shared_check(next,sizeof(*next));if(error)return error;
    NpSharedSet current;error=np_shared_store_read(base,&current);if(error)return error;
    if(!expected || memcmp(expected,&current,sizeof(current)))return "shared-set-changed";
    if(current.storage!=next->storage || strcmp(current.key,next->key) || memcmp(current.volume,next->volume,16))return "shared-set-identity";
    if(!memcmp(&current,next,sizeof(current)))return NULL;
    if(stage_set(base,next) || rename_file(base,"shared.set","shared.set.prev") ||
       rename_file(base,"shared.set.new","shared.set"))return "shared-set-commit";
    /* Retain the previous manifest until the committed file is verified. */
    error=read_set(base,"shared.set",&current);
    if(error || memcmp(&current,next,sizeof(current)))return "shared-set-commit";
    char p[256];if(path(p,base,"shared.set.prev") || fileXioRemove(p)<0)return "shared-set-recovery-required";
    return np_shared_store_read(base,&current);
}
