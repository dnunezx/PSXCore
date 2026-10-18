/* Capture the pinned main-thread context before normal POPS startup.
 * k0/k1 are reserved kernel scratch, as in the native EE ABI. Other GPRs and
 * both packed HI/LO banks retain native behavior. Records use uncached RAM. */
#if NP_CD_SWAP
    .section ._psxcore_cd_swap_runtime.entry,"ax",@progbits
    .set noreorder
    .set noat
    .balign 16
    .macro BASE
    lui $k0,%hi(np_cd_swap_context+0x20000000)
    addiu $k0,$k0,%lo(np_cd_swap_context+0x20000000)
    .endm
    .globl np_cd_swap_context_boot
np_cd_swap_context_boot:
    /* Replaced JAL's link, before native _InitSys and its first EI. Only
     * GetThreadId/ReferThreadStatus syscalls run; neither uses caller stack. */
    lui $ra,0x20
    addiu $ra,$ra,0x1a0
    BASE
    .irp reg,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25
    sq $\reg,288+16*(\reg-1)($k0)
    .endr
    sq $gp,688($k0)
    sq $fp,704($k0)
    sq $ra,720($k0)
    pmfhi $k1
    sq $k1,736($k0)
    pmflo $k1
    sq $k1,752($k0)
    sw $sp,28($k0)
    mfc0 $k1,$12
    sw $k1,32($k0)
    sw $gp,36($k0)
    li $v1,47
    syscall
    BASE
    sw $v0,20($k0)
    move $a0,$v0
    addiu $a1,$k0,48
    li $v1,48
    syscall
    BASE
    sw $v0,24($k0)
    sync
    li $k1,1
    sw $k1,16($k0)
    lq $k1,736($k0)
    pmthi $k1
    lq $k1,752($k0)
    pmtlo $k1
    .irp reg,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25
    lq $\reg,288+16*(\reg-1)($k0)
    .endr
    lq $gp,688($k0)
    lq $fp,704($k0)
    lq $ra,720($k0)
    j 0x235ea8
    nop

    .section ._psxcore_cd_swap_runtime.data,"aw",@progbits
    .balign 16
    .globl np_cd_swap_context
np_cd_swap_context:
    .word 0x58544344,1,768,0
    .space 752
    .globl np_cd_swap_context_end
np_cd_swap_context_end:
#endif
