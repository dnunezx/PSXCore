/* Original explicit file-disc group validation and read-only path routing. */
#include "psxcore_discset.h"
#include "psxcore_boot.h"
#include "psxcore_cdtoc.h"
#include <limits.h>
#include <string.h>
static int nonzero(const unsigned char *p,size_t n) {
    unsigned any=0;for(size_t i=0;i<n;++i)any|=p[i];return any!=0;
}
void np_file_disc_save_binding(const char (*cards)[NP_FILE_PATH_BYTES],uint32_t backend,
    uint32_t device,const unsigned char volume[16],unsigned char out[32]) {
    struct {char cards[2][NP_FILE_PATH_BYTES];uint32_t backend,device;unsigned char volume[16];} b;
    memset(&b,0,sizeof(b));
    for(unsigned i=0;i<2;++i) {
        size_t n=0;while(n<NP_FILE_PATH_BYTES && cards[i][n])++n;
        if(n<NP_FILE_PATH_BYTES)memcpy(b.cards[i],cards[i],n);
    }
    b.backend=backend;b.device=device;memcpy(b.volume,volume,16);np_boot_sha256(&b,sizeof(b),out);
}
const char *np_file_disc_set_check(const NpFileDiscSet *s,size_t bytes,
    const char *initial,const unsigned char volume[16],const unsigned char binding[32]) {
    if(!s || !initial || !volume || !binding || bytes!=sizeof(*s))return "disc-set-input";
    if(s->magic!=NP_DISC_SET_MAGIC || s->version!=1 || s->size!=sizeof(*s) ||
       !s->count || s->count>NP_DISC_SET_LIMIT || s->initial>=s->count)return "disc-set-format";
    if(memcmp(s->disc_volume,volume,16) || memcmp(s->save_binding,binding,32) ||
       !nonzero(binding,32))return "disc-set-binding";
    for(unsigned i=0;i<NP_DISC_SET_LIMIT;++i) {
        const NpFileDiscEntry *e=&s->entries[i];
        if(i>=s->count) {
            if(nonzero((const unsigned char *)e,sizeof(*e)))return "disc-set-unused";
            continue;
        }
        if(!np_file_path_valid(e->path,sizeof(e->path),0))return "disc-set-path";
        size_t n=strlen(e->path);
        for(size_t j=n+1;j<sizeof(e->path);++j)if(e->path[j])return "disc-set-path-padding";
        if(!e->sectors || !e->tracks || e->tracks>99 || e->image_bytes>INT32_MAX ||
           e->image_bytes!=NP_VCD_DATA_OFFSET+(uint64_t)e->sectors*2352 ||
           !nonzero(e->header_sha256,32) || !nonzero(e->toc_sha256,32))return "disc-set-identity";
        for(unsigned j=0;j<i;++j) {
            const unsigned char *a=(const unsigned char *)e->path,*b=(const unsigned char *)s->entries[j].path;
            size_t k=0;while(a[k] && b[k]) {
                unsigned x=a[k],y=b[k];if(x>='A' && x<='Z')x+=32;if(y>='A' && y<='Z')y+=32;
                if(x!=y)break;
                ++k;
            }
            if(!a[k] && !b[k])return "disc-set-duplicate";
        }
    }
    return strcmp(s->entries[s->initial].path,initial)?"disc-set-initial":NULL;
}
int np_file_disc_slot(const char *name,unsigned *slot) {
    if(!name || !slot || strncmp(name,"/disc/disc",10) || name[10]<'0' || name[10]>'3' || name[11])return -1;
    *slot=(unsigned)(name[10]-'0');return 0;
}
const NpFileDiscEntry *np_file_disc_entry(const NpFileDiscSet *s,unsigned slot) {
    if(!s || !s->count || s->count>4 || s->initial>=s->count || slot>=s->count)return NULL;
    return &s->entries[(s->initial+slot)%s->count];
}
const char *np_file_disc_set_route(const NpFileDiscSet *s,unsigned unit,const char *name,
    const char cards[2][NP_FILE_PATH_BYTES],char *out,size_t capacity) {
    if(out && capacity)out[0]=0;
    if(!s)return "disc-set-route-input";
    const NpFileDiscEntry *e=np_file_disc_entry(s,0);
    if(!e)return "disc-set-route-format";
    if(unit==1)return np_file_route(unit,name,e->path,cards,out,capacity);
    unsigned slot;
    if(unit || np_file_disc_slot(name,&slot) || !(e=np_file_disc_entry(s,slot)))return "disc-set-route-slot";
    return np_file_route(0,"/disc/disc0",e->path,cards,out,capacity);
}
const char *np_file_disc_entry_check(const NpFileDiscEntry *e,const unsigned char *h,size_t n) {
    if(!e || !h || n!=NP_FILE_HEADER_HASH_BYTES)return "disc-set-header-size";
    unsigned char hash[32];np_boot_sha256(h,n,hash);
    if(memcmp(hash,e->header_sha256,32))return "disc-set-header-hash";
    np_boot_sha256(h,1024,hash);
    if(memcmp(hash,e->toc_sha256,32))return "disc-set-toc-hash";
    NpVcdInfo info;const char *error=np_vcd_check(h,n,e->image_bytes,&info);
    uint32_t last,tracks;
    if(error)return error;
    if(info.sectors!=e->sectors || info.tracks!=e->tracks ||
       np_cd_toc_geometry(h,1024,e->sectors,&last,&tracks) || tracks!=e->tracks)return "disc-set-toc-geometry";
    return NULL;
}
