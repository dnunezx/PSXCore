/* Bounded read-only descriptor tail. Caller serializes access and checks media. */
#ifndef NP_DISC_TAIL_H
#define NP_DISC_TAIL_H
#include <stdint.h>
#include <string.h>
#include <errno.h>
typedef struct {
    uint32_t physical,logical;
    unsigned valid,bytes;
    unsigned char data[512];
} NpDiscTail;
static void np_disc_tail_position(NpDiscTail *s,uint32_t position) {
    s->physical=s->logical=position;s->bytes=0;s->valid=1;
}
/* Returns 1 only when the request is satisfied without touching the backend. */
static int np_disc_tail_seek(NpDiscTail *s,int64_t offset,int whence,int64_t *result) {
    if(!s->valid || (whence!=0 && whence!=1))return 0;
    uint32_t base=whence==1?s->logical:0;
    if(offset<-(int64_t)base || offset>(int64_t)UINT32_MAX-base)return 0;
    uint32_t target=base+offset;
    if(target>s->physical || s->physical-target>s->bytes)return 0;
    s->logical=target;*result=target;return 1;
}
static int np_disc_tail_read(NpDiscTail *s,int fd,void *buffer,int n,
                             int (*read_backing)(int,void *,int)) {
    if(n<0)return -EINVAL;
    if(!s->valid)return read_backing(fd,buffer,n);
    unsigned available=s->physical-s->logical;
    if(s->logical>s->physical || available>s->bytes)return -EIO;
    unsigned prefix=available<(unsigned)n?available:(unsigned)n;
    unsigned char *out=buffer;
    if(prefix)memcpy(out,s->data+s->bytes-available,prefix);
    s->logical+=prefix;
    if(prefix==(unsigned)n)return n;
    int needed=n-(int)prefix;
    int got=read_backing(fd,out+prefix,needed);
    if(got<0) {
        /* Prefix already delivered; report that short read before the error. */
        return prefix?(int)prefix:got;
    }
    if(got>needed){s->valid=0;return -EIO;}
    if((unsigned)got>UINT32_MAX-s->physical){s->valid=0;return (int)prefix+got;}
    if(got>=512) {
        memcpy(s->data,out+prefix+got-512,512);s->bytes=512;
    } else if(got) {
        unsigned keep=s->bytes<512u-(unsigned)got?s->bytes:512u-(unsigned)got;
        memmove(s->data,s->data+s->bytes-keep,keep);
        memcpy(s->data+keep,out+prefix,got);s->bytes=keep+got;
    }
    s->physical+=(unsigned)got;s->logical=s->physical;
    return (int)prefix+got;
}
#endif
