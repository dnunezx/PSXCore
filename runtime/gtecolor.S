/* Shared POPS IRGB/ORGB conversion. Preserve the native helpers' clobber set:
 * v0/a0/a1/a2 only; allocated PS1 registers, flags and wide GPRs stay intact. */
    .set noreorder
    .set noat
    .section ._psxcore_gte_color,"ax",@progbits
    .balign 16
    .macro NP_ORGB_CHANNEL reg,offset
    lh \reg,\offset($gp)
    sra \reg,\reg,7
    pmaxw \reg,\reg,$zero
    pminw \reg,\reg,$v0
    .endm
    .globl np_gte_orgb
np_gte_orgb:
    /* IRGB and ORGB read the same current color. Clamp signed IR/128
     * to 0..31, pack RGB555 and publish full zero-extended words to both
     * mirrors without setting GTE flags. Signed word min/max avoids
     * all three saturation branches and uses the native clobber set. */
    addiu $v0,$zero,31
    NP_ORGB_CHANNEL $a0,468
    NP_ORGB_CHANNEL $a1,472
    NP_ORGB_CHANNEL $a2,476
    sll $a1,$a1,5
    sll $a2,$a2,10
    or $v0,$a0,$a1
    or $v0,$v0,$a2
    sw $v0,544($gp)
    sw $v0,548($gp)
    jr $ra
    nop
    .globl np_gte_orgb_end
np_gte_orgb_end:
    /* Compiler adapters occupy the former alignment padding. These run
     * after native register allocation and use only caller-clobbered GPRs.
     * Preserve the displaced mask/opcode operations and the existing input
     * and zero-depth helper entry addresses. */
    .globl np_gte_mfc_sxyp,np_gte_swc_sxyp,np_gte_sxyp_end
np_gte_mfc_sxyp:
    addiu $v0,$s0,-492
    addiu $a1,$zero,488
    movz $s0,$a1,$v0
    j 0x207a30
    lui $v0,0x8c00
np_gte_swc_sxyp:
    addiu $v1,$a1,-492
    addiu $a0,$zero,488
    movz $a1,$a0,$v1
    lui $v1,0x8c00
    j 0x207b60
    sll $v0,$v0,16
np_gte_sxyp_end:
    /* Keep the existing input/zero-depth helper addresses. */
    .balign 128
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

/* The class-3 write formatter arrives with its original stack frame, source
 * allocation at 0(sp), destination offset in s1 and output cursor in r10/v0.
 * s0 was already restored in the native branch delay slot. Only FLAG's
 * offset is handled here; the native epilogue restores the remaining state. */
    .section ._psxcore_gte_transfer,"ax",@progbits
    .balign 16
    .globl np_gte_flag_write,np_gte_flag_write_end
np_gte_flag_write:
    addiu $v1,$s1,-684
    bnez $v1,3f
    nop
    lui $a0,%hi(np_gte_flag_template)
    addiu $a0,$a0,%lo(np_gte_flag_template)
    addiu $a1,$zero,12
1:
    lw $a2,0($a0)
    addiu $a0,$a0,4
    sw $a2,0($10)
    addiu $a1,$a1,-1
    bnez $a1,1b
    addiu $10,$10,4
    lw $a0,0($sp)
    sll $a1,$a0,16
    lw $a2,-48($10)
    or $a2,$a2,$a1
    sw $a2,-48($10)
    sll $a1,$a0,21
    lw $a2,-24($10)
    or $a2,$a2,$a1
    sw $a2,-24($10)
    move $v0,$10
3:
    j 0x207b38
    nop
np_gte_flag_template:
    /* Generated code uses only POPS's reserved v0 scratch, preserving the
     * allocated source and RAM mask. Clear reserved bits and incoming bit31,
     * then set the summary byte only when a selected input bit is present.
     * Both stores address FLAG; neighboring GTE state is never touched. */
    .word 0x00001040 /* SLL v0,source,1; rt supplied by the allocator. */
    .word 0x00021342 /* SRL v0,v0,13 */
    .word 0x00021300 /* SLL v0,v0,12 */
    .word 0xaf8202ac /* SW v0,FLAG(gp) */
    .word 0x3c027f87 /* LUI v0,0x7f87 */
    .word 0x3442e000 /* ORI v0,v0,0xe000 */
    .word 0x00021024 /* AND v0,source,v0; rs supplied by the allocator. */
    .word 0x10400004 /* BEQZ v0,end */
    .word 0
    .word 0x938202af /* LBU v0,FLAG+3(gp) */
    .word 0x34420080 /* ORI v0,v0,0x80 */
    .word 0xa38202af /* SB v0,FLAG+3(gp) */
np_gte_flag_write_end:
