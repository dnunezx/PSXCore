/* Original bounded validation of the TOC format consumed by pinned POPS. */
#ifndef PSXCORE_CDTOC_H
#define PSXCORE_CDTOC_H
#include <stddef.h>
#include <stdint.h>
#ifndef NP_CD_TOC_CODE
#define NP_CD_TOC_CODE
#endif
static inline NP_CD_TOC_CODE int np_cd_bcd(unsigned b) {
    return (b&15)<10 && (b>>4)<10?(int)((b>>4)*10+(b&15)):-1;
}
static inline NP_CD_TOC_CODE int np_cd_msf(const unsigned char *p) {
    int m=np_cd_bcd(p[0]),s=np_cd_bcd(p[1]),f=np_cd_bcd(p[2]);
    return m<0 || s<0 || s>=60 || f<0 || f>=75?-1:m*4500+s*75+f;
}
/* Reject extended minutes, malformed/nonmonotonic tracks and data beyond the
 * 1024-byte native TOC. Outputs publish only after every record validates. */
static inline NP_CD_TOC_CODE int np_cd_toc_geometry(const unsigned char *p,size_t bytes,
    uint32_t sectors,uint32_t *last_lba,uint32_t *tracks_out) {
    if(!p || bytes!=1024 || !last_lba || !tracks_out || !sectors ||
       p[0]!=0x41 || p[2]!=0xa0 || p[7]!=1 || p[8]!=0x20 ||
       p[12]!=0xa1 || p[22]!=0xa2 || p[26])return -1;
    int tracks=np_cd_bcd(p[17]),lead=np_cd_msf(p+27);
    if(tracks<1 || tracks>99 || lead<150 || (uint32_t)(lead-150)!=sectors)return -1;
    int previous=149;
    for(int i=0;i<tracks;++i) {
        const unsigned char *r=p+30+10*i;
        int start=np_cd_msf(r+7);
        if(np_cd_bcd(r[2])!=i+1 || r[6] || (r[0]!=0x01 && r[0]!=0x41) ||
           start<=previous || start>=lead || (!i && start!=150))return -1;
        previous=start;
    }
    *last_lba=sectors-1;*tracks_out=(uint32_t)tracks;return 0;
}
#endif
