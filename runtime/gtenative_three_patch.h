#ifndef NP_GTE_NATIVE_THREE_PATCH_H
#define NP_GTE_NATIVE_THREE_PATCH_H
#include "psxcore_patch.h"
#ifndef NP_MDEC_ZERO_SCALE
#define NP_MDEC_ZERO_SCALE NP_MDEC_BLOCK_FASTPATH
#endif
#if NP_MDEC_ZERO_SCALE && !NP_MDEC_BLOCK_FASTPATH
#error "NP_MDEC_ZERO_SCALE requires NP_MDEC_BLOCK_FASTPATH"
#endif
#ifndef NP_GTE_MATRIX_MODES
#define NP_GTE_MATRIX_MODES 1
#endif
#ifndef NP_GTE_NATIVE_THREE_EXTERNAL_SYMBOLS
extern void np_gte_native_three_begin(void),np_gte_native_three_end(void);
extern void np_gte_avsz_tail(void),np_gte_avsz_tail_end(void);
extern void np_gte_compile_rtps(void);
extern void np_gte_compile_rtpt(void);
extern void np_gte_compile_gpf(void);
extern void np_gte_compile_gpl(void);
extern void np_gte_compile_intpl(void);
#if NP_MDEC_ZERO_SCALE && NP_MDEC_BLOCK_FASTPATH
extern void np_mdec_zero_quant_init(void),np_mdec_zero_rle(void),np_mdec_zero_frame(void);
extern const unsigned short np_mdec_zero_factors[64];
#endif
#if NP_GTE_MATRIX_MODES
extern void np_gte_matrix_compile(void);
#endif
extern void np_gte_compile_nclip(void);
extern void np_gte_screen_native(void);
extern void np_gte_depth_native(void);
extern void np_gte_near_template(void),np_gte_near_template_end(void);
#endif
static inline const char *np_gte_native_three_plan(NpPatchSet *set) {
 const uintptr_t begin=(uintptr_t)np_gte_native_three_begin,end=(uintptr_t)np_gte_native_three_end;
 if(!set || set->count>NP_PATCH_LIMIT-(11+(NP_GTE_MATRIX_MODES?1:0)+((NP_MDEC_ZERO_SCALE && NP_MDEC_BLOCK_FASTPATH)?2:0)))return "gte-native-three-capacity";
 if(begin!=0xece00 || end<=begin || end>0xef000 || ((begin|end)&3))return "gte-native-three-resident";
 const uintptr_t entries[]={(uintptr_t)np_gte_compile_rtps,(uintptr_t)np_gte_compile_rtpt,(uintptr_t)np_gte_compile_gpf,(uintptr_t)np_gte_compile_gpl,(uintptr_t)np_gte_compile_intpl,(uintptr_t)np_gte_avsz_tail,(uintptr_t)np_gte_compile_nclip,(uintptr_t)np_gte_screen_native,(uintptr_t)np_gte_depth_native
#if NP_MDEC_ZERO_SCALE && NP_MDEC_BLOCK_FASTPATH
 ,(uintptr_t)np_mdec_zero_quant_init,(uintptr_t)np_mdec_zero_rle,(uintptr_t)np_mdec_zero_frame
#endif
#if NP_GTE_MATRIX_MODES
 ,(uintptr_t)np_gte_matrix_compile
#endif
 };
 for(unsigned i=0;i<sizeof(entries)/sizeof(entries[0]);++i)if(entries[i]<begin || entries[i]>=end || (entries[i]&3))return "gte-native-three-entry";
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
 static const uint32_t mac0_expected_206840[]={0x3c020021u,0x10000105u};
 error=np_patch_add(set,0x206840,mac0_expected_206840,(const uint32_t[]){0x08000000u|((uint32_t)np_gte_compile_nclip>>2),0x00000000u},8);if(error)return error;
 static const uint32_t mac0_expected_208aac[]={0x000c643bu,0x000d6c3bu};
 error=np_patch_add(set,0x208aac,mac0_expected_208aac,(const uint32_t[]){0x08000000u|((uint32_t)np_gte_screen_native>>2),0x000c643bu},8);if(error)return error;
 static const uint32_t mac0_expected_208b20[]={0x848200ecu,0x004b5818u};
 error=np_patch_add(set,0x208b20,mac0_expected_208b20,(const uint32_t[]){0x08000000u|((uint32_t)np_gte_depth_native>>2),0x848200ecu},8);if(error)return error;
#if NP_GTE_MATRIX_MODES
 static const uint32_t matrix_expected[]={0x15600009u,0x26310004u};
 error=np_patch_add(set,0x206c10,matrix_expected,(const uint32_t[]){0x08000000u|((uint32_t)np_gte_matrix_compile>>2),0},8);if(error)return error;
#endif
#if NP_MDEC_ZERO_SCALE && NP_MDEC_BLOCK_FASTPATH
 const uintptr_t factors=(uintptr_t)np_mdec_zero_factors;
 if((factors&15) || factors<begin || factors+128>end)return "mdec-zero-factors";
 static const uint32_t zero_cache_expected[]={0x03e00008u,0};
 error=np_patch_add(set,0x20d29c,zero_cache_expected,(const uint32_t[]){0x08000000u|((uint32_t)np_mdec_zero_quant_init>>2),0},8);if(error)return error;
 static const uint32_t zero_frame_expected[]={0x27bdffa0u,0x3c030059u,0xffb10018u};
 error=np_patch_add(set,0x20dbb8,zero_frame_expected,(const uint32_t[]){0x08000000u|((uint32_t)np_mdec_zero_frame>>2),0x3c030059u,0},12);if(error)return error;
#endif
 return NULL;
}
#endif
