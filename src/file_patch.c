#include "psxcore_file_patch.h"
#include "psxcore_compat.h"
const char *np_file_storage_plan(uint32_t runtime,NpPatchSet *set) {
    if(runtime!=NP_RUNTIME_KNOWN || !set || set->count>NP_PATCH_LIMIT)return "file-storage-plan-input";
    /* Already loaded SDK DEV9/BDM/FatFs replace these four native HDD modules.
     * Keep native SIO2, pad, sound and IOPCD. No module payload is changed. */
    static const uint32_t sites[4]={0x2003f0,0x200420,0x200450,0x200480};
    static const uint32_t delays[4]={0x0000382d,0x0000382d,0x03a0382d,0x24060010};
    NpPatchSet next=*set;
    for(unsigned i=0;i<4;++i) {
        uint32_t before[2]={0x0c080076,delays[i]},after[2]={0x24020001,delays[i]};
        const char *error=np_patch_add(&next,sites[i],before,after,sizeof(before));
        if(error)return error;
    }
    *set=next;return NULL;
}
