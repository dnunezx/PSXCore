#include "psxcore_discmap.h"
#include <stdio.h>
#include <string.h>

static int nonzero(const unsigned char *p) {
    unsigned any=0;for(unsigned i=0;i<32;++i)any|=p[i];return any!=0;
}
static int entry_valid(const NpDiscMapEntry *e) {
    if(!e || !memchr(e->partition,0,sizeof(e->partition)) || !nonzero(e->sha256))return 0;
    size_t n=strlen(e->partition);
    if(n<=5 || n>37 || strncmp(e->partition,"hdd0:",5) ||
        !strncmp(e->partition,"hdd0:PP.PSXC.",13))return 0;
    const char *dot=strchr(e->partition+5,'.');if(!dot || !dot[1])return 0;
    for(size_t i=5;i<n;++i) {
        unsigned char c=e->partition[i];
        if(!((c>='A' && c<='Z') || (c>='a' && c<='z') || (c>='0' && c<='9') ||
            c=='.' || c=='_' || c=='-'))return 0;
    }
    if(!memchr(e->serial,0,sizeof(e->serial)))return 0;
    NpLaunchRequest r;np_launch_init(&r);r.environment=NP_ENV_PS2;
    memcpy(r.disc_partition,e->partition,sizeof(r.disc_partition));
    memcpy(r.serial,e->serial,sizeof(r.serial));
    memcpy(r.disc_sha256,e->sha256,32);r.header_sha1[0]=1;
    return np_launch_check(&r,sizeof(r),NULL)==NULL;
}
static const char *map_check(const NpDiscMap *map) {
    if(!map || !map->count || map->count>NP_DISC_MAP_LIMIT)return "disc-map-count";
    for(unsigned i=0;i<map->count;++i) {
        if(!entry_valid(&map->entries[i]))return "disc-map-entry";
        for(unsigned j=0;j<i;++j) {
            if(!memcmp(map->entries[i].sha256,map->entries[j].sha256,32))return "disc-map-duplicate-image";
            if(!strcmp(map->entries[i].partition,map->entries[j].partition))return "disc-map-duplicate-partition";
        }
    }
    return NULL;
}
static int hex(unsigned char c) {
    if(c>='0' && c<='9')return c-'0';
    if(c>='a' && c<='f')return c-'a'+10;
    if(c>='A' && c<='F')return c-'A'+10;
    return -1;
}
const char *np_disc_map_parse(const void *bytes,size_t size,NpDiscMap *out) {
    if(out)memset(out,0,sizeof(*out));
    if(!bytes || !out || !size || size>NP_DISC_MAP_BYTES || memchr(bytes,0,size))return "disc-map-size";
    const unsigned char *p=bytes;size_t start=0;unsigned line=0;NpDiscMap map={0};
    while(start<size) {
        size_t end=start;while(end<size && p[end]!='\n')++end;
        if(end==size)return "disc-map-newline";
        size_t n=end-start;if(n && p[start+n-1]=='\r')--n;
        if(!line++) {
            static const char header[]="PSXCORE-DISCMAP 2";
            if(n!=sizeof(header)-1 || memcmp(p+start,header,n))return "disc-map-version";
        }else {
            if(map.count==NP_DISC_MAP_LIMIT)return "disc-map-count";
            if(n<77 || n>=160 || p[start+64]!=' ')return "disc-map-line";
            NpDiscMapEntry *e=&map.entries[map.count++];
            for(unsigned i=0;i<32;++i) {
                int a=hex(p[start+2*i]),b=hex(p[start+2*i+1]);
                if(a<0 || b<0)return "disc-map-hash";
                e->sha256[i]=(unsigned char)((a<<4)|b);
            }
            char fields[160];memcpy(fields,p+start+65,n-65);fields[n-65]=0;
            char *tokens[2];tokens[0]=fields;
            for(unsigned i=1;i<2;++i) {
                char *space=strchr(tokens[i-1],' ');
                if(!space || space==tokens[i-1])return "disc-map-line";
                *space=0;tokens[i]=space+1;
            }
            if(!tokens[1][0] || strchr(tokens[1],' ') || strlen(tokens[0])>=sizeof(e->partition) ||
                strlen(tokens[1])>=sizeof(e->serial))return "disc-map-line";
            strcpy(e->partition,tokens[0]);
            if(strcmp(tokens[1],"-"))strcpy(e->serial,tokens[1]);
        }
        start=end+1;
    }
    const char *error=map_check(&map);if(error)return error;
    *out=map;return NULL;
}
const char *np_disc_map_find(const NpDiscMap *map,const unsigned char sha256[32],NpDiscMapEntry *out) {
    if(out)memset(out,0,sizeof(*out));
    if(!sha256 || !out || !nonzero(sha256))return "disc-map-identity";
    const char *error=map_check(map);if(error)return error;
    for(unsigned i=0;i<map->count;++i)if(!memcmp(map->entries[i].sha256,sha256,32)) {
        *out=map->entries[i];return NULL;
    }
    return "disc-map-unmapped";
}
const char *np_disc_map_match(const NpDiscMapEntry *entry,const NpDiscIdentity *selected,const NpDiscIdentity *prepared) {
    if(!entry_valid(entry) || !selected || !prepared || selected->bytes<262144 || prepared->bytes<262144)
        return "disc-map-identity";
    if(memcmp(entry->sha256,selected->sha256,32))return "disc-map-selection-changed";
    if(selected->bytes!=prepared->bytes || memcmp(selected->sha256,prepared->sha256,32) ||
        memcmp(selected->header_sha1,prepared->header_sha1,20))
        return "disc-map-prepared-mismatch";
    return NULL;
}
const char *np_disc_map_launch(const NpDiscMapEntry *entry,const NpDiscIdentity *identity,
    uint32_t environment,const NpCompatOptions *options,NpLaunchRequest *out) {
    if(out)memset(out,0,sizeof(*out));
    if(!out || !entry_valid(entry) || !identity || identity->bytes<262144)return "disc-map-identity";
    if(memcmp(entry->sha256,identity->sha256,32))return "disc-map-selection-changed";
    NpLaunchRequest r;np_launch_init(&r);r.environment=environment;
    if(options)r.options=*options;
    memcpy(r.disc_partition,entry->partition,sizeof(r.disc_partition));
    memcpy(r.serial,entry->serial,sizeof(r.serial));
    memcpy(r.disc_sha256,identity->sha256,32);memcpy(r.header_sha1,identity->header_sha1,20);
    const char *error=np_launch_check(&r,sizeof(r),NULL);if(error)return error;
    *out=r;return NULL;
}
const char *np_disc_map_argument(const NpDiscMapEntry *entry,char *out,size_t capacity) {
    if(out && capacity)out[0]=0;
    if(!entry_valid(entry))return "disc-map-entry";
    if(!out || capacity<=strlen(entry->partition)+strlen(":/disc/disc0"))return "disc-map-argument-size";
    snprintf(out,capacity,"%s:/disc/disc0",entry->partition);return NULL;
}
