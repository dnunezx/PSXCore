/* Original bounded transfer; no caller memory, libc or caller stack after entry. */
    .section ._psxcore_transfer,"ax",@progbits
    .set noreorder
    .set noat
    .globl np_runtime_transfer
    .ent np_runtime_transfer
np_runtime_transfer:
    lui $sp,0x01f0
    addiu $sp,$sp,-64
    move $s0,$a0
    jal DIntr
    nop
    lw $s1,4($s0)
    addiu $s2,$s0,8
1:
    lw $t0,0($s2)
    lui $t1,0x0180
    addu $t0,$t0,$t1
    lw $t2,4($s2)
    lw $t3,8($s2)
    lw $8,12($s2)
    subu $8,$8,$t3
2:
    beqz $t3,3f
    nop
    lbu $9,0($t0)
    sb $9,0($t2)
    addiu $t0,$t0,1
    addiu $t2,$t2,1
    addiu $t3,$t3,-1
    b 2b
    nop
3:
    beqz $8,4f
    nop
    sb $zero,0($t2)
    addiu $t2,$t2,1
    addiu $8,$8,-1
    b 3b
    nop
4:
    addiu $s2,$s2,16
    addiu $s1,$s1,-1
    bnez $s1,1b
    nop
    jal FlushCache
    move $a0,$zero
    jal FlushCache
    li $a0,2
    lw $a0,0($s0)
    move $a1,$zero /* SDK crt0 initializes the child's gp. */
    lw $a2,136($s0)
    addiu $a3,$s0,140 /* NpElf: 8 + 8*16. */
    jal _ExecPS2
    nop
5:
    b 5b
    nop
    .end np_runtime_transfer
    .globl np_runtime_transfer_end
np_runtime_transfer_end:
