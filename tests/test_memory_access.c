#include "../runtime/memoryaccess.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

int main(void) {
    uint32_t helpers[16]={0};
    const unsigned ops[]={0x20,0x21,0x23,0x24,0x25,0x28,0x29,0x2b};
    for(unsigned i=0;i<8;++i)helpers[ops[i]-0x20]=0xf3000+32*i;
    /* Actual reservations, arbitrary allocated registers and signed offsets.
     * Check delay-slot argument setup and that later instructions survive. */
    for(unsigned op_index=0;op_index<8;++op_index)
    for(unsigned rt=0;rt<32;++rt)
    for(unsigned imm=0;imm<4;++imm) {
        const unsigned offsets[]={0,0x7fff,0x8000,0xffff};
        const unsigned op=ops[op_index];
        const uint32_t word=(op<<26)|(4<<21)|(rt<<16)|offsets[imm];
        uint32_t code[4]={word,0,0,0x12345678};
        assert(np_memory_emit_rewrite(word,1,code,helpers)==1);
        unsigned call=(op&8)?1:0,argument=(op&8)?0:1;
        assert(code[call]==(0x0c000000u|(helpers[op-0x20]>>2)));
        assert(code[argument]==(0x24070000u|offsets[imm]));
        assert(code[2]==((op&8)?((rt<<21)|0x282d):(0x0040002d|(rt<<11))));
        assert(code[3]==0x12345678);
    }
    /* A malformed reservation must fail before any word changes. */
    const uint32_t word=0x8c980010;
    for(unsigned bad=0;bad<4;++bad) {
        uint32_t code[3]={word,0,0},original[3];
        if(bad<3)code[bad]^=1;
        memcpy(original,code,sizeof(code));
        uint32_t before=helpers[3];
        if(bad==3)helpers[3]|=1;
        assert(np_memory_emit_rewrite(word,1,code,helpers)==-1);
        assert(!memcmp(code,original,sizeof(code)));
        helpers[3]=before;
    }
    uint32_t untouched[3]={word,0,0};
    assert(np_memory_emit_rewrite(word,29,untouched,helpers)==0);
    assert(untouched[0]==word && !untouched[1] && !untouched[2]);
    assert(np_memory_emit_rewrite(0x88980010,1,untouched,helpers)==0);
    assert(np_memory_emit_rewrite(word,1,0,helpers)==-1);
    assert(np_memory_emit_rewrite(word,1,untouched,0)==-1);
    /* Mixed groups preserve merge operations, order and adjacent code. */
    for (unsigned count=1; count<=8; ++count) {
        uint32_t records[8][3], regs[8], code[26]={0x12345678};
        unsigned words=1, expected=0;
        for (unsigned i=0; i<count; ++i) {
            unsigned op=i%3==1 ? 0x22 : ops[i];
            records[i][0]=op; records[i][1]=i+1; records[i][2]=0xfff0+i*2;
            regs[i]=count==8 && i==0 ? 30 : 8+i;
            code[words++]=(op<<26)|(4<<21)|(regs[i]<<16)|records[i][2];
            if ((op&3)!=2) { words+=2; ++expected; }
        }
        code[words]=0x87654321;
        uint32_t original[26]; memcpy(original,code,sizeof(code));
        assert(np_memory_group_rewrite(code+words,count,1,records,regs,helpers)==(int)expected);
        assert(code[0]==original[0] && code[words]==original[words]);
        unsigned at=1;
        for (unsigned i=0; i<count; ++i) {
            if ((records[i][0]&3)==2) { assert(code[at]==original[at]); ++at; continue; }
            uint32_t scalar[3]={original[at],0,0};
            assert(np_memory_emit_rewrite(scalar[0],1,scalar,helpers)==1);
            assert(!memcmp(code+at,scalar,sizeof(scalar))); at+=3;
        }
        /* Rejection of the final reservation leaves the entire group intact. */
        memcpy(code,original,sizeof(code)); code[words-1]^=1;
        memcpy(original,code,sizeof(code));
        assert(np_memory_group_rewrite(code+words,count,1,records,regs,helpers)<0);
        assert(!memcmp(code,original,sizeof(code)));
    }
    uint32_t records[2][3]={{0x29,1,0x1c00},{0x29,2,0x1c02}};
    uint32_t regs[2]={8,9};
    uint32_t group[6]={0xa4881c00,0,0,0xa4891c02,0,0},original[6];
    memcpy(original,group,sizeof(group));
    for (unsigned bad=0; bad<7; ++bad) {
        uint32_t saved=helpers[9];
        if (bad==0) regs[1]=4;
        if (bad==1) records[1][0]=0x27;
        if (bad==2) records[1][1]=32;
        if (bad==3) helpers[9]|=1;
        assert(np_memory_group_rewrite(group+6,bad==4?9:2,bad==5?32:1,
                                      records,regs,bad==6?0:helpers)<0);
        assert(!memcmp(group,original,sizeof(group)));
        regs[1]=9; records[1][0]=0x29; records[1][1]=2; helpers[9]=saved;
    }
    assert(np_memory_group_rewrite(0,0,1,0,0,0)==0);
    assert(np_memory_group_rewrite(group+6,2,29,records,regs,helpers)==0);
    assert(!memcmp(group,original,sizeof(group)));
    puts("Memory emitter: 1024 scalar reservations and rejection/unchanged cases passed");
    puts("Grouped emitter: mixed groups of 1-8, atomic rejection and native-only paths passed");
}
