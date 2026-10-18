#if NP_SPU_REVERB_READONLY
/* Exact scalar work reduction for the checksum-pinned native reverb leaf.
 * Keep each guest read/write volatile and in native order, including live
 * ATTR.bit7 checks. Word arithmetic deliberately wraps before signed shifts.
 * rN names track the native low word; sN names track private stack temporaries.
 * The ABI bridge sends unsupported counts to the original guarded routine.
 * Final multiply operands restore both native HI/LO units before returning. */
#include <stdint.h>

/* Read a signed halfword with the native instruction. The uint32_t result
 * retains the same low word while the instruction supplies sign extension. */
static __inline__ __attribute__((always_inline)) uint32_t np_reverb_read_s16(uint32_t address) {
    uint32_t value;
    __asm__ volatile("lh %0,%1" : "=r"(value)
        : "m"(*(volatile const int16_t *)(uintptr_t)address));
    return value;
}


/* PMAXW/PMINW perform signed word comparisons. Borrow a volatile wide
 * temporary, then return through a scalar instruction so callee registers'
 * upper 64-bit lanes retain the native ABI. Neither operation changes HI/LO. */
static __inline__ __attribute__((always_inline)) uint32_t np_reverb_clip16(uint32_t value) {
    uint32_t result;
    __asm__("pmaxw $8,%1,%2\n\tpminw $8,$8,%3\n\tsll %0,$8,0"
        : "=r"(result) : "r"(value),"r"(-32768),"r"(32767) : "$8");
    return result;
}

__attribute__((section("._psxcore_reverb_exact"),noinline)) int np_reverb_exact32(int samples) {
uint32_t r1=0,r2=0,r3=0,r4=0,r5=0,r6=0,r7=0,r8=0,r9=0,r10=0,r11=0,r12=0,r13=0,r14=0,r15=0,r16=0,r17=0,r18=0,r19=0,r20=0,r21=0,work_start=0,r23=0,r24=0,context=0,r26=0,r27=0,r28=0,irq_address=0,r31=0;
uint32_t remaining=0,s8=0,sc=0,s10=0,s14=0,s18=0,s1c=0,s20=0,s24=0,s28=0,s2c=0,s34=0,s38=0,s3c=0,s40=0,s44=0,s48=0,s4c=0,s50=0,s54=0,s58=0,s5c=0,s60=0,s64=0,s68=0,s6c=0,s70=0,s74=0,s78=0,s7c=0,s80=0,s84=0,s88=0,s8c=0,s90=0,s94=0,s98=0,s9c=0,sa0=0,sa4=0,sa8=0,sac=0,sb0=0,sb4=0,sb8=0,sbc=0,sc0=0,sc4=0,sc8=0,scc=0,sample_ptr=0;
uint32_t mx0=0,my0=0,mx1=0,my1=0;
r4=(uint32_t)samples;r5=0x7ab980;r6=0x70000350;
r3=(uint32_t)*(volatile uint16_t *)(uintptr_t)(r6+(uint32_t)418);
r2=(uint32_t)*(volatile uint16_t *)(uintptr_t)(r6+(uint32_t)420);
r3=r3<<3;
r2=r2<<3;
work_start=r3+r5;
{int take=((int32_t)r4<=0);irq_address=r2+r5; if(take)goto L212fd0;}
context=r6+0u;
remaining=r4;
r2=context+(uint32_t)2896;
sample_ptr=r2;
L212408:;
r2=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2520);
r2=r2^1u;
{int take=(r2==0u);*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2520)=(uint32_t)r2; if(take)goto L2124f0;}
r4=sample_ptr;
r10=0u+(uint32_t)32767;
r5=0u+(uint32_t)32767;
r6=0u+(uint32_t)-32768;
r3=(uint32_t)*(volatile uint32_t *)(uintptr_t)(r4+(uint32_t)8);
r7=0u+(uint32_t)22963;
r18=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2492);
r12=0u+(uint32_t)4807;
r2=(int32_t)r10<(int32_t)r3;
r23=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2488);
r5=np_reverb_clip16(r3);
r24=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2504);
r2=(int32_t)r5<-32768;
r19=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2508);
r5=r5;
r21=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2500);
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2468)=(uint32_t)r5;
r14=0u+(uint32_t)4871;
r20=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2512);
{uint32_t x=r18,y=r7;mx0=x;my0=y;r4=x*y;}
{uint32_t x=r23,y=r14;mx0=x;my0=y;r2=x*y;}
{uint32_t x=r24,y=r7;mx1=x;my1=y;r3=x*y;}
{uint32_t x=r19,y=r12;mx1=x;my1=y;r8=x*y;}
r7=sample_ptr;
{uint32_t x=r20,y=r12;mx0=x;my0=y;r15=x*y;}
{uint32_t x=r21,y=r14;mx0=x;my0=y;r11=x*y;}
r6=(uint32_t)*(volatile uint32_t *)(uintptr_t)(r7+(uint32_t)12);
r5=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2484);
r9=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2496);
r8=r8+r2;
r2=(int32_t)r10<(int32_t)r6;
r11=r15+r11;
r10=np_reverb_clip16(r6);
r4=r4-r5;
r3=r3-r9;
r4=r4+r8;
r3=r3+r11;
r2=(int32_t)r10<-32768;
r8=0u+(uint32_t)-32768;
r4=(int32_t)r4>>15;
r10=r10;
r3=(int32_t)r3>>15;
s8=r11;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2480)=(uint32_t)r10;
*(volatile uint32_t *)(uintptr_t)(r7+(uint32_t)8)=(uint32_t)r4;
*(volatile uint32_t *)(uintptr_t)(r7+(uint32_t)12)=(uint32_t)r3;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2484)=(uint32_t)r23;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2488)=(uint32_t)r18;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2492)=(uint32_t)r19;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2496)=(uint32_t)r21;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2500)=(uint32_t)r24;
{int take=(0u==0u);*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2504)=(uint32_t)r20; if(take)goto L212fb4;}
;
L2124f0:;
r12=sample_ptr;
r14=0u+(uint32_t)32767;
r15=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2468);
r6=0u+(uint32_t)-32768;
r3=(uint32_t)*(volatile uint32_t *)(uintptr_t)(r12+(uint32_t)8);
r7=0u+(uint32_t)4807;
r5=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2464);
r9=0u|32768u;
r2=(int32_t)r14<(int32_t)r3;
r14=np_reverb_clip16(r3);
r2=0u+(uint32_t)22963;
{uint32_t x=r15,y=r2;mx0=x;my0=y;r4=x*y;}
r2=(int32_t)r14<-32768;
r6=r14;
r3=0u+(uint32_t)4871;
sc=r6;
{uint32_t x=r5,y=r3;mx0=x;my0=y;r5=x*y;}
{uint32_t x=r6,y=r7;mx0=x;my0=y;r2=x*y;}
r8=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2548);
r3=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2460);
s28=r8;
r4=r4-r3;
r2=r2+r5;
r12=np_reverb_read_s16(context+(uint32_t)462);
r4=r4+r2;
r24=(int32_t)r4>>15;
r5=sample_ptr;
s18=r12;
r3=np_reverb_read_s16(r8+(uint32_t)0);
r8=0u+(uint32_t)32767;
r14=np_reverb_read_s16(context+(uint32_t)508);
s10=r8;
{uint32_t x=r3,y=r12;mx0=x;my0=y;r3=x*y;}
{uint32_t x=r24,y=r14;mx0=x;my0=y;r4=x*y;}
r14=0u+(uint32_t)32767;
r15=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2620);
s24=r15;
r3=(int32_t)r3>>15;
r2=np_reverb_read_s16(context+(uint32_t)452);
s34=r4;
s1c=r2;
r9=r9-r2;
r7=s34;
r6=np_reverb_read_s16(r15+(uint32_t)0);
s20=r9;
r24=(int32_t)r7>>15;
r3=r24+r3;
r7=0u+(uint32_t)-32768;
r4=(uint32_t)*(volatile uint32_t *)(uintptr_t)(r5+(uint32_t)12);
r5=0u+(uint32_t)22963;
r10=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2524);
{uint32_t x=r3,y=r2;mx0=x;my0=y;r3=x*y;}
s38=r8;
r2=(int32_t)r8<(int32_t)r4;
r14=np_reverb_clip16(r4);
{uint32_t x=r6,y=r9;mx0=x;my0=y;r6=x*y;}
r12=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2584);
r2=(int32_t)r14<-32768;
r7=r14;
r14=0u+(uint32_t)4871;
s2c=r12;
r3=(int32_t)r3>>15;
r6=(int32_t)r6>>15;
r11=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2624);
r3=r3+r6;
r15=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2480);
r2=(int32_t)r8<(int32_t)r3;
s10=r7;
r8=np_reverb_clip16(r3);
{uint32_t x=r15,y=r5;mx0=x;my0=y;r4=x*y;}
r15=0u+(uint32_t)4807;
r12=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2476);
{uint32_t x=r7,y=r15;mx1=x;my1=y;r5=x*y;}
r3=0u+(uint32_t)-32768;
{uint32_t x=r12,y=r14;mx0=x;my0=y;r12=x*y;}
s14=r12;
r5=r5+r12;
r6=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2472);
r2=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2580);
r9=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2556);
r4=r4-r6;
s48=r2;
r2=(int32_t)r8<-32768;
r8=r8;
r3=irq_address^r10;
r4=r4+r5;
if(*(volatile uint16_t *)(uintptr_t)0x700004fa&128u)*(volatile uint16_t *)(uintptr_t)(r10+(uint32_t)0)=(uint16_t)r8;
r23=(int32_t)r4>>15;
r4=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2628);
r5=s2c;
if(r3==0)irq_address=0u;
s44=r4;
r3=irq_address^r9;
r6=s18;
if(r3==0)irq_address=0u;
r2=np_reverb_read_s16(r5+(uint32_t)0);
r10=r10+(uint32_t)2;
r5=np_reverb_read_s16(r11+(uint32_t)0);
{uint32_t x=r2,y=r6;mx0=x;my0=y;r2=x*y;}
r6=0u+(uint32_t)32767;
r7=s20;
r12=s1c;
r8=s24;
{uint32_t x=r5,y=r7;mx0=x;my0=y;r5=x*y;}
r2=(int32_t)r2>>15;
r14=s28;
r2=r24+r2;
r4=irq_address^r8;
{uint32_t x=r2,y=r12;mx0=x;my0=y;r2=x*y;}
r12=8585216u;
if(r4==0)irq_address=0u;
r5=(int32_t)r5>>15;
r4=irq_address^r11;
r11=r11+(uint32_t)2;
if(r4==0)irq_address=0u;
r15=s2c;
r2=(int32_t)r2>>15;
r3=irq_address^r14;
r2=r2+r5;
r5=s38;
if(r3==0)irq_address=0u;
r3=(int32_t)r6<(int32_t)r2;
r5=np_reverb_clip16(r2);
r8=r8+(uint32_t)2;
r4=irq_address^r15;
r15=r15+(uint32_t)2;
r14=r14+(uint32_t)2;
r7=8585216u;
s24=r8;
r12=r12+(uint32_t)-18050;
s28=r14;
r14=0u+(uint32_t)-32768;
s2c=r15;
r15=r12<r11;
r2=(int32_t)r5<-32768;
r7=r7+(uint32_t)-18050;
r14=r5;
r2=r12+0u;
r7=r7<r10;
r24=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2552);
r20=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2560);
r19=r12<r8;
s3c=r7;
if(r4==0)irq_address=0u;
s40=r15;
if(*(volatile uint16_t *)(uintptr_t)0x700004fa&128u)*(volatile uint16_t *)(uintptr_t)(r9+(uint32_t)0)=(uint16_t)r14;
r9=r9+(uint32_t)2;
r5=r12<r9;
r14=s2c;
if(r5!=0)r9=work_start;
r3=s28;
r7=s48;
r8=r2<r14;
r14=s3c;
r12=r2<r3;
r15=s18;
r3=np_reverb_read_s16(r7+(uint32_t)0);
if(r14!=0)r10=work_start;
r7=s44;
r5=s24;
{uint32_t x=r3,y=r15;mx0=x;my0=y;r3=x*y;}
r14=s28;
r4=np_reverb_read_s16(r7+(uint32_t)0);
if(r19!=0)r5=work_start;
r15=s20;
if(r12!=0)r14=work_start;
r2=np_reverb_read_s16(context+(uint32_t)510);
r3=(int32_t)r3>>15;
r7=s40;
{uint32_t x=r4,y=r15;mx0=x;my0=y;r4=x*y;}
s24=r5;
{uint32_t x=r23,y=r2;mx0=x;my0=y;r2=x*y;}
s28=r14;
if(r7!=0)r11=work_start;
r15=s2c;
r4=(int32_t)r4>>15;
if(r8!=0)r15=work_start;
r23=(int32_t)r2>>15;
s2c=r15;
r3=r23+r3;
r2=s1c;
r5=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2468);
r8=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2480);
{uint32_t x=r3,y=r2;mx0=x;my0=y;r3=x*y;}
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2460)=(uint32_t)r5;
r5=0u+(uint32_t)32767;
r2=irq_address^r20;
r7=sc;
if(r2==0)irq_address=0u;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2472)=(uint32_t)r8;
r3=(int32_t)r3>>15;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2464)=(uint32_t)r7;
r3=r3+r4;
r2=(int32_t)r6<(int32_t)r3;
r12=s10;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2476)=(uint32_t)r12;
s4c=r6;
r6=np_reverb_clip16(r3);
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2524)=(uint32_t)r10;
r2=(int32_t)r6<-32768;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2556)=(uint32_t)r9;
r3=0u+(uint32_t)-32768;
r10=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2528);
r6=r6;
r14=s24;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2624)=(uint32_t)r11;
r2=irq_address^r10;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2620)=(uint32_t)r14;
if(r2==0)irq_address=0u;
r15=s28;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2548)=(uint32_t)r15;
r4=s2c;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2584)=(uint32_t)r4;
s98=r5;
r11=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2632);
sa4=r5;
if(*(volatile uint16_t *)(uintptr_t)0x700004fa&128u)*(volatile uint16_t *)(uintptr_t)(r20+(uint32_t)0)=(uint16_t)r6;
r20=r20+(uint32_t)2;
r15=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2540);
r3=np_reverb_read_s16(r24+(uint32_t)0);
r6=s18;
r7=s44;
{uint32_t x=r3,y=r6;mx0=x;my0=y;r3=x*y;}
r12=s48;
r2=irq_address^r7;
r7=0u+(uint32_t)-32768;
if(r2==0)irq_address=0u;
r4=np_reverb_read_s16(r11+(uint32_t)0);
r2=irq_address^r11;
r11=r11+(uint32_t)2;
if(r2==0)irq_address=0u;
r8=s20;
r2=irq_address^r12;
r14=s1c;
r3=(int32_t)r3>>15;
if(r2==0)irq_address=0u;
r3=r23+r3;
r2=irq_address^r24;
{uint32_t x=r4,y=r8;mx0=x;my0=y;r4=x*y;}
r8=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2532);
{uint32_t x=r3,y=r14;mx0=x;my0=y;r3=x*y;}
if(r2==0)irq_address=0u;
r2=irq_address^r8;
s5c=r15;
if(r2==0)irq_address=0u;
r6=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2564);
r2=irq_address^r15;
r4=(int32_t)r4>>15;
r3=(int32_t)r3>>15;
if(r2==0)irq_address=0u;
r3=r3+r4;
r2=s4c;
r4=(int32_t)r5<(int32_t)r3;
r5=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2572);
r2=np_reverb_clip16(r3);
r24=r24+(uint32_t)2;
s4c=r2;
r2=irq_address^r6;
if(r2==0)irq_address=0u;
r4=s4c;
r2=irq_address^r5;
r9=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2568);
if(r2==0)irq_address=0u;
r3=(int32_t)r4<-32768;
r7=r4;
if(*(volatile uint16_t *)(uintptr_t)0x700004fa&128u)*(volatile uint16_t *)(uintptr_t)(r10+(uint32_t)0)=(uint16_t)r7;
r10=r10+(uint32_t)2;
r12=s44;
r2=s5c;
r12=r12+(uint32_t)2;
r4=s5c;
s44=r12;
r4=r4+(uint32_t)2;
r12=s48;
r14=np_reverb_read_s16(r8+(uint32_t)0);
r8=r8+(uint32_t)2;
r12=r12+(uint32_t)2;
s60=r14;
r14=irq_address^r9;
r15=np_reverb_read_s16(r5+(uint32_t)0);
r5=r5+(uint32_t)2;
s68=r15;
r2=np_reverb_read_s16(r2+(uint32_t)0);
r3=np_reverb_read_s16(r6+(uint32_t)0);
r6=r6+(uint32_t)2;
s70=r2;
r2=8585216u;
s5c=r4;
r2=r2+(uint32_t)-18050;
r21=r2<r20;
r7=np_reverb_read_s16(r9+(uint32_t)0);
r9=r9+(uint32_t)2;
s48=r12;
if(r21!=0)r20=work_start;
s88=r7;
r7=r2<r10;
sc8=r14;
r14=r2+0u;
r13=r14<r24;
r15=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2576);
if(r13!=0)r24=work_start;
s7c=r15;
r15=s44;
r4=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2536);
s50=r7;
r2=r14<r15;
s80=r4;
r4=r14<r11;
r7=s48;
if(r4!=0)r11=work_start;
r12=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2544);
r7=r14<r7;
s84=r12;
r12=r14<r8;
s54=r7;
if(r12!=0)r8=work_start;
r15=s5c;
r4=s5c;
r7=r14<r15;
r14=r14<r6;
s74=r14;
r14=8585216u;
r15=8585216u;
r14=r14+(uint32_t)-18050;
r15=r15+(uint32_t)-18050;
r14=r14<r9;
r15=r15<r5;
s8c=r14;
s78=r15;
if(r7!=0)r4=work_start;
r15=s50;
r14=s44;
if(r15!=0)r10=work_start;
r15=s48;
if(r2!=0)r14=work_start;
r2=s54;
s44=r14;
if(r2!=0)r15=work_start;
s5c=r4;
s48=r15;
r7=s74;
r12=s78;
r14=s8c;
if(r7!=0)r6=work_start;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2528)=(uint32_t)r10;
if(r12!=0)r5=work_start;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2560)=(uint32_t)r20;
r12=0u+(uint32_t)32767;
if(r14!=0)r9=work_start;
r2=s44;
r15=sc8;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2628)=(uint32_t)r2;
if(r15==0)irq_address=0u;
r4=s7c;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2632)=(uint32_t)r11;
r2=irq_address^r4;
r7=s48;
if(r2==0)irq_address=0u;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2580)=(uint32_t)r7;
sb0=r12;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2552)=(uint32_t)r24;
r14=s80;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2532)=(uint32_t)r8;
r2=irq_address^r14;
r15=s5c;
if(r2==0)irq_address=0u;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2540)=(uint32_t)r15;
r4=s84;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2564)=(uint32_t)r6;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2572)=(uint32_t)r5;
r2=irq_address^r4;
sc0=r12;
if(r2==0)irq_address=0u;
r8=np_reverb_read_s16(context+(uint32_t)454);
sbc=r12;
r6=s60;
r14=np_reverb_read_s16(context+(uint32_t)456);
sc4=r12;
{uint32_t x=r6,y=r8;mx0=x;my0=y;r11=x*y;}
r12=s80;
r5=np_reverb_read_s16(context+(uint32_t)458);
r2=s7c;
s58=r5;
r11=(int32_t)r11>>15;
r5=s68;
r4=np_reverb_read_s16(r12+(uint32_t)0);
r12=r12+(uint32_t)2;
r15=np_reverb_read_s16(context+(uint32_t)460);
s80=r12;
{uint32_t x=r4,y=r8;mx1=x;my1=y;r4=x*y;}
r12=s84;
r6=np_reverb_read_s16(r2+(uint32_t)0);
r2=r2+(uint32_t)2;
s7c=r2;
{uint32_t x=r5,y=r15;mx0=x;my0=y;r2=x*y;}
r7=s70;
{uint32_t x=r6,y=r15;mx0=x;my0=y;r6=x*y;}
r5=np_reverb_read_s16(r12+(uint32_t)0);
r12=r12+(uint32_t)2;
s84=r12;
{uint32_t x=r7,y=r14;mx0=x;my0=y;r10=x*y;}
r7=s58;
r2=(int32_t)r2>>15;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2568)=(uint32_t)r9;
{uint32_t x=r5,y=r14;mx1=x;my1=y;r5=x*y;}
s64=r11;
{uint32_t x=r3,y=r7;mx1=x;my1=y;r3=x*y;}
r11=(int32_t)r10>>15;
r14=0u+(uint32_t)32767;
r7=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2604);
r6=(int32_t)r6>>15;
r12=s64;
r4=(int32_t)r4>>15;
s94=r7;
r3=(int32_t)r3>>15;
r2=r12+r2;
r3=r3+r11;
r9=np_reverb_read_s16(context+(uint32_t)464);
r4=r4+r6;
r10=np_reverb_read_s16(r7+(uint32_t)0);
r5=(int32_t)r5>>15;
s6c=r2;
{uint32_t x=r10,y=r9;mx0=x;my0=y;r2=x*y;}
r8=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2588);
r7=s6c;
s90=r8;
r24=r7+r3;
r2=(int32_t)r2>>15;
r12=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2612);
r24=r24-r2;
r11=np_reverb_read_s16(context+(uint32_t)466);
r2=(int32_t)r14<(int32_t)r24;
sa0=r12;
r8=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2596);
r15=s88;
r3=s58;
s9c=r8;
{uint32_t x=r15,y=r3;mx0=x;my0=y;r7=x*y;}
{uint32_t x=r24,y=r9;mx0=x;my0=y;r3=x*y;}
r15=s98;
r12=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2608);
r15=np_reverb_clip16(r24);
sac=r12;
r2=(int32_t)r15<-32768;
r12=s90;
r8=(int32_t)r7>>15;
r7=0u+(uint32_t)-32768;
r3=(int32_t)r3>>15;
r7=r15;
r2=r8+r5;
if(*(volatile uint16_t *)(uintptr_t)0x700004fa&128u)*(volatile uint16_t *)(uintptr_t)(r12+(uint32_t)0)=(uint16_t)r7;
r7=0u+(uint32_t)15855;
r15=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2592);
r24=r10+r3;
r14=sa0;
r23=r4+r2;
r4=s94;
r8=0u+(uint32_t)32767;
r6=np_reverb_read_s16(r14+(uint32_t)0);
sa8=r15;
{uint32_t x=r6,y=r11;mx0=x;my0=y;r3=x*y;}
r5=s9c;
r2=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2616);
r15=s9c;
sb8=r2;
r2=irq_address^r12;
if(r2==0)irq_address=0u;
r12=r12+(uint32_t)2;
s90=r12;
r2=irq_address^r4;
r3=(int32_t)r3>>15;
if(r2==0)irq_address=0u;
r24=r24-r3;
r12=sa4;
r4=r4+(uint32_t)2;
r10=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2492);
r2=(int32_t)r8<(int32_t)r24;
s94=r4;
r4=irq_address^r5;
{uint32_t x=r24,y=r11;mx0=x;my0=y;r5=x*y;}
r12=np_reverb_clip16(r24);
if(r4==0)irq_address=0u;
r3=irq_address^r14;
r14=0u+(uint32_t)-32768;
r2=(int32_t)r12<-32768;
if(r3==0)irq_address=0u;
r14=r12;
r5=(int32_t)r5>>15;
if(*(volatile uint16_t *)(uintptr_t)0x700004fa&128u)*(volatile uint16_t *)(uintptr_t)(r15+(uint32_t)0)=(uint16_t)r14;
r15=r15+(uint32_t)2;
s9c=r15;
r15=0u+(uint32_t)15948;
r2=sac;
r24=r6+r5;
r4=sc0;
r3=0u+(uint32_t)32767;
r21=np_reverb_read_s16(r2+(uint32_t)0);
r2=(int32_t)r3<(int32_t)r24;
r4=np_reverb_clip16(r24);
r8=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2484);
sc0=r4;
{uint32_t x=r21,y=r9;mx0=x;my0=y;r4=x*y;}
{uint32_t x=r10,y=r7;mx1=x;my1=y;r10=x*y;}
r7=sa8;
r12=sc0;
r14=0u+(uint32_t)-32768;
r5=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2488);
r2=irq_address^r7;
r3=(int32_t)r12<-32768;
r4=(int32_t)r4>>15;
r14=r12;
r12=sb0;
r23=r23-r4;
r4=sac;
if(r2==0)irq_address=0u;
r2=0u+(uint32_t)412;
r7=0u+(uint32_t)32767;
{uint32_t x=r5,y=r15;mx0=x;my0=y;r24=x*y;}
{uint32_t x=r14,y=r2;mx1=x;my1=y;r5=x*y;}
r2=(int32_t)r7<(int32_t)r23;
r12=np_reverb_clip16(r23);
sc0=r14;
r2=(int32_t)r12<-32768;
r15=sa8;
r14=0u+(uint32_t)-32768;
r6=0u+(uint32_t)424;
r14=r12;
{uint32_t x=r8,y=r6;mx1=x;my1=y;r8=x*y;}
r3=irq_address^r4;
r6=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2600);
if(*(volatile uint16_t *)(uintptr_t)0x700004fa&128u)*(volatile uint16_t *)(uintptr_t)(r15+(uint32_t)0)=(uint16_t)r14;
r15=r15+(uint32_t)2;
sa8=r15;
if(r3==0)irq_address=0u;
r2=sb8;
r3=irq_address^r6;
if(r3==0)irq_address=0u;
{uint32_t x=r23,y=r9;mx0=x;my0=y;r9=x*y;}
r3=np_reverb_read_s16(r2+(uint32_t)0);
r10=r10+r8;
r4=sa0;
r5=r5+r24;
r7=sac;
{uint32_t x=r3,y=r11;mx0=x;my0=y;r2=x*y;}
r8=sb8;
r4=r4+(uint32_t)2;
r12=sb8;
r7=r7+(uint32_t)2;
r8=irq_address^r8;
r15=s7c;
sa0=r4;
r9=(int32_t)r9>>15;
r4=s80;
r10=r10+r5;
sac=r7;
r14=8585216u;
r5=s84;
r14=r14+(uint32_t)-18050;
r7=s90;
r23=r21+r9;
scc=r8;
r12=r12+(uint32_t)2;
r8=s94;
r2=(int32_t)r2>>15;
sb8=r12;
r12=0u+(uint32_t)32767;
r23=r23-r2;
r16=r14<r15;
r24=r14<r4;
r4=s9c;
r15=r14<r5;
r5=8585216u;
r17=r14<r7;
r7=sa0;
r18=r14<r8;
r14=sbc;
r2=(int32_t)r12<(int32_t)r23;
r12=0u+(uint32_t)-32768;
r14=np_reverb_clip16(r23);
r2=8585216u;
r2=r2+(uint32_t)-18050;
r5=r5+(uint32_t)-18050;
r9=r2<r4;
r2=(int32_t)r14<-32768;
r12=r14;
r14=sa8;
{uint32_t x=r23,y=r11;mx0=x;my0=y;r11=x*y;}
r8=r5<r7;
r14=r5<r14;
r7=r5+0u;
sb4=r14;
r14=0u+(uint32_t)32767;
if(*(volatile uint16_t *)(uintptr_t)0x700004fa&128u)*(volatile uint16_t *)(uintptr_t)(r6+(uint32_t)0)=(uint16_t)r12;
r6=r6+(uint32_t)2;
r11=(int32_t)r11>>15;
r4=r5<r6;
r2=sac;
r23=r3+r11;
r12=sb8;
if(r4!=0)r6=work_start;
r3=r5<r2;
r2=(int32_t)r14<(int32_t)r23;
r5=r7<r12;
r7=sc4;
r12=s7c;
r10=(int32_t)r10>>15;
r7=np_reverb_clip16(r23);
r2=s84;
r14=s80;
if(r16!=0)r12=work_start;
sc4=r7;
if(r15!=0)r2=work_start;
r7=s90;
if(r24!=0)r14=work_start;
s7c=r12;
if(r17!=0)r7=work_start;
s80=r14;
s84=r2;
s90=r7;
r12=s94;
r14=s9c;
r15=sa0;
if(r18!=0)r12=work_start;
s94=r12;
if(r9!=0)r14=work_start;
r2=sa8;
if(r8!=0)r15=work_start;
r7=sb4;
r8=sac;
r12=sb8;
if(r7!=0)r2=work_start;
s9c=r14;
if(r3!=0)r8=work_start;
r14=s7c;
if(r5!=0)r12=work_start;
sa0=r15;
r5=0u+(uint32_t)-32768;
sa8=r2;
sac=r8;
sb8=r12;
r12=0u+(uint32_t)412;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2576)=(uint32_t)r14;
r15=s80;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2536)=(uint32_t)r15;
r4=s84;
r3=sc4;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2544)=(uint32_t)r4;
r2=(int32_t)r3<-32768;
r7=s90;
r5=r3;
sc4=r5;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2588)=(uint32_t)r7;
r14=s94;
r8=sc4;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2604)=(uint32_t)r14;
{uint32_t x=r8,y=r12;mx0=x;my0=y;r5=x*y;}
r2=s9c;
r15=scc;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2596)=(uint32_t)r2;
if(r15==0)irq_address=0u;
r15=0u+(uint32_t)15855;
r3=sa0;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2612)=(uint32_t)r3;
r4=sa8;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2592)=(uint32_t)r4;
r7=sac;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2608)=(uint32_t)r7;
r7=0u+(uint32_t)15948;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2600)=(uint32_t)r6;
r6=0u+(uint32_t)424;
r8=sb8;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2616)=(uint32_t)r8;
r12=sample_ptr;
*(volatile uint32_t *)(uintptr_t)(r12+(uint32_t)8)=(uint32_t)r10;
r2=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2504);
r3=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2496);
r4=(uint32_t)*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2500);
{uint32_t x=r2,y=r15;mx0=x;my0=y;r2=x*y;}
{uint32_t x=r3,y=r6;mx1=x;my1=y;r3=x*y;}
r14=sc0;
{uint32_t x=r4,y=r7;mx0=x;my0=y;r4=x*y;}
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2508)=(uint32_t)r14;
r2=r2+r3;
r5=r5+r4;
r2=r2+r5;
r2=(int32_t)r2>>15;
*(volatile uint32_t *)(uintptr_t)(r12+(uint32_t)12)=(uint32_t)r2;
r8=sc4;
*(volatile uint32_t *)(uintptr_t)(context+(uint32_t)2512)=(uint32_t)r8;
L212fb4:;
r12=remaining;
r14=sample_ptr;
r12=r12+(uint32_t)-1;
r14=r14+(uint32_t)16;
remaining=r12;
{int take=(r12!=0u);sample_ptr=r14; if(take)goto L212408;}
L212fd0:;
__asm__ volatile("mult $zero,%0,%1\n\tmult1 $zero,%2,%3" : : "r"(mx0),"r"(my0),"r"(mx1),"r"(my1) : "hi","lo","memory");
return irq_address==0;
}
#endif
