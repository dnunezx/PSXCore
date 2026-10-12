/* Preserve a code address, never an import stub or a mutable export slot.
 * Load only after the verified POPS IOP reset, before the private SDK manager.
 * No guest libraries or firmware bytes are changed by this module. */
#ifdef NP_HOST_TEST
#include "nativegate_stub.h"
#else
#include "irx_imports.h"
#endif
#include "psxcore_nativegate.h"
#include <errno.h>
#include <stdint.h>
IRX_ID("psxcore_nativegate",1,0);
extern struct irx_export_table _exp_psxnat;
static int (*native_adddrv)(NpNativeDevice *);

/* IOMAN from the checksum-verified IOPRP252 image: IO/File_Manager 2.3,
 * library ioman 1.4, text=0x1e60, data=0x1f0, bss=0x290.
 * These first six AddDrv instructions contain no relocations or GP accesses.
 * The inspected native module uses absolute relocated addresses, not GP.
 * The EE loader separately verifies the complete supplied firmware hash. */
static const uint32_t adddrv_head[]={
    0x27bdffd8,0xafb1001c,0x00808821,0x27a40010,0xafbf0024,0xafb20020,
};
static int capture(void) {
    iop_library_t request={.version=0x104,.name="ioman"};
    const uint32_t *entries=QueryLibraryEntryTable(&request);
    lc_internals_t *core=GetLoadcoreInternalData();
    if(!entries || !core)return -ENOENT;
    /* A bound also rejects corrupt/cyclic image lists without hanging boot. */
    ModuleInfo_t *owner=core->image_info;
    unsigned visited=0;
    for(;owner && visited<256;owner=owner->next,++visited) {
        if(!owner->name || strcmp(owner->name,"IO/File_Manager"))continue;
        uintptr_t start=owner->text_start;
        if(owner->version!=0x203 || owner->text_size!=0x1e60 ||
           owner->data_size!=0x1f0 || owner->bss_size!=0x290 ||
           (start&3) || !start || start>UINTPTR_MAX-0x22e0)return -EINVAL;
#ifdef _IOP
        if(start+0x22e0>0x200000)return -EINVAL;
#endif
        /* Validate ownership/extent before reading the queried table. The
         * table and code offsets pin the native revision and reject IOMANX. */
        if((uintptr_t)entries!=start+0x1cb4)return -EINVAL;
        const iop_library_t *library=(const void *)(start+0x1ca0);
        if(library->version!=0x104 || memcmp(library->name,"ioman\0\0\0",8) ||
           (uintptr_t)entries[20]!=start+0x18e8 || entries[35]!=0 ||
           memcmp((const void *)(start+0x18e8),adddrv_head,sizeof(adddrv_head)))return -EINVAL;
        native_adddrv=(int (*)(NpNativeDevice *))(uintptr_t)entries[20];
        return 0;
    }
    return -ENOENT;
}
int np_file_native_adddrv(NpNativeDevice *device) {
    if(!native_adddrv)return -ENODEV;
    if(!device)return -EINVAL;
    int result=native_adddrv(device);
    printf("PSXCORE native-adddrv=completed entry=%p device=%p result=%d\n",
        (void *)native_adddrv,(void *)device,result);
    return result;
}
void _retonly(void) {}
int _start(int argc,char **argv) {
    (void)argc;(void)argv;
    if(native_adddrv)return MODULE_NO_RESIDENT_END;
    int error=capture();
    if(!error)error=RegisterLibraryEntries(&_exp_psxnat);
    if(error) {
        native_adddrv=NULL;
        printf("PSXCORE native-capture=rejected result=%d\n",error);
        return MODULE_NO_RESIDENT_END;
    }
    printf("PSXCORE native-capture=ready entry=%p library=psxnat revision=IOPRP252\n",
        (void *)native_adddrv);
    return MODULE_RESIDENT_END;
}
