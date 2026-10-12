/* Default-off scheduler-tail probe. No admission gate or media-change action. */
#if NP_CD_RENDEZVOUS
    .section ._psxcore_cd_rendezvous,"ax",@progbits
    .set noreorder
    .set noat
    .balign 16
    .globl np_cd_rendezvous_begin
np_cd_rendezvous_begin:
    .globl np_cd_rendezvous_tail
np_cd_rendezvous_tail:
    /* Original epilogue delay slot has already restored its native frame.
     * The probe neither calls native helpers nor modifies native state. */
    addiu $sp,$sp,-64
    sq $t0,0($sp)
    sq $t1,16($sp)
    sq $t2,32($sp)
    sq $t3,48($sp)
    la $t0,np_cd_rendezvous_observation
    lui $t1,0x2000
    or $t0,$t0,$t1
    lw $t1,16($t0)
    andi $t2,$t1,1
    bnez $t2,8f
    nop
    addiu $t1,$t1,1
    sw $t1,16($t0)
    sync
    lw $t1,20($t0)
    li $t2,-1
    beq $t1,$t2,1f
    nop
    addiu $t1,$t1,1
    b 2f
    sw $t1,20($t0)
1:
    li $t1,1
    sw $t1,24($t0)
2:
    /* Main-thread fields are read after cancellation cleanup. Other EE/IOP
     * owners can progress between reads; this record is not exclusive. */
    lui $t1,0x0050
    lw $t2,0x2ea0($t1)
    sw $t2,32($t0)
    lui $t1,0x7000
    addiu $t1,$t1,0x10a0
    lw $t2,40($t1)
    sw $t2,36($t0)
    lw $t2,44($t1)
    sw $t2,40($t0)
    lw $t2,48($t1)
    sw $t2,44($t0)
    lw $t2,52($t1)
    sw $t2,48($t0)
    lw $t2,56($t1)
    sw $t2,52($t0)
    lui $t1,0x0085
    lw $t2,0x0978($t1)
    sw $t2,56($t0)
    lw $t2,0x0974($t1)
    sw $t2,60($t0)
    lw $t2,0x0970($t1)
    sw $t2,64($t0)
    lui $t1,0x0086
    lw $t2,-0x5580($t1)
    sw $t2,100($t0)
    lw $t2,-0x557c($t1)
    sw $t2,104($t0)
    lui $t1,0x0050
    lw $t2,0x2f00($t1)
    sw $t2,68($t0)
    lw $t2,0x2f04($t1)
    sw $t2,72($t0)
    lw $t2,0x2898($t1)
    sw $t2,76($t0)
    lw $t2,0x6cdc($t1)
    sw $t2,80($t0)
    lw $t2,0x6ccc($t1)
    sw $t2,84($t0)
    lw $t2,0x6c40($t1)
    sw $t2,88($t0)
    lw $t2,0x2ef8($t1)
    sw $t2,92($t0)
    lw $t2,0x2ee8($t1)
    sw $t2,96($t0)
    lw $t2,0x7920($t1)
    sw $t2,108($t0)
    lw $t2,0x2e98($t1)
    sw $t2,120($t0)
    lui $t1,0x0052
    lw $t2,0x7930($t1)
    sw $t2,112($t0)
    lui $t1,0x005a
    lw $t2,-0x097c($t1)
    sw $t2,116($t0)
    sync
    lw $t1,16($t0)
    addiu $t1,$t1,1
    sw $t1,16($t0)
    b 9f
    nop
8:
    /* Sticky rejection: an unexpected nested writer cannot appear idle. */
    li $t1,1
    sw $t1,28($t0)
9:
    lq $t0,0($sp)
    lq $t1,16($sp)
    lq $t2,32($sp)
    lq $t3,48($sp)
    jr $ra
    addiu $sp,$sp,64
    .balign 16
    .globl np_cd_rendezvous_observation
np_cd_rendezvous_observation:
    .word 0x42524443,1,128,0
    .space 112
    .globl np_cd_rendezvous_end
np_cd_rendezvous_end:
#endif
