/* Register every mass/typed device in the private SDK manager. Never send an
 * SDK device or descriptor to POPS's native IOMAN 1.4 registry. */
#include <iomanX.h>
#include "psxcore_volume.h"
static int volume_adddrv(iop_device_t *device);
#define AddDrv volume_adddrv
#define DelDrv iomanX_DelDrv
#include "fs_driver.c"

static unsigned char identity_sector[512] __attribute__((aligned(64)));
static int volume_ioctl(iop_file_t *fd,int cmd,void *data,unsigned int datalen,void *rdata,unsigned int rdatalen) {
    if((unsigned)cmd!=NP_VOLUME_IOCTL)return fs_ioctl2(fd,cmd,data,datalen,rdata,rdatalen);
    if(!rdata || rdatalen!=sizeof(NpVolumeIdentity) || !fd || !fd->privdata)return -EINVAL;
    memset(rdata,0,rdatalen);
    FIL *file=fd->privdata;
    _fs_lock();
    struct block_device *bd=file->obj.fs?fatfs_fs_driver_get_mounted_bd_from_index(file->obj.fs->pdrv):NULL;
    int result=-ENODEV;
    if(bd && bd->read && bd->sectorSize==512 && bd->read(bd,0,identity_sector,1)==1)
        result=np_volume_identity_build(rdata,identity_sector,bd->sectorOffset,bd->sectorCount,bd->sectorSize);
    _fs_unlock();return result;
}
static int volume_adddrv(iop_device_t *device) {
    /* Both mass and typed aliases share the SDK operation table. */
    device->ops->ioctl2=volume_ioctl;
    return iomanX_AddDrv(device);
}
