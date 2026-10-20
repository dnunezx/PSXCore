/* Pinned POPS INTPL sf=1/lm=0 correction. The native calculation and its
 * signed intermediate saturation remain intact. Select signed final IR
 * saturation without changing the DPCS/DPCT entries sharing the same tail. */
    .set noreorder
    .set noat
    .section ._psxcore_gte_intpl,"ax",@progbits
    .balign 16
    .globl np_gte_intpl_begin,np_gte_intpl_compile
    .globl np_gte_intpl_signed,np_gte_intpl_clamp,np_gte_intpl_end
np_gte_intpl_begin:
np_gte_intpl_compile:
    /* s0 retains the complete guest opcode at the pinned compiler arm.
     * Other sf/lm combinations keep their original native destination. */
    andi $v0,$s0,0x400
    bnez $v0,1f
    nop
    srl $v0,$s0,19
    andi $v0,$v0,1
    beqz $v0,1f
    nop
    la $a0,np_gte_intpl_signed
    j 0x206c5c
    nop
1:
    li $a0,0x209770
    j 0x206c5c
    nop
np_gte_intpl_signed:
    /* Generated JAL links are word aligned. Carry the signed selector in
     * their unused low bit only while the leaf calculation runs; restore
     * the exact 128-bit link before rejoining native IR/FIFO publication.
     * No global selector, extra stack or additional allocated GPR is used. */
    pxor $14,$14,$14
    li $14,1
    por $ra,$ra,$14
    j 0x209770
    nop
np_gte_intpl_clamp:
    /* The displaced delay slot has already loaded v0=32767. Ordinary
     * INTPL/DPCS/DPCT links retain every native comparison and flag. */
    andi $14,$ra,1
    bnez $14,2f
    nop
    j 0x209870
    slt $14,$8,$zero
2:
    pxor $14,$14,$14
    li $14,1
    pxor $ra,$ra,$14
    li $a1,-32768
    .macro NP_INTPL_SIGNED reg,bit
    slt $14,\reg,$a1
    slt $15,$v0,\reg
    movn \reg,$a1,$14
    movn \reg,$v0,$15
    or $14,$14,$15
    sll $14,$14,\bit
    or $at,$at,$14
    .endm
    NP_INTPL_SIGNED $8,24
    NP_INTPL_SIGNED $9,23
    NP_INTPL_SIGNED $10,22
    j 0x2098c0
    nop
np_gte_intpl_end:
