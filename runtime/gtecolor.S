/* Shared POPS IRGB/ORGB conversion. Preserve the native helpers' clobber set:
 * v0/a0/a1/a2 only; allocated PS1 registers, flags and wide GPRs stay intact. */
    .set noreorder
    .set noat
    .section ._psxcore_gte_color,"ax",@progbits
    .balign 16
    .macro NP_ORGB_CHANNEL reg,offset
    lh \reg,\offset($gp)
    slti $v0,\reg,0
    movn \reg,$zero,$v0
    srl \reg,\reg,7
    sltiu $v0,\reg,32
    bnez $v0,1f
    nop
    addiu \reg,$zero,31
1:
    .endm
    .globl np_gte_orgb
np_gte_orgb:
    /* IR1..IR3 are signed halfwords. ORGB clamps each IR/128 to 0..31
     * without setting GTE flags, then packs once as RGB555. */
    NP_ORGB_CHANNEL $a0,468
    NP_ORGB_CHANNEL $a1,472
    NP_ORGB_CHANNEL $a2,476
    sll $a1,$a1,5
    sll $a2,$a2,10
    or $v0,$a0,$a1
    or $v0,$v0,$a2
    sh $v0,548($gp)
    jr $ra
    nop
    .globl np_gte_orgb_end
np_gte_orgb_end:
    .balign 16
    .globl np_gte_irgb
np_gte_irgb:
    /* PEXT5 expands RGB555 to 8-bit channels (channel << 3). GTE IRGB
     * writes require channel << 7; extract directly at that scale. Ignore
     * bit 15 and upper input bits, without changing GTE flags or IR0. */
    lw $v0,544($gp)
    andi $a0,$v0,31
    sll $a0,$a0,7
    srl $a1,$v0,5
    andi $a1,$a1,31
    sll $a1,$a1,7
    srl $a2,$v0,10
    andi $a2,$a2,31
    sll $a2,$a2,7
    sw $a0,468($gp)
    sw $a1,472($gp)
    sw $a2,476($gp)
    jr $ra
    nop
    .globl np_gte_irgb_end
np_gte_irgb_end:

/* RTPS/RTPT may project a zero SZ3, including H=0. Native POPS reaches
 * its compiler-generated division trap for that valid GTE overflow case.
 * Preserve the native nonzero division and saturate only the zero case. */
    .balign 16
    .globl np_gte_zero_depth,np_gte_zero_depth_end
np_gte_zero_depth:
    bnez $11,1f
    nop
    lui $11,1
    ori $11,$11,0xffff
    lui $v0,0x8002
    or $at,$at,$v0
    j 0x208a8c
    nop
1:
    .word 0x01ab001b /* Native DIVU $13,$11; avoid assembler ABI aliases. */
    mflo $11
    j 0x208a8c
    nop
np_gte_zero_depth_end:
