#ifndef NP_GTE_NATIVE_THREE_PATCH_H
#define NP_GTE_NATIVE_THREE_PATCH_H
#include "psxcore_patch.h"
#ifndef NP_GTE_NATIVE_THREE_EXTERNAL_SYMBOLS
extern void np_gte_native_three_begin(void),np_gte_native_three_end(void);
extern void np_gte_avsz_tail(void),np_gte_avsz_tail_end(void);
extern void np_gte_compile_rtps(void);
extern void np_gte_compile_rtpt(void);
extern void np_gte_compile_gpf(void);
extern void np_gte_compile_gpl(void);
extern void np_gte_compile_intpl(void);
#endif
static inline const char *np_gte_native_three_plan(NpPatchSet *set) {
 const uintptr_t begin=(uintptr_t)np_gte_native_three_begin,end=(uintptr_t)np_gte_native_three_end;
 if(!set || set->count>NP_PATCH_LIMIT-8)return "gte-native-three-capacity";
 if(begin!=0xece00 || end<=begin || end>0xef000 || ((begin|end)&3))return "gte-native-three-resident";
 const uintptr_t entries[]={(uintptr_t)np_gte_compile_rtps,(uintptr_t)np_gte_compile_rtpt,(uintptr_t)np_gte_compile_gpf,(uintptr_t)np_gte_compile_gpl,(uintptr_t)np_gte_compile_intpl,(uintptr_t)np_gte_avsz_tail};
 for(unsigned i=0;i<6;++i)if(entries[i]<begin || entries[i]>=end || (entries[i]&3))return "gte-native-three-entry";
 if((uintptr_t)np_gte_avsz_tail_end-(uintptr_t)np_gte_avsz_tail!=92)return "gte-native-three-avsz";
 const char *error;
 static const unsigned char expected0[]={0x21,0x0,0x2,0x3c,0x15,0x1,0x0,0x10};
 error=np_patch_add(set,0x206800,expected0,(const uint32_t[]){0x08000000u|((uint32_t)np_gte_compile_rtps>>2),0},sizeof(expected0));if(error)return error;
 static const unsigned char expected1[]={0x21,0x0,0x2,0x3c,0x11,0x1,0x0,0x10};
 error=np_patch_add(set,0x206810,expected1,(const uint32_t[]){0x08000000u|((uint32_t)np_gte_compile_rtpt>>2),0},sizeof(expected1));if(error)return error;
 static const unsigned char expected2[]={0x21,0x0,0x2,0x3c,0xc3,0x0,0x0,0x10};
 error=np_patch_add(set,0x206948,expected2,(const uint32_t[]){0x08000000u|((uint32_t)np_gte_compile_gpf>>2),0},sizeof(expected2));if(error)return error;
 static const unsigned char expected3[]={0x21,0x0,0x2,0x3c,0xa3,0x0,0x0,0x10};
 error=np_patch_add(set,0x2069c8,expected3,(const uint32_t[]){0x08000000u|((uint32_t)np_gte_compile_gpl>>2),0},sizeof(expected3));if(error)return error;
 static const unsigned char expected4[]={0x21,0x0,0x2,0x3c,0x65,0x0,0x0,0x10};
 error=np_patch_add(set,0x206ac0,expected4,(const uint32_t[]){0x08000000u|((uint32_t)np_gte_compile_intpl>>2),0},sizeof(expected4));if(error)return error;
 static const unsigned char expected5[]={0x18,0x40,0x19,0x1,0xff,0xff,0x2,0x34,0x6,0x80,0x3,0x3c,0x2d,0x8,0x0,0x0,0x60,0x0,0x88,0xac,0x3,0x43,0x8,0x0,0x2a,0x28,0x0,0x1,0xb,0x40,0x5,0x0};
 error=np_patch_add(set,0x208d40,expected5,(const unsigned char *)np_gte_avsz_tail+0,sizeof(expected5));if(error)return error;
 static const unsigned char expected6[]={0xb,0x28,0x65,0x0,0x25,0x8,0x25,0x0,0x2a,0x28,0x48,0x0,0xb,0x40,0x45,0x0,0xb,0x28,0x65,0x0,0x25,0x8,0x25,0x0,0x1c,0x0,0x88,0xa4,0x87,0x7f,0x2,0x3c};
 error=np_patch_add(set,0x208d60,expected6,(const unsigned char *)np_gte_avsz_tail+32,sizeof(expected6));if(error)return error;
 static const unsigned char expected7[]={0x0,0xe0,0x42,0x34,0x0,0x80,0x3,0x3c,0x24,0x10,0x41,0x0,0xa,0x18,0x2,0x0,0x25,0x8,0x23,0x0,0x8,0x0,0xe0,0x3,0xfc,0x0,0x81,0xac};
 error=np_patch_add(set,0x208d80,expected7,(const unsigned char *)np_gte_avsz_tail+64,sizeof(expected7));if(error)return error;
 return NULL;
}
#endif
