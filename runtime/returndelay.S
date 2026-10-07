/* PS1 v0 return-delay load cancellation. No game address or identity match.
 * Read through the existing helper even when the pending result is discarded.
 * Only the pinned constant-return/scalar JR-ra prefix selects these wrappers. */
    .section ._psxcore_return_delay,"ax",@progbits
    .set noreorder
    .set noat
    .balign 4
    .macro NP_RETURN_LOAD name,index
    .globl \name
\name:
    b np_memory_return_load
    li $a2,\index
    .endm
    NP_RETURN_LOAD np_memory_return_lb,0
    NP_RETURN_LOAD np_memory_return_lh,1
    NP_RETURN_LOAD np_memory_return_lw,3
    NP_RETURN_LOAD np_memory_return_lbu,4
    NP_RETURN_LOAD np_memory_return_lhu,5

np_memory_return_load:
    /* Slots are indexed by native register number. Preserve the full 128-bit
     * allocated values across both the native read and the C predicate. k1 is
     * deliberately not restored: native I/O cycle charges must survive. */
    addiu $sp,$sp,-544
    sq $at,16($sp)
    sq $v1,48($sp)
    sq $a0,64($sp)
    sq $a1,80($sp)
    sq $a2,96($sp)
    sq $a3,112($sp)
    .irp reg,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,30,31
    sq $\reg,(\reg*16)($sp)
    .endr
    sll $v0,$a2,2
    la $v1,np_memory_helpers
    addu $v1,$v1,$v0
    lw $v0,0($v1)
    jalr $v0
    nop
    sq $v0,512($sp)
    lw $8,0($gp)
    lw $at,16($sp)
    and $8,$8,$at
    andi $9,$8,3
    bnez $9,np_memory_return_loaded
    nop
    lui $9,0x0080
    sltu $9,$8,$9
    beqz $9,np_memory_return_loaded
    nop
    lui $9,0x001f
    ori $9,$9,0xffff
    and $8,$8,$9
    lui $9,0x4000
    or $8,$8,$9
    lw $a0,0($8)
    jal np_memory_return_load_cancelled
    nop
    beqz $v0,np_memory_return_loaded
    nop
    /* RA points to the checked generated move from v0 into its allocated
     * destination. Return that register's pre-load value on cancellation. */
    lw $8,496($sp)
    lw $8,0($8)
    srl $8,$8,11
    andi $8,$8,31
    sll $8,$8,4
    addu $8,$8,$sp
    lq $v0,0($8)
    la $8,np_memory_return_load_selected
    lw $9,0($8)
    addiu $9,$9,1
    sw $9,0($8)
    b np_memory_return_restore
    nop
np_memory_return_loaded:
    lq $v0,512($sp)
np_memory_return_restore:
    lq $at,16($sp)
    lq $v1,48($sp)
    lq $a0,64($sp)
    lq $a1,80($sp)
    lq $a2,96($sp)
    lq $a3,112($sp)
    .irp reg,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,30,31
    lq $\reg,(\reg*16)($sp)
    .endr
    jr $ra
    addiu $sp,$sp,544
    .balign 4
    .globl np_memory_return_helpers
np_memory_return_helpers:
    .word np_memory_return_lb,np_memory_return_lh,0,np_memory_return_lw
    .word np_memory_return_lbu,np_memory_return_lhu,0,0
    .globl np_memory_return_load_selected
np_memory_return_load_selected:
    .word 0
