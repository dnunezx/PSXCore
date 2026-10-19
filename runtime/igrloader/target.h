/* Return-only ELF layout admission. The SDK performs the actual loading. */
#ifndef NP_IGR_TARGET_H
#define NP_IGR_TARGET_H
#include <stdint.h>
#include <stddef.h>
#define NP_IGR_USER_FIRST 0x000ef000u
#define NP_IGR_USER_END   0x01ff0000u
typedef struct { uint32_t first,end; } NpIgrTargetRange;
typedef struct { uint32_t entry,count; NpIgrTargetRange ranges[8]; } NpIgrTarget;
static uint32_t np_igr_u32(const unsigned char *p) {
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static unsigned np_igr_u16(const unsigned char *p) { return p[0]|(unsigned)p[1]<<8; }
static int np_igr_target_header(const unsigned char h[52],uint32_t bytes,
    uint32_t *offset,unsigned *count) {
    if(h[0]!=127 || h[1]!='E' || h[2]!='L' || h[3]!='F' || h[4]!=1 || h[5]!=1 || h[6]!=1 ||
       np_igr_u16(h+16)!=2 || np_igr_u16(h+18)!=8 || np_igr_u32(h+20)!=1 ||
       np_igr_u16(h+40)!=52 || np_igr_u16(h+42)!=32)return 0;
    *offset=np_igr_u32(h+28);*count=np_igr_u16(h+44);
    return *count && *count<=8 && *offset>=52 && (uint64_t)*offset+*count*32<=bytes;
}
static int np_igr_target_plan(const unsigned char h[52],const unsigned char *ph,
    uint32_t bytes,NpIgrTarget *target) {
    uint32_t offset;unsigned count;
    if(!np_igr_target_header(h,bytes,&offset,&count))return 0;
    NpIgrTarget t={.entry=np_igr_u32(h+24)};unsigned executable=0;
    if(t.entry&3)return 0;
    for(unsigned i=0;i<count;++i,ph+=32) {
        if(np_igr_u32(ph)!=1)continue;
        uint32_t start=np_igr_u32(ph+8),file=np_igr_u32(ph+16),memory=np_igr_u32(ph+20);
        if(file>memory || (uint64_t)np_igr_u32(ph+4)+file>bytes || np_igr_u32(ph+12)!=start)return 0;
        if(!memory)continue;
        uint64_t end=(uint64_t)start+memory;
        if(start<NP_IGR_USER_FIRST || end>NP_IGR_USER_END)return 0;
        for(unsigned j=0;j<t.count;++j)
            if(start<t.ranges[j].end && t.ranges[j].first<end)return 0;
        if((np_igr_u32(ph+24)&1) && t.entry>=start && (uint64_t)t.entry+4<=(uint64_t)start+file)executable=1;
        unsigned n=t.count++;
        while(n && t.ranges[n-1].first>start) {t.ranges[n]=t.ranges[n-1];--n;}
        t.ranges[n]=(NpIgrTargetRange){start,(uint32_t)end};
    }
    if(!t.count || !executable)return 0;
    *target=t;return 1;
}
#endif
