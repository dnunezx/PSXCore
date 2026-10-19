    .set push
/* Exact guest copy for the verified 640x1, 8-byte-aligned upload.
 * The .Luc_fallback FIFO accounting, native publication, waits and display work
 * remain after this copy. No memory outside the admitted payload is read. */
    .set noreorder
    .set noat
    .balign 16
    .globl np_gp0_upload_copy,np_gp0_upload_copy_end
np_gp0_upload_copy:
    andi $v0,$a0,15
    li $v1,8
    bne $v0,$v1,.Luc_fallback
    nop
    li $v0,320
    bne $a3,$v0,.Luc_fallback
    nop
    li $v0,0x20868600
    bne $s0,$v0,.Luc_fallback
    nop
    li $v0,0x40000000
    subu $v0,$a0,$v0
    li $v1,0x1ffb01
    sltu $v0,$v0,$v1
    beqz $v0,.Luc_fallback
    nop
    addiu $sp,$sp,-16
    sq $8,0($sp)
    move $v1,$a0
    li $a0,320
.Lupload_copy:
    ld $v0,0($v1)
    ld $8,8($v1)
    pcpyld $s1,$8,$v0
    sq $s1,0($s0)
    addiu $s0,$s0,16
    addiu $v1,$v1,16
    addiu $a0,$a0,-4
    bnez $a0,.Lupload_copy
    nop
    sw $v1,16($sp)
    dsra32 $v0,$8,0
    lq $8,0($sp)
    addiu $sp,$sp,16
    mtsab $zero,4
    j 0x224454
    nop
.Luc_fallback:
    andi $v0,$a0,15
    bnez $v0,.Luc_unaligned
    move $v1,$a0
    j 0x224350
    nop
.Luc_unaligned:
    j 0x22436c
    nop
np_gp0_upload_copy_end:

    .set pop
