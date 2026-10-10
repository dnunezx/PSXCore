/* Original PSXCore direct-file validation and routing. No filesystem writes. */
#include "psxcore_file.h"
#include "psxcore_boot.h"
#include <limits.h>
#include <string.h>

int np_file_volume_id_present(const unsigned char id[16]) {
    unsigned any=0;for(unsigned i=0;i<16;++i)any|=id[i];return any!=0;
}
static int hex_digit(unsigned char c) {
    if(c>='0' && c<='9')return c-'0';
    if(c>='a' && c<='f')return c-'a'+10;
    if(c>='A' && c<='F')return c-'A'+10;
    return -1;
}
int np_file_volume_id_parse(const char *hex,unsigned char out[16]) {
    unsigned char id[16]={0};memset(out,0,16);
    if(!hex)return 0;
    for(unsigned i=0;i<16;++i) {
        int hi=hex_digit((unsigned char)hex[i*2]);if(hi<0)return 0;
        int lo=hex_digit((unsigned char)hex[i*2+1]);if(lo<0)return 0;
        id[i]=(unsigned char)(hi*16+lo);
    }
    if(hex[32] || !np_file_volume_id_present(id))return 0;
    memcpy(out,id,16);return 1;
}
void np_file_volume_id_text(const unsigned char id[16],char out[33]) {
    static const char hex[]="0123456789abcdef";
    for(unsigned i=0;i<16;++i){out[i*2]=hex[id[i]>>4];out[i*2+1]=hex[id[i]&15];}out[32]=0;
}
void np_file_volume_marker_init(NpFileVolumeMarker *m,const unsigned char id[16]) {
    unsigned char copy[16];memcpy(copy,id,16);
    memset(m,0,sizeof(*m));m->magic=NP_FILE_VOLUME_MAGIC;m->version=1;
    memcpy(m->id,copy,16);np_boot_sha256(m,24,m->sha256);
}
const char *np_file_volume_marker_check(const void *bytes,size_t size,unsigned char id[16]) {
    NpFileVolumeMarker m;unsigned char hash[32];
    if(!bytes || size!=sizeof(m)){memset(id,0,16);return "file-volume-marker-size";}
    memcpy(&m,bytes,sizeof(m));memset(id,0,16);unsigned extra=0;
    for(unsigned i=0;i<sizeof(m.reserved);++i)extra|=m.reserved[i];
    if(m.magic!=NP_FILE_VOLUME_MAGIC || m.version!=1 || extra || !np_file_volume_id_present(m.id))return "file-volume-marker-format";
    np_boot_sha256(&m,24,hash);if(memcmp(hash,m.sha256,32))return "file-volume-marker-hash";
    memcpy(id,m.id,16);return NULL;
}
const char *np_file_backend_driver(uint32_t backend) {
    switch(backend) {
    case NP_FILE_BACKEND_ATA:return "ata";
    case NP_FILE_BACKEND_USB:return "usb";
    case NP_FILE_BACKEND_MX4SIO:return "mx4sio";
    case NP_FILE_BACKEND_MMCE:return "mmce";
    default:return NULL;
    }
}
const char *np_file_endpoint_check(const NpFileEndpoint *e,int saves) {
    if(!e || !np_file_backend_driver(e->backend))return "file-endpoint-backend";
    if(e->reserved || (e->required&~NP_FILE_CAP_ALL))return "file-endpoint-reserved";
    if((e->backend==NP_FILE_BACKEND_ATA && e->device) ||
       (e->backend!=NP_FILE_BACKEND_ATA && e->device>1))return "file-endpoint-device";
    uint32_t needed=saves?NP_FILE_SAVE_CAPS:NP_FILE_DISC_CAPS;
    if((e->required&needed)!=needed)return "file-endpoint-capabilities";
    return NULL;
}
#ifndef NP_FILE_IO_ONLY
static int zero(const void *bytes,size_t size) {
    const unsigned char *p=bytes;
    while(size--)if(*p++)return 0;
    return 1;
}
#endif
static int key_valid(const char *key) {
    size_t n=strlen(key);
    if(!n || n>32)return 0;
    for(size_t i=0;i<n;++i) {
        unsigned char c=key[i];
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'))return 0;
    }
    return 1;
}
int np_file_path_valid(const char *p,size_t capacity,int directory) {
    if(!p || !capacity)return 0;
    size_t length=0;
    while(length<capacity && p[length])++length;
    if(length==capacity || length<2 || p[0]!='/')return 0;
    const char *part=p+1;
    for(const unsigned char *s=(const unsigned char *)part;;++s) {
        if(!*s || *s=='/') {
            size_t n=(const char *)s-part;
            if(!n || (n==1&&part[0]=='.') || (n==2&&part[0]=='.'&&part[1]=='.'))return 0;
            if(!*s)break;
            part=(const char *)s+1;
        } else if(*s<32 || *s==127 || *s==':' || *s=='\\' || *s=='*' || *s=='?' || *s=='"' || *s=='<' || *s=='>' || *s=='|')return 0;
    }
    return directory || np_vcd_filename(p);
}
#ifndef NP_FILE_IO_ONLY
void np_file_launch_init(NpFileLaunchRequest *r) {
    if(!r)return;
    memset(r,0,sizeof(*r));r->magic=NP_FILE_MAGIC;r->version=NP_FILE_VERSION;r->size=sizeof(*r);
    r->runtime=NP_RUNTIME_KNOWN;r->environment=NP_ENV_PS2;r->options.diagnostics=1;
    memcpy(r->driver,"ata",4);memcpy(r->runtime_root,"/POPS",6);
}
const char *np_file_launch_native(const NpFileLaunchRequest *f,NpLaunchRequest *r) {
    if(!f || !r)return "file-launch-output";
    np_launch_init(r);r->runtime=f->runtime;r->environment=f->environment;r->save_mode=f->save_mode;r->options=f->options;
    /* Only the bridge accepts these synthetic mount tokens. No APA operation. */
    memcpy(r->disc_partition,"hdd0:PP.FILE",12);
    memcpy(r->save_partition,"hdd0:PP.PSXC.FILE",17);memcpy(r->save_key,"FILE",5);
    memcpy(r->serial,f->serial,sizeof(r->serial));
    memcpy(r->disc_sha256,f->disc_sha256,32);memcpy(r->header_sha1,f->header_sha1,20);
    return np_launch_check(r,sizeof(*r),NULL);
}
const char *np_file_launch_check(const void *bytes,size_t size,NpCompatResult *resolved) {
    NpFileLaunchRequest r;NpLaunchRequest native;
    if(resolved)memset(resolved,0,sizeof(*resolved));
    if(!bytes || size!=sizeof(r))return "file-launch-size";
    memcpy(&r,bytes,sizeof(r));
    if(r.magic!=NP_FILE_MAGIC || r.version!=NP_FILE_VERSION || r.size!=sizeof(r))return "file-launch-version";
    if(r.reserved || r.reserved_tail || !zero(r.padding,sizeof(r.padding)))return "file-launch-reserved";
    if(r.save_mode!=NP_SAVE_ISOLATED)return "file-launch-shared-unimplemented";
    /* Initial release supports a single internal ATA drive; expand explicitly. */
    if(memcmp(r.driver,"ata",4) || r.device)return "file-launch-device";
    if(!np_file_path_valid(r.disc_path,sizeof(r.disc_path),0) ||
       !np_file_path_valid(r.runtime_root,sizeof(r.runtime_root),1))return "file-launch-path";
    if(!memchr(r.save_key,0,sizeof(r.save_key)) || !key_valid(r.save_key))return "file-launch-save-key";
    if(!memchr(r.serial,0,sizeof(r.serial)))return "file-launch-serial";
    if(r.image_bytes>INT32_MAX || r.image_bytes<NP_VCD_DATA_OFFSET+NP_VCD_SECTOR_BYTES ||
       (r.image_bytes-NP_VCD_DATA_OFFSET)%NP_VCD_SECTOR_BYTES)return "file-launch-image-size";
    const char *error=np_file_launch_native(&r,&native);
    return error?error:np_launch_check(&native,sizeof(native),resolved);
}
/* Copy common fields explicitly so wire layouts remain independent. */
#define COPY_FILE_FIELDS(d,s) do { \
    (d)->runtime=(s)->runtime;(d)->environment=(s)->environment; \
    (d)->save_mode=(s)->save_mode;(d)->options=(s)->options; \
    (d)->image_bytes=(s)->image_bytes; \
    memcpy((d)->disc_path,(s)->disc_path,sizeof((d)->disc_path)); \
    memcpy((d)->runtime_root,(s)->runtime_root,sizeof((d)->runtime_root)); \
    memcpy((d)->save_key,(s)->save_key,sizeof((d)->save_key)); \
    memcpy((d)->serial,(s)->serial,sizeof((d)->serial)); \
    memcpy((d)->disc_sha256,(s)->disc_sha256,32); \
    memcpy((d)->header_sha1,(s)->header_sha1,20); \
} while(0)
void np_file_launch_init_v4(NpFileLaunchRequestV4 *r) {
    if(!r)return;
    NpFileLaunchRequest old;np_file_launch_init(&old);
    memset(r,0,sizeof(*r));COPY_FILE_FIELDS(r,&old);
    r->magic=NP_FILE_MAGIC;r->version=NP_FILE_VERSION_4;r->size=sizeof(*r);
    r->disc=(NpFileEndpoint){NP_FILE_BACKEND_ATA,0,NP_FILE_DISC_CAPS,0};
    r->saves=(NpFileEndpoint){NP_FILE_BACKEND_ATA,0,NP_FILE_SAVE_CAPS,0};
}
static void project_v4(const NpFileLaunchRequestV4 *f,NpFileLaunchRequest *old) {
    np_file_launch_init(old);COPY_FILE_FIELDS(old,f);
}
const char *np_file_launch_native_v4(const NpFileLaunchRequestV4 *f,NpLaunchRequest *r) {
    if(!f || !r)return "file-launch-output";
    NpFileLaunchRequest old;project_v4(f,&old);
    return np_file_launch_native(&old,r);
}
const char *np_file_launch_check_v4(const void *bytes,size_t size,NpCompatResult *resolved) {
    NpFileLaunchRequestV4 r;
    if(resolved)memset(resolved,0,sizeof(*resolved));
    if(!bytes || size!=sizeof(r))return "file-launch-size";
    memcpy(&r,bytes,sizeof(r));
    if(r.magic!=NP_FILE_MAGIC || r.version!=NP_FILE_VERSION_4 || r.size!=sizeof(r))return "file-launch-version";
    if(r.reserved || r.reserved_tail || !zero(r.padding,sizeof(r.padding)))return "file-launch-reserved";
    const char *error=np_file_endpoint_check(&r.disc,0);
    if(!error)error=np_file_endpoint_check(&r.saves,1);
    if(error)return error;
    NpFileLaunchRequest old;project_v4(&r,&old);
    return np_file_launch_check(&old,sizeof(old),resolved);
}
const char *np_file_launch_decode(const void *bytes,size_t size,NpFileLaunchRequestV4 *out,NpCompatResult *resolved) {
    if(resolved)memset(resolved,0,sizeof(*resolved));
    if(!out)return "file-launch-output";
    memset(out,0,sizeof(*out));
    const char *error;
    NpFileLaunchRequestV4 decoded;
    if(size==sizeof(NpFileLaunchRequest)) {
        error=np_file_launch_check(bytes,size,resolved);
        if(error)return error;
        NpFileLaunchRequest old;memcpy(&old,bytes,sizeof(old));
        np_file_launch_init_v4(&decoded);COPY_FILE_FIELDS(&decoded,&old);
    } else {
        error=np_file_launch_check_v4(bytes,size,resolved);
        if(error)return error;
        memcpy(&decoded,bytes,sizeof(decoded));
    }
    *out=decoded;return NULL;
}
#undef COPY_FILE_FIELDS
void np_file_launch_init_v5(NpFileLaunchRequestV5 *r) {
    if(!r)return;
    memset(r,0,sizeof(*r));np_file_launch_init_v4(&r->launch);
    r->launch.version=NP_FILE_VERSION_5;r->launch.size=sizeof(*r);
}
const char *np_file_launch_check_v5(const void *bytes,size_t size,NpCompatResult *resolved) {
    NpFileLaunchRequestV5 r;
    if(resolved)memset(resolved,0,sizeof(*resolved));
    if(!bytes || size!=sizeof(r))return "file-launch-size";
    memcpy(&r,bytes,sizeof(r));
    if(r.launch.version!=NP_FILE_VERSION_5 || r.launch.size!=sizeof(r))return "file-launch-version";
    int shared=r.launch.save_mode==NP_SAVE_SHARED;
    /* Shared ownership is defined only for volume-bound v5 requests. Keep
     * legacy v3/v4 shared requests rejected by their original validators. */
    if(shared)r.launch.save_mode=NP_SAVE_ISOLATED;
    r.launch.version=NP_FILE_VERSION_4;r.launch.size=sizeof(r.launch);
    const char *error=np_file_launch_check_v4(&r.launch,sizeof(r.launch),resolved);if(error)return error;
    int disc=np_file_volume_id_present(r.disc_volume),saves=np_file_volume_id_present(r.save_volume);
    if(shared && (!disc || !saves)) {
        if(resolved)memset(resolved,0,sizeof(*resolved));
        return "file-shared-volume-required";
    }
    int endpoints_differ=r.launch.disc.backend!=r.launch.saves.backend || r.launch.disc.device!=r.launch.saves.device;
    if(disc!=saves || (endpoints_differ && (!disc || !memcmp(r.disc_volume,r.save_volume,16)))) {
        if(resolved)memset(resolved,0,sizeof(*resolved));
        return "file-launch-volume-identities";
    }
    return NULL;
}
const char *np_file_launch_decode_v5(const void *bytes,size_t size,NpFileLaunchRequestV5 *out,NpCompatResult *resolved) {
    if(resolved)memset(resolved,0,sizeof(*resolved));
    if(!out)return "file-launch-output";
    memset(out,0,sizeof(*out));const char *error;
    if(size==sizeof(*out)) {
        error=np_file_launch_check_v5(bytes,size,resolved);if(error)return error;
        memcpy(out,bytes,size);
    } else {
        NpFileLaunchRequestV4 old;error=np_file_launch_decode(bytes,size,&old,resolved);if(error)return error;
        if(old.disc.backend!=old.saves.backend || old.disc.device!=old.saves.device) {
            if(resolved)memset(resolved,0,sizeof(*resolved));
            return "file-launch-volume-identities";
        }
        out->launch=old;out->launch.version=NP_FILE_VERSION_5;out->launch.size=sizeof(*out);
    }
    return NULL;
}
#endif
const char *np_file_route(unsigned unit,const char *name,const char *disc,const char *key,char *out,size_t capacity) {
    if(out&&capacity)out[0]=0;
    if(!out || !capacity || !name || !disc || !key)return "file-route-input";
    const char *suffix=NULL;
    if(unit==0 && !strcmp(name,"/disc/disc0")) {
        if(!np_file_path_valid(disc,NP_FILE_PATH_BYTES,0))return "file-route-disc";
        if(strlen(disc)>=capacity)return "file-route-size";
        memcpy(out,disc,strlen(disc)+1);return NULL;
    }
    if(unit!=1 || !key_valid(key))return "file-route-path";
    if(!strcmp(name,"/ps1emu/card0"))suffix="card0";
    else if(!strcmp(name,"/ps1emu/card1"))suffix="card1";
    else if(!strcmp(name,"/ps1emu/card0.bak"))suffix="card0.bak";
    else if(!strcmp(name,"/ps1emu/card1.bak"))suffix="card1.bak";
    else return "file-route-path";
    const char prefix[]="/POPS/SAVES/";
    size_t k=strlen(key),s=strlen(suffix),n=sizeof(prefix)-1+k+1+s;
    if(n>=capacity)return "file-route-size";
    memcpy(out,prefix,sizeof(prefix)-1);memcpy(out+sizeof(prefix)-1,key,k);
    out[sizeof(prefix)-1+k]='/';memcpy(out+sizeof(prefix)+k,suffix,s+1);
    return NULL;
}
int np_file_card_header_valid(const unsigned char *p,size_t size) {
    if(!p || size<128 || p[0]!='M' || p[1]!='C')return 0;
    unsigned char checksum=0;for(unsigned i=0;i<127;++i)checksum^=p[i];
    return checksum==p[127];
}
void np_file_card_frame(unsigned frame,unsigned char p[128]) {
    memset(p,0,128);
    if(frame==0 || frame==63){p[0]='M';p[1]='C';}
    else if(frame>=1 && frame<=15){p[0]=0xa0;p[8]=p[9]=0xff;}
    else if(frame>=16 && frame<=35){p[0]=p[1]=p[2]=p[3]=p[8]=p[9]=0xff;}
    for(unsigned i=0;i<127;++i)p[127]^=p[i];
}
const char *np_file_read_span(uint64_t size,uint32_t lba,uint32_t count,uint32_t *offset,uint32_t *length,uint32_t *alignment) {
    if(!offset || !length || !alignment)return "file-read-output";
    *offset=*length=*alignment=0;
    if(!count || count>16 || size>INT32_MAX)return "file-read-count";
    uint64_t start=NP_VCD_DATA_OFFSET+(uint64_t)lba*NP_VCD_SECTOR_BYTES;
    uint64_t total=(uint64_t)count*NP_VCD_SECTOR_BYTES;
    if(start>size || total>size-start)return "file-read-bounds";
    *alignment=(uint32_t)start&511;*offset=(uint32_t)start-*alignment;
    total+=*alignment;
    uint64_t padded=total+512-(total&511);
    *length=(uint32_t)(padded<=size-*offset?padded:total);
    return NULL;
}
