/* Original SHA-1 operations for the existing native header verification job. */
#include "psxcore_hash.h"
#include <string.h>
static uint32_t rol(uint32_t x,unsigned n) { return (x<<n)|(x>>(32-n)); }
static void block(uint32_t state[5],const unsigned char p[64]) {
    uint32_t w[80];
    for(unsigned i=0;i<16;++i)w[i]=((uint32_t)p[4*i]<<24)|((uint32_t)p[4*i+1]<<16)|
        ((uint32_t)p[4*i+2]<<8)|p[4*i+3];
    for(unsigned i=16;i<80;++i)w[i]=rol(w[i-3]^w[i-8]^w[i-14]^w[i-16],1);
    uint32_t a=state[0],b=state[1],c=state[2],d=state[3],e=state[4];
    for(unsigned i=0;i<80;++i) {
        uint32_t f,k;
        if(i<20) { f=(b&c)|(~b&d);k=0x5a827999; }
        else if(i<40) { f=b^c^d;k=0x6ed9eba1; }
        else if(i<60) { f=(b&c)|(b&d)|(c&d);k=0x8f1bbcdc; }
        else { f=b^c^d;k=0xca62c1d6; }
        uint32_t next=rol(a,5)+f+e+k+w[i];e=d;d=c;c=rol(b,30);b=a;a=next;
    }
    state[0]+=a;state[1]+=b;state[2]+=c;state[3]+=d;state[4]+=e;
}
void np_sha1_init(NpSha1 *hash) {
    static const uint32_t initial[5]={0x67452301,0xefcdab89,0x98badcfe,0x10325476,0xc3d2e1f0};
    memset(hash,0,sizeof(*hash));memcpy(hash->state,initial,sizeof(initial));
}
void np_sha1_update(NpSha1 *hash,const void *bytes,size_t size) {
    const unsigned char *p=bytes;hash->bytes+=(uint64_t)size;
    while(size) {
        if(!hash->used && size>=64) { block(hash->state,p);p+=64;size-=64;continue; }
        size_t n=64-hash->used;if(n>size)n=size;
        memcpy(hash->buffer+hash->used,p,n);hash->used+=n;p+=n;size-=n;
        if(hash->used==64) { block(hash->state,hash->buffer);hash->used=0; }
    }
}
void np_sha1_final(NpSha1 *hash,unsigned char digest[20]) {
    uint64_t bits=hash->bytes*8;unsigned char tail[128]={0};
    memcpy(tail,hash->buffer,hash->used);tail[hash->used]=0x80;
    unsigned end=hash->used<56?64:128;
    for(unsigned i=0;i<8;++i)tail[end-1-i]=(unsigned char)(bits>>(8*i));
    block(hash->state,tail);if(end==128)block(hash->state,tail+64);
    for(unsigned i=0;i<20;++i)digest[i]=(unsigned char)(hash->state[i/4]>>(24-8*(i%4)));
    memset(hash,0,sizeof(*hash));
}
