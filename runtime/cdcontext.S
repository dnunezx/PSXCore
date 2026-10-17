/* Original minimal-frame diagnostics. Never calls the gate or changes media.
 * k0/k1 are reserved kernel scratch, as in the native EE ABI. Other GPRs and
 * both packed HI/LO banks retain native behavior. Records use uncached RAM. */
#if NP_CD_CONTEXT
    .section ._psxcore_cd_gate.entry,"ax",@progbits
    .set noreorder
    .set noat
    .balign 16
    .macro BASE
    lui $k0,%hi(np_cd_context+0x20000000)
    addiu $k0,$k0,%lo(np_cd_context+0x20000000)
    .endm
    .globl np_cd_context_boot
np_cd_context_boot:
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

    /* Briefly disable EIE while preserving its incoming value. The initial
     * 16 bytes fit the pinned writer's original 16-byte frame, and the
     * scheduler's just-restored 32-byte frame. AT survives interrupts, unlike
     * k0/k1; do not hold either kernel scratch register live before DI.
     * native call, syscall, wait or admission action occurs at either entry.
     * Rows: visits, flags, min/max SP, first/last RA, status OR/AND,
     * first GP, last SP, sequence, reserved. Flags never authorize a gate. */
    .macro ENTER row,write=0
    addiu $sp,$sp,-16
    sq $at,0($sp)
    mfc0 $at,$12
    andi $at,$at,6
    bnez $at,2f
    nop
    /* POPS normally uses user mode with EDI set. Conservatively require
     * explicit EI/DI permission in every mode; refuse EXL/ERL contexts. */
    mfc0 $at,$12
    srl $at,$at,17
    andi $at,$at,1
    bnez $at,1f
    nop
2:
    /* No masking permission: use only the saved ordinary AT. A nonzero
     * rejected SP is a sticky fault marker, without unsafe scratch writes. */
    lui $at,%hi(np_cd_context+0x20000000)
    addiu $at,$at,%lo(np_cd_context+0x20000000)
    sw $sp,\row+4($at)
    lq $at,0($sp)
    addiu $sp,$sp,16
    .if \write
    j np_cd_gate_native_write
    .else
    jr $ra
    .endif
    nop
1:
    mfc0 $at,$12
    di
    sync.p
    BASE
    sw $at,40($k0)
    lq $at,0($sp)
    addiu $sp,$sp,16
    b np_cd_context_sample
    li $k1,\row
    .endm
    .globl np_cd_context_write
np_cd_context_write:
    ENTER 96,1
    .globl np_cd_context_tail
np_cd_context_tail:
    ENTER 160
    .section ._psxcore_cd_park.text,"ax",@progbits
np_cd_context_sample:
    sq $at,224($k0)
    sq $t0,240($k0)
    sq $t1,256($k0)
    sq $t2,272($k0)
    addu $k0,$k0,$k1
    lui $t0,%hi(np_cd_context+0x20000000)
    addiu $t0,$t0,%lo(np_cd_context+0x20000000)
    lw $k1,40($t0)
    sw $k1,48($k0)
    lw $t0,40($k0)
    addiu $t0,$t0,1
    sw $t0,40($k0)
    sync
    lw $t0,0($k0)
    li $t1,-1
    beq $t0,$t1,8f
    nop
    bnez $t0,1f
    nop
    sw $sp,8($k0)
    sw $sp,12($k0)
    sw $ra,16($k0)
    sw $gp,32($k0)
    sw $k1,28($k0)
1:
    addiu $t0,$t0,1
    sw $t0,0($k0)
    sw $ra,20($k0)
    sw $sp,36($k0)
    lw $t0,8($k0)
    sltu $at,$sp,$t0
    beqz $at,2f
    nop
    sw $sp,8($k0)
2:
    lw $t0,12($k0)
    sltu $at,$t0,$sp
    beqz $at,3f
    nop
    sw $sp,12($k0)
3:
    lw $t0,24($k0)
    or $t0,$t0,$k1
    sw $t0,24($k0)
    lw $t0,28($k0)
    and $t0,$t0,$k1
    sw $t0,28($k0)
    /* Reject incomplete/error boot receipt, wrong gp, alignment, non-thread
     * mode, or SP outside the kernel-reported main stack, with 1280 bytes
     * below SP. This is an observation predicate, never activation consent. */
    lw $t2,4($k0)
    lui $t1,%hi(np_cd_context+0x20000000)
    addiu $t1,$t1,%lo(np_cd_context+0x20000000)
    lw $t0,16($t1)
    beqz $t0,7f
    lw $t0,24($t1)
    bltz $t0,7f
    lw $t0,20($t1)
    blez $t0,7f
    andi $t0,$sp,15
    bnez $t0,7f
    andi $t0,$k1,6
    bnez $t0,7f
    lui $t0,0x51
    addiu $t0,$t0,-22928
    bne $gp,$t0,7f
    lw $t0,56($t1)
    sltu $at,$sp,$t0
    bnez $at,7f
    lw $t1,60($t1)
    blez $t1,7f
    addu $t1,$t1,$t0
    sltu $at,$t1,$t0
    bnez $at,7f
    lui $at,0x200
    sltu $at,$at,$t1
    bnez $at,7f
    nop
    sltu $at,$t1,$sp
    bnez $at,7f
    subu $t0,$sp,$t0
    sltiu $at,$t0,1280
    beqz $at,6f
    nop
7:
    ori $t2,$t2,1
    sw $t2,4($k0)
6:
    b 9f
    nop
8:
    li $t0,2
    sw $t0,4($k0)
9:
    sync
    lw $t0,40($k0)
    addiu $t0,$t0,1
    sw $t0,40($k0)
    lui $t0,%hi(np_cd_context+0x20000000+96)
    addiu $t0,$t0,%lo(np_cd_context+0x20000000+96)
    bne $k0,$t0,np_cd_context_return
    nop
    .macro RESTORE
    BASE
    lq $at,224($k0)
    lq $t0,240($k0)
    lq $t1,256($k0)
    lq $t2,272($k0)
    lw $k1,40($k0)
    srl $k1,$k1,16
    andi $k1,$k1,1
    beqz $k1,5f
    nop
    ei
    sync.p
5:
    .endm
    RESTORE
    j np_cd_gate_native_write
    nop
np_cd_context_return:
    RESTORE
    jr $ra
    nop
    .section ._psxcore_cd_gate.data,"aw",@progbits
    .balign 16
    .globl np_cd_context
np_cd_context:
    .word 0x58544344,1,768,0
    .space 752
    .globl np_cd_context_end
np_cd_context_end:
#endif
