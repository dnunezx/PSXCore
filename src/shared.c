#include "psxcore_shared.h"
#include "psxcore_boot.h"
#include <string.h>

static int any(const unsigned char *p,size_t n) {
    while(n--)if(*p++)return 1;
    return 0;
}
int np_shared_key_valid(const char *key,uint32_t storage) {
    if(!key || (storage!=NP_SHARED_NATIVE && storage!=NP_SHARED_FILE))return 0;
    size_t limit=storage==NP_SHARED_NATIVE?20:32,n=0;
    for(;n<=limit && key[n];++n) {
        unsigned char c=(unsigned char)key[n];
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'))return 0;
    }
    return n && n<=limit;
}
static int text(const char *p,size_t size) {
    size_t i=0;
    for(;i<size && p[i];++i)if((unsigned char)p[i]<32 || (unsigned char)p[i]>126)return 0;
    if(!i || i==size)return 0;
    for(;i<size;++i)if(p[i])return 0;
    return 1;
}
static void seal(NpSharedSet *s) {
    np_boot_sha256(s,offsetof(NpSharedSet,sha256),s->sha256);
}
const char *np_shared_check(const void *bytes,size_t size) {
    if(!bytes || size!=sizeof(NpSharedSet))return "shared-set-size";
    NpSharedSet s;memcpy(&s,bytes,sizeof(s));
    if(s.magic!=NP_SHARED_MAGIC || s.version!=NP_SHARED_VERSION || s.size!=sizeof(s))return "shared-set-version";
    if(s.count>NP_SHARED_MEMBERS || !text(s.key,sizeof(s.key)) ||
       !np_shared_key_valid(s.key,s.storage) || !text(s.name,sizeof(s.name)))return "shared-set-format";
    if((s.storage==NP_SHARED_FILE)!=any(s.volume,sizeof(s.volume)))return "shared-set-volume";
    for(unsigned i=0;i<s.count;++i) {
        if(!any(s.members[i],32))return "shared-set-member";
        for(unsigned j=0;j<i;++j)if(!memcmp(s.members[i],s.members[j],32))return "shared-set-duplicate";
    }
    if(any((const unsigned char *)&s.members[s.count],(NP_SHARED_MEMBERS-s.count)*32))return "shared-set-padding";
    unsigned char hash[32];np_boot_sha256(&s,offsetof(NpSharedSet,sha256),hash);
    return memcmp(hash,s.sha256,32)?"shared-set-hash":NULL;
}
const char *np_shared_init(NpSharedSet *s,uint32_t storage,const char *key,
    const char *name,const unsigned char volume[16]) {
    if(!s)return "shared-set-output";
    if(!np_shared_key_valid(key,storage) || !name || !name[0] || strlen(name)>=sizeof(s->name))return "shared-set-format";
    NpSharedSet next={0};next.magic=NP_SHARED_MAGIC;next.version=NP_SHARED_VERSION;next.size=sizeof(next);next.storage=storage;
    strcpy(next.key,key);strcpy(next.name,name);if(volume)memcpy(next.volume,volume,16);seal(&next);
    const char *error=np_shared_check(&next,sizeof(next));if(!error)*s=next;return error;
}
const char *np_shared_match(const NpSharedSet *s,uint32_t storage,const char *key,
    const unsigned char volume[16],const unsigned char disc[32]) {
    const char *error=np_shared_check(s,sizeof(*s));if(error)return error;
    if(!key || !disc || s->storage!=storage || strcmp(s->key,key))return "shared-set-identity";
    unsigned char zero[16]={0};if(memcmp(s->volume,volume?volume:zero,16))return "shared-set-volume-mismatch";
    for(unsigned i=0;i<s->count;++i)if(!memcmp(s->members[i],disc,32))return NULL;
    return "shared-set-not-enrolled";
}
const char *np_shared_member(NpSharedSet *s,const unsigned char disc[32],int enroll) {
    const char *error=np_shared_check(s,sizeof(*s));if(error)return error;
    if(!disc || !any(disc,32))return "shared-set-member";
    unsigned i=0;while(i<s->count && memcmp(s->members[i],disc,32))++i;
    if(enroll) {
        if(i<s->count)return NULL;
        if(s->count==NP_SHARED_MEMBERS)return "shared-set-full";
        memcpy(s->members[s->count++],disc,32);
    } else {
        if(i==s->count)return NULL;
        memmove(s->members+i,s->members+i+1,(s->count-i-1)*32);memset(s->members+--s->count,0,32);
    }
    seal(s);return NULL;
}
