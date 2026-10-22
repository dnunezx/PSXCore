/* Arithmetic for checksum-pinned POPS guest adapters. No host services,
 * timing changes, mutable selectors or dependency on the loader's gp. */
#include <stdint.h>
#include <stddef.h>

typedef int64_t Wide;
typedef struct { uint32_t r[64]; } Gte;
static Wide product(int32_t a,int32_t b) {
#ifdef _EE
    Wide v;
    __asm__ volatile("mult $zero,%1,%2\n\tpmfhl.lw %0" : "=r"(v) : "r"(a),"r"(b) : "hi","lo");
    return v;
#else
    return (Wide)a*b;
#endif
}
static uint64_t unsigned_product(uint32_t a,uint32_t b) {
#ifdef _EE
    uint64_t v;
    __asm__ volatile("multu $zero,%1,%2\n\tpmfhl.lw %0" : "=r"(v) : "r"(a),"r"(b) : "hi","lo");
    return v;
#else
    return (uint64_t)a*b;
#endif
}
static int32_t s16(uint32_t v) { return (int16_t)v; }
static Wide wrap44(Wide v,unsigned channel,uint32_t *flags) {
    if(v>=((Wide)1<<43))*flags|=1u<<(30-channel);
    if(v<-((Wide)1<<43))*flags|=1u<<(27-channel);
    return (Wide)((uint64_t)v<<20)>>20;
}
static int32_t limit(Wide v,int32_t lo,int32_t hi,uint32_t bit,uint32_t *flags) {
    if(v<lo){*flags|=bit;return lo;}
    if(v>hi){*flags|=bit;return hi;}
    return (int32_t)v;
}
static void check0(Wide v,uint32_t *flags) {
    if(v>INT32_MAX)*flags|=1u<<16;
    if(v<INT32_MIN)*flags|=1u<<15;
}
static int32_t ir(Wide v,unsigned ch,unsigned lm,uint32_t *flags) {
    return limit(v,lm?0:-32768,32767,1u<<(24-ch),flags);
}
static void publish(Gte *g,Wide v,unsigned ch,unsigned shift,unsigned lm,uint32_t *f) {
    int32_t mac=(int32_t)(v>>shift);
    g->r[25+ch]=(uint32_t)mac;
    g->r[9+ch]=(uint32_t)ir(mac,ch,lm,f);
}
static void color(Gte *g,uint32_t *f) {
    uint32_t rgb=g->r[6]&0xff000000u;
    for(unsigned ch=0;ch<3;++ch)
        rgb|=(uint32_t)limit((int32_t)g->r[25+ch]>>4,0,255,1u<<(21-ch),f)<<(8*ch);
    g->r[20]=g->r[21];g->r[21]=g->r[22];g->r[22]=rgb;
}
static unsigned reciprocal(unsigned h,unsigned z,uint32_t *f) {
    if(h>=z*2){*f|=1u<<17;return 0x1ffff;}
    unsigned shift;
#ifdef _EE
    unsigned redundant;
    __asm__("plzcw %0,%1" : "=r"(redundant) : "r"(z));
    shift=redundant-15;
#else
    shift=0;while((z<<shift)<0x8000)++shift;
#endif
    unsigned d=z<<shift,n=h<<shift,index=(d-0x7fc0)>>7;
    /* The last hardware seed is zero, rather than unsigned underflow. */
    int seed=(int)((0x40000/(index+256)+1)/2)-257;
    unsigned u=(unsigned)(seed<0?0:seed)+257;
    unsigned correction=(0x2000080-d*u)>>8;
    unsigned factor=(0x80+correction*u)>>8;
    unsigned q=(unsigned)((unsigned_product(n,factor)+0x8000)>>16);
    return q>0x1ffff?0x1ffff:q;
}
static void matrix(Gte *g,unsigned selector,int32_t m[9]) {
    if(selector==3){
        int32_t r=(g->r[6]&255)*16;
        m[0]=-r;m[1]=r;m[2]=s16(g->r[8]);
        for(unsigned i=3;i<6;++i)m[i]=s16(g->r[33]);
        for(unsigned i=6;i<9;++i)m[i]=s16(g->r[34]);
    }else{
        unsigned base=32+selector*8;
        for(unsigned i=0;i<9;++i)m[i]=s16(g->r[base+i/2]>>((i&1)*16));
    }
}
static void vector(Gte *g,unsigned selector,int32_t v[3]) {
    if(selector==3){for(unsigned i=0;i<3;++i)v[i]=s16(g->r[9+i]);}
    else{v[0]=s16(g->r[2*selector]);v[1]=s16(g->r[2*selector]>>16);v[2]=s16(g->r[2*selector+1]);}
}
static Wide dot(const int32_t *m,const int32_t *v,int32_t t,unsigned ch,uint32_t *f) {
    Wide x=(Wide)t*4096;
    for(unsigned j=0;j<3;++j)x=wrap44(x+product(m[j],v[j]),ch,f);
    return x;
}
static void transform(Gte *g,unsigned op,unsigned sf,unsigned lm,uint32_t *f) {
    int32_t m[9],v[3];matrix(g,0,m);
    unsigned count=op==0x30?3:1;
    for(unsigned k=0;k<count;++k){
        Wide raw[3];vector(g,k,v);
        for(unsigned ch=0;ch<3;++ch){
            raw[ch]=dot(m+3*ch,v,(int32_t)g->r[37+ch],ch,f);
            int32_t mac=(int32_t)(raw[ch]>>sf);
            g->r[25+ch]=(uint32_t)mac;
            if(ch<2)g->r[9+ch]=(uint32_t)ir(mac,ch,lm,f);
            else{
                (void)ir(raw[ch]>>12,ch,0,f);
                uint32_t ignored=0;
                g->r[11]=(uint32_t)ir(mac,ch,lm,&ignored);
            }
        }
        unsigned z=(unsigned)limit(raw[2]>>12,0,65535,1u<<18,f);
        g->r[16]=g->r[17];g->r[17]=g->r[18];g->r[18]=g->r[19];g->r[19]=z;
        unsigned q=reciprocal(g->r[58]&65535,z,f);
        int32_t xy[2];
        for(unsigned ch=0;ch<2;++ch){
            Wide a=product((int32_t)g->r[9+ch],(int32_t)q)+(int32_t)g->r[56+ch];
            check0(a,f);xy[ch]=limit(a>>16,-1024,1023,1u<<(14-ch),f);
        }
        g->r[12]=g->r[13];g->r[13]=g->r[14];
        g->r[14]=((uint32_t)xy[0]&65535)|((uint32_t)xy[1]<<16);
        if(k+1==count){
            Wide a=product(s16(g->r[59]),(int32_t)q)+(int32_t)g->r[60];
            check0(a,f);g->r[24]=(uint32_t)a;
            g->r[8]=(uint32_t)limit(a>>12,0,4096,1u<<12,f);
        }
    }
}
void np_gte_arithmetic_c(Gte *g,uint32_t opcode) {
    unsigned op=opcode&63,sf=(opcode&(1u<<19))?12:0,lm=(opcode>>10)&1;
    uint32_t f=0;
    if(op==1 || op==0x30)transform(g,op,sf,lm,&f);
    else if(op==6){
        int32_t x[3],y[3];
        for(unsigned i=0;i<3;++i){x[i]=s16(g->r[12+i]);y[i]=s16(g->r[12+i]>>16);}
        Wide a=product(x[0],y[1]-y[2])+product(x[1],y[2]-y[0])+product(x[2],y[0]-y[1]);
        check0(a,&f);g->r[24]=(uint32_t)a;
    }else if(op==0x2d || op==0x2e){
        unsigned n=op==0x2d?3:4,sum=0;
        for(unsigned i=20-n;i<20;++i)sum+=g->r[i]&65535;
        Wide a=product(s16(g->r[n==3?61:62]),(int32_t)sum);
        check0(a,&f);g->r[24]=(uint32_t)a;
        g->r[7]=(uint32_t)limit(a>>12,0,65535,1u<<18,&f);
    }else if(op==0x12){
        int32_t m[9],v[3];matrix(g,(opcode>>17)&3,m);vector(g,(opcode>>15)&3,v);
        unsigned cv=(opcode>>13)&3;
        for(unsigned ch=0;ch<3;++ch){
            Wide a;
            if(cv==2){
                a=wrap44((Wide)(int32_t)g->r[53+ch]*4096+product(m[3*ch],v[0]),ch,&f);
                (void)ir((int32_t)(a>>sf),ch,0,&f);
                a=wrap44(product(m[3*ch+1],v[1]),ch,&f);
                a=wrap44(a+product(m[3*ch+2],v[2]),ch,&f);
            }else a=dot(m+ch*3,v,cv==3?0:(int32_t)g->r[(cv==0?37:45)+ch],ch,&f);
            publish(g,a,ch,sf,lm,&f);
        }
    }else if(op==0x11){
        int32_t v[3];vector(g,3,v);
        for(unsigned ch=0;ch<3;++ch){
            Wide base=(Wide)v[ch]*4096;
            Wide d=wrap44((Wide)(int32_t)g->r[53+ch]*4096-base,ch,&f);
            int32_t delta=ir((int32_t)(d>>sf),ch,0,&f);
            Wide a=wrap44(base+product(delta,s16(g->r[8])),ch,&f);
            publish(g,a,ch,sf,lm,&f);
        }
        color(g,&f);
    }else if(op==0x3d || op==0x3e){
        int32_t v[3];vector(g,3,v);
        for(unsigned ch=0;ch<3;++ch){
            Wide a=product(v[ch],s16(g->r[8]));
            if(op==0x3e)a+=(Wide)(int32_t)g->r[25+ch]*((Wide)1<<sf);
            publish(g,wrap44(a,ch,&f),ch,sf,lm,&f);
        }
        color(g,&f);
    }
    g->r[63]=f|((f&0x7f87e000)?0x80000000u:0);
}

static const int16_t standard_scale[64]={
  23170,23170,23170,23170,23170,23170,23170,23170,
  32138,27245,18204,6392,-6393,-18205,-27246,-32139,
  30273,12539,-12540,-30274,-30274,-12540,12539,30273,
  27245,-6393,-32139,-18205,18204,32138,6392,-27246,
  23170,-23171,-23171,23170,23170,-23171,-23171,23170,
  18204,-32139,6392,27245,-27246,-6393,32138,-18205,
  12539,-30274,30273,-12540,-12540,30273,-30274,12539,
  6392,-18205,27245,-32139,32138,-27246,18204,-6393};
static const uint8_t zigzag[64]={
  0,1,8,16,9,2,3,10,17,24,32,25,18,11,4,5,12,19,26,33,40,48,
  41,34,27,20,13,6,7,14,21,28,35,42,49,56,57,50,43,36,29,22,15,
  23,30,37,44,51,58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63};

/* Returns zero only for the original standard-transform path. Raw POPS RLE
 * coefficients remain unchanged; qscale zero undoes its zigzag placement.
 * Scale and quant tables are the guest's native uploaded tables. */
int np_mdec_arithmetic_c(const int16_t *raw,const int16_t *expanded,int16_t *out) {
    uintptr_t offset=(uintptr_t)expanded-0x597d60u;
    if(offset>=0x4100 || (offset&127))return 0;
    unsigned q=(unsigned)(offset>>8),chroma=(offset>>7)&1;
    const volatile int16_t *scale=(const volatile int16_t *)0x59be60;
    if(q){
        unsigned i=0;
        while(i<64 && (scale[i]>>3)==(standard_scale[i]>>3))++i;
        if(i==64)return 0;
    }
    const volatile uint8_t *qt=(const volatile uint8_t *)(uintptr_t)(0x597ce0+64*chroma);
    int16_t block[64],temp[64];
    for(unsigned k=0;k<64;++k){
        int32_t v=raw[zigzag[k]];
        if(!q)block[k]=(int16_t)(v*2);
        else{
            v=k?(v*qt[k]*(int32_t)q+4)>>3:v*qt[0];
            if(v<-1024)v=-1024;
            if(v>1023)v=1023;
            block[zigzag[k]]=(int16_t)v;
        }
    }
    /* Integer matrix transform, using the scale table's actual upper 13 bits.
     * A temporary block makes in-place input/output safe. */
    for(unsigned pass=0;pass<2;++pass){
        const int16_t *src=pass?temp:block;
        int16_t *dst=pass?out:temp;
        for(unsigned x=0;x<8;++x)for(unsigned y=0;y<8;++y){
            int32_t sum=0;
            for(unsigned z=0;z<8;++z)sum+=(int32_t)src[y+z*8]*(scale[x+z*8]>>3);
            dst[x+y*8]=(int16_t)((sum+4095)>>13);
        }
    }
    return 1;
}
