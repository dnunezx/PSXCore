/* Original transparent main-loop return binding. Normal interrupts stay on.
 * This revision-checked shim requires the exact SP/RA/GP validated for the pinned runtime
 * before allocating its full frame. k0/k1 never carry live unmasked values. */
#if NP_CD_SWAP
#define CD_FRAME 672
#define CD_HOME 32
    .section ._psxcore_cd_swap_runtime.entry,"ax",@progbits
    .set noreorder
    .set noat
    .balign 16
    .globl np_cd_swap_runtime_tail
np_cd_swap_runtime_tail:
    addiu $sp,$sp,-16
    sq $at,0($sp)
    lui $at,0x1ff
    ori $at,$at,0xec90
    bne $sp,$at,9f
    nop
    lui $at,0x20
    ori $at,$at,0x0d7c
    bne $ra,$at,9f
    nop
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
    lq $at,0($sp)
    addiu $sp,$sp,16
    addiu $sp,$sp,-CD_FRAME
    .irp reg,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25
    sq $\reg,CD_HOME+16*(\reg-1)($sp)
    .endr
    sq $gp,CD_HOME+400($sp)
    sq $fp,CD_HOME+416($sp)
    sq $ra,CD_HOME+432($sp)
    pmfhi $t0
    sq $t0,CD_HOME+448($sp)
    pmflo $t0
    sq $t0,CD_HOME+464($sp)
    .irp reg,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    swc1 $f\reg,CD_HOME+480+4*\reg($sp)
    .endr
    cfc1 $t0,$31
    sw $t0,CD_HOME+608($sp)
    mfsa $t0
    sw $t0,CD_HOME+612($sp)
    addiu $a0,$sp,CD_FRAME
    mfc0 $a1,$12
    jal np_cd_swap_runtime_tick
    nop
    lw $t0,CD_HOME+612($sp)
    mtsa $t0
    lw $t0,CD_HOME+608($sp)
    ctc1 $t0,$31
    .irp reg,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    lwc1 $f\reg,CD_HOME+480+4*\reg($sp)
    .endr
    lq $t0,CD_HOME+448($sp)
    pmthi $t0
    lq $t0,CD_HOME+464($sp)
    pmtlo $t0
    .irp reg,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25
    lq $\reg,CD_HOME+16*(\reg-1)($sp)
    .endr
    lq $gp,CD_HOME+400($sp)
    lq $fp,CD_HOME+416($sp)
    lq $ra,CD_HOME+432($sp)
    jr $ra
    addiu $sp,$sp,CD_FRAME
9:
    lui $at,%hi(np_cd_swap_runtime+0x20000000)
    addiu $at,$at,%lo(np_cd_swap_runtime+0x20000000)
    sw $sp,64($at)
    lq $at,0($sp)
    jr $ra
    addiu $sp,$sp,16
    .balign 16
    .section ._psxcore_cd_swap_runtime.code_end,"a",@progbits
    .globl np_cd_swap_runtime_code_end
np_cd_swap_runtime_code_end:
    .section ._psxcore_cd_swap_runtime.end,"a",@progbits
    .globl np_cd_swap_runtime_end
np_cd_swap_runtime_end:
#endif
