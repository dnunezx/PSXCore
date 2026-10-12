#include "psxcore_volume.h"
#include "psxcore_boot.h"
#include <string.h>
int np_volume_identity_valid(const NpVolumeIdentity *id) {
    unsigned bits=0;for(unsigned i=0;i<32;++i)bits|=id->boot_sha256[i];
    return id->magic==NP_VOLUME_MAGIC && id->version==1 && id->size==sizeof(*id) &&
        id->sector_size==512 && id->sectors && id->offset<=UINT64_MAX-id->sectors && !id->reserved && bits;
}
int np_volume_identity_build(NpVolumeIdentity *out,unsigned char sector[512],
    uint64_t offset,uint64_t sectors,uint32_t sector_size) {
    memset(out,0,sizeof(*out));
    if(sector_size!=512 || !sectors || offset>UINT64_MAX-sectors || sector[510]!=0x55 || sector[511]!=0xaa)return -1;
    if(!memcmp(sector+3,"EXFAT   ",8)) {
        /* FatFs may update volume dirty flags and percent-used on sync. */
        sector[106]=sector[107]=sector[112]=0;
    } else {
        if(sector[11]!=0 || sector[12]!=2)return -1;
        /* Reserved FAT extended-BPB status byte; volume serial is retained. */
        if(!memcmp(sector+82,"FAT32   ",8))sector[65]=0;
        else sector[37]=0;
    }
    out->magic=NP_VOLUME_MAGIC;out->version=1;out->size=sizeof(*out);
    out->offset=offset;out->sectors=sectors;out->sector_size=sector_size;
    np_boot_sha256(sector,512,out->boot_sha256);return 0;
}
