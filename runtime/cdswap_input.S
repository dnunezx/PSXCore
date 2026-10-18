/* Original transparent input interceptor; native menu handler still runs. */
#if NP_CD_SWAP
    .section ._psxcore_cd_swap_runtime.entry,"ax",@progbits
    .set noreorder
    .set noat
    .balign 16
    .globl np_cd_swap_input
np_cd_swap_input:
    addiu $sp,$sp,-16
    sq $at,0($sp)
    lui $at,0x50
    ori $at,$at,0xa670
    bne $gp,$at,9f
    nop
    mfc0 $at,$12
    andi $at,$at,6
    bnez $at,9f
    nop
    mfc0 $at,$12
    srl $at,$at,16
    andi $at,$at,3
    addiu $at,$at,-3
    bnez $at,9f
    nop
    lui $at,%hi(np_cd_swap_context+0x20000000+16)
    lw $at,%lo(np_cd_swap_context+0x20000000+16)($at)
    addiu $at,$at,-1
    bnez $at,9f
    nop
    lui $at,%hi(np_cd_swap_context+0x20000000+24)
    lw $at,%lo(np_cd_swap_context+0x20000000+24)($at)
    bltz $at,9f
    nop
    lui $at,%hi(np_cd_swap_context+0x20000000+56)
    lw $at,%lo(np_cd_swap_context+0x20000000+56)($at)
    beqz $at,9f
    nop
    sltu $at,$sp,$at
    bnez $at,9f
    nop
    lui $at,%hi(np_cd_swap_context+0x20000000+56)
    lw $at,%lo(np_cd_swap_context+0x20000000+56)($at)
    subu $at,$sp,$at
    sltiu $at,$at,8192
    bnez $at,9f
    nop
    lui $at,0x200
    sltu $at,$sp,$at
    beqz $at,9f
    nop
    /* The lower-bound check established room before saving another GPR.
     * Check the original SP against the kernel-reported stack end as well. */
    addiu $sp,$sp,-16
    sq $t0,0($sp)
    lui $at,%hi(np_cd_swap_context+0x20000000+56)
    lw $t0,%lo(np_cd_swap_context+0x20000000+56)($at)
    lui $at,%hi(np_cd_swap_context+0x20000000+60)
    lw $at,%lo(np_cd_swap_context+0x20000000+60)($at)
    addu $at,$at,$t0
    sltu $t0,$at,$t0
    bnez $t0,8f
    nop
    lui $t0,0x200
    sltu $t0,$t0,$at
    bnez $t0,8f
    nop
    addiu $t0,$sp,32
    sltu $t0,$at,$t0
    bnez $t0,8f
    nop
    lq $t0,0($sp)
    addiu $sp,$sp,16
    lq $at,0($sp)
    addiu $sp,$sp,16
    addiu $sp,$sp,-672
    /* Leave 32 bytes for the callee's ABI argument home area. */
    .irp reg,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25
    sq $\reg,32+16*(\reg-1)($sp)
    .endr
    sq $gp,432($sp)
    sq $fp,448($sp)
    sq $ra,464($sp)
    pmfhi $t0
    sq $t0,480($sp)
    pmflo $t0
    sq $t0,496($sp)
    .irp reg,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    swc1 $f\reg,512+4*\reg($sp)
    .endr
    cfc1 $t0,$31
    sw $t0,640($sp)
    mfsa $t0
    sw $t0,644($sp)
    jal np_cd_swap_buttons
    move $a2,$s3
    lw $t0,644($sp)
    mtsa $t0
    lw $t0,640($sp)
    ctc1 $t0,$31
    .irp reg,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    lwc1 $f\reg,512+4*\reg($sp)
    .endr
    lq $t0,480($sp)
    pmthi $t0
    lq $t0,496($sp)
    pmtlo $t0
    .irp reg,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25
    lq $\reg,32+16*(\reg-1)($sp)
    .endr
    lq $gp,432($sp)
    lq $fp,448($sp)
    lq $ra,464($sp)
    addiu $sp,$sp,672
    j 0x2289e0
    nop
8:
    lq $t0,0($sp)
    addiu $sp,$sp,16
9:
    lq $at,0($sp)
    addiu $sp,$sp,16
    j 0x2289e0
    nop
#endif
