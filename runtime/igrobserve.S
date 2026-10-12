/* Opt-in telemetry only. All calls and their results retain the native ABI.
 * Original native calls only unless the separate one-shot NP_IGR_PROBE is set.
 * Observer state uses the uncached alias and independent publication counters. */
#if NP_IGR_OBSERVE
    .section ._psxcore_igr_observe,"ax",@progbits
    .set noreorder
    .set noat
    .balign 16
    .globl np_igr_observe_begin
np_igr_observe_begin:
    .macro OBS_STATE
    la $t0,np_igr_observation
    lui $t1,0x2000
    or $t0,$t0,$t1
    .endm
    .macro OBS_BEGIN offset
    lw $t1,\offset($t0)
    addiu $t1,$t1,1
    sw $t1,\offset($t0)
    sync
    .endm
    .macro OBS_END offset
    sync
    lw $t1,\offset($t0)
    addiu $t1,$t1,1
    sw $t1,\offset($t0)
    .endm
    .globl np_igr_pad_copy_observe
np_igr_pad_copy_observe:
    addiu $sp,$sp,-96
    sw $a0,32($sp)
    sw $a1,36($sp)
    sq $ra,48($sp)
    OBS_STATE
    OBS_BEGIN 8
    lw $t1,12($t0)
    addiu $t1,$t1,1
    sw $t1,12($t0)
    sw $a1,16($t0)
    sw $a0,20($t0)
    sw $a2,24($t0)
    sw $zero,28($t0)
    sw $zero,32($t0)
    sw $zero,36($t0)
    sw $zero,40($t0)
    sw $zero,44($t0)
    sw $zero,48($t0)
    sw $zero,52($t0)
    /* Only the four 128-byte halves of POPS's two checked slot-zero buffers.
     * Do not inspect arbitrary native pointers or short/oversize reports. */
    li $t1,0x1fffffff
    and $t2,$a1,$t1
    li $t1,0x00506e00
    subu $t2,$t2,$t1
    sltiu $t1,$t2,512
    beqz $t1,1f
    andi $t2,$t2,127
    bnez $t2,1f
    sltiu $t1,$a2,4
    bnez $t1,1f
    sltiu $t1,$a2,33
    beqz $t1,1f
    nop
    lw $t1,88($a1)
    sw $t1,28($t0)
    lw $t1,96($a1)
    sw $t1,36($t0)
    lbu $t1,112($a1)
    sw $t1,40($t0)
    lbu $t1,113($a1)
    sw $t1,44($t0)
    li $t1,1
    sw $t1,52($t0)
1:
    jal 0x240bbc
    nop
    sq $v0,64($sp)
    sq $v1,80($sp)
    OBS_STATE
    lw $t1,52($t0)
    beqz $t1,2f
    lw $t2,36($sp)
    lw $t1,88($t2)
    sw $t1,32($t0)
    lw $t2,32($sp)
    lbu $t1,0($t2)
    lbu $t3,1($t2)
    sll $t3,$t3,8
    or $t1,$t1,$t3
    lbu $t3,2($t2)
    sll $t3,$t3,16
    or $t1,$t1,$t3
    lbu $t3,3($t2)
    sll $t3,$t3,24
    or $t1,$t1,$t3
    sw $t1,48($t0)
2:
    OBS_END 8
    lq $v0,64($sp)
    lq $v1,80($sp)
    lq $ra,48($sp)
    jr $ra
    addiu $sp,$sp,96

    /* Called inside the existing checked save-wait wrapper. Only t0/t1 change;
     * keep the native wait arguments and wide v0/v1 result intact. */
    .globl np_igr_wait_enter
np_igr_wait_enter:
    OBS_STATE
    OBS_BEGIN 56
    lw $t1,60($t0)
    addiu $t1,$t1,1
    sw $t1,60($t0)
    li $t1,1
    sw $t1,68($t0)
    li $t1,0x507920
    lw $t1,0($t1)
    sw $t1,76($t0)
    li $t1,0x527930
    lw $t1,0($t1)
    sw $t1,80($t0)
    OBS_END 56
    jr $ra
    nop
    .globl np_igr_wait_exit
np_igr_wait_exit:
    OBS_STATE
    OBS_BEGIN 56
    lw $t1,64($t0)
    addiu $t1,$t1,1
    sw $t1,64($t0)
    sw $v0,72($t0)
    sll $t1,$v0,0
    bgez $t1,3f
    li $t1,2
    li $t1,3
3:
    sw $t1,68($t0)
    li $t1,0x507920
    lw $t1,0($t1)
    sw $t1,76($t0)
    li $t1,0x527930
    lw $t1,0($t1)
    sw $t1,80($t0)
    OBS_END 56
    jr $ra
    nop

    .globl np_igr_alarm_observe
np_igr_alarm_observe:
    addiu $sp,$sp,-96
    sw $a0,32($sp)
    sq $ra,48($sp)
    jal 0x22e850
    nop
    sq $v0,64($sp)
    sq $v1,80($sp)
    OBS_STATE
    OBS_BEGIN 84
    lw $t1,88($t0)
    addiu $t1,$t1,1
    sw $t1,88($t0)
    lw $t1,32($sp)
    sw $t1,92($t0)
    sw $v0,96($t0)
    OBS_END 84
    lq $v0,64($sp)
    lq $v1,80($sp)
    lq $ra,48($sp)
    jr $ra
    addiu $sp,$sp,96
    /* Native serial handler is selected in v0 by POPS. Forward its untouched
     * arguments/delay-slot effects, then retain the full native return words. */
    .globl np_igr_serial_observe
np_igr_serial_observe:
    addiu $sp,$sp,-96
    sw $a0,32($sp)
    sw $a1,36($sp)
    sw $a2,40($sp)
    sw $v0,44($sp)
    sq $ra,48($sp)
    jalr $v0
    nop
    sq $v0,64($sp)
    sq $v1,80($sp)
    la $t0,np_igr_boundary_observation
    lui $t1,0x2000
    or $t0,$t0,$t1
    OBS_BEGIN 8
    lw $t1,12($t0)
    addiu $t1,$t1,1
    sw $t1,12($t0)
    lw $t1,44($sp)
    sw $t1,16($t0)
    lw $t1,32($sp)
    sw $t1,20($t0)
    lw $t1,36($sp)
    sw $t1,24($t0)
    lw $t1,40($sp)
    sw $t1,28($t0)
    sw $v0,32($t0)
    OBS_END 8
    lq $v0,64($sp)
    lq $v1,80($sp)
    lq $ra,48($sp)
    jr $ra
    addiu $sp,$sp,96

    .globl np_igr_event_observe
np_igr_event_observe:
    addiu $sp,$sp,-96
    sq $ra,48($sp)
    jal 0x200e98
    nop
    sq $v0,64($sp)
    sq $v1,80($sp)
    la $t0,np_igr_boundary_observation
    lui $t1,0x2000
    or $t0,$t0,$t1
    OBS_BEGIN 40
    lw $t1,44($t0)
    addiu $t1,$t1,1
    sw $t1,44($t0)
    li $t2,0x506d90
    lw $t1,20($t2)
    sw $t1,48($t0)
    lw $t1,24($t2)
    sw $t1,52($t0)
    lw $t1,8($t2)
    sw $t1,56($t0)
    lw $t1,4($t2)
    sw $t1,60($t0)
    lw $t1,-32496($gp)
    sw $t1,64($t0)
    lw $t1,-30672($gp)
    sw $t1,68($t0)
    lw $t1,-30680($gp)
    sw $t1,72($t0)
    lw $t1,-30612($gp)
    sw $t1,76($t0)
    li $t2,0x507920
    lw $t1,0($t2)
    sw $t1,80($t0)
    li $t2,0x527930
    lw $t1,0($t2)
    sw $t1,84($t0)
    OBS_END 40
#if NP_IGR_PROBE
    /* One diagnostic yield after dispatch, never a pause acknowledgement. */
    jal np_igr_probe_once
    nop
#endif
    lq $v0,64($sp)
    lq $v1,80($sp)
    lq $ra,48($sp)
    jr $ra
    addiu $sp,$sp,96
    .balign 16
    .globl np_igr_observation
np_igr_observation:
    .word 0x4f47504e,1
    .space 104
    .globl np_igr_boundary_observation
np_igr_boundary_observation:
    .word 0x4247504e,1
    .space 88
    .globl np_igr_observe_end
np_igr_observe_end:
#endif
