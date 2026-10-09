/* Flush the linear prefix before POPS publishes an internal loop entry.
 * Native s3 is the compiler's pending guest-cycle count. Without this flush,
 * a backward branch repeats the prefix charge together with the loop body. */
    .section ._psxcore_hooks,"ax",@progbits
    .set noreorder
    .set noat
    .globl np_cpu_loop_cycle_boundary,np_cpu_loop_cycle_boundary_end
np_cpu_loop_cycle_boundary:
    addiu $sp,$sp,-16
    sq $ra,0($sp)
    jal 0x206ca8
    nop
    beqz $s3,1f
    nop
    /* Native blocks have at most 1024 source instructions. Keep the emitted
     * signed immediate bounded rather than truncating an unexpected count. */
    addiu $v1,$s3,-1
    sltiu $v1,$v1,0x7fff
    beqz $v1,1f
    nop
    lui $a0,0x10
    sltu $v1,$v0,$a0
    bnez $v1,1f
    nop
    lui $a0,0x200
    sltu $v1,$v0,$a0
    beqz $v1,1f
    nop
    andi $v1,$v0,3
    bnez $v1,1f
    nop
    subu $v1,$zero,$s3
    andi $v1,$v1,0xffff
    lui $a0,0x277b
    or $v1,$v1,$a0
    sw $v1,0($v0)
    addiu $v0,$v0,4
    move $s3,$zero
1:
    lq $ra,0($sp)
    jr $ra
    addiu $sp,$sp,16
np_cpu_loop_cycle_boundary_end:
