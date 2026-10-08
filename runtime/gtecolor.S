/* Shared POPS ORGB conversion. Preserve the native helper's clobber set:
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
