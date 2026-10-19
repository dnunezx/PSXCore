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
    addiu $sp,$sp,-32
    sq $8,0($sp)
    sq $9,16($sp)
    move $v1,$a0
    li $a0,3
    /* Eight initial bytes, then aligned source quadwords. Every source
     * byte is read once; the last eight bytes use an exact LD as well. */
    ld $v0,0($v1)
    lq $8,8($v1)
    pcpyld $s1,$8,$v0
    sq $s1,0($s0)
    addiu $s0,$s0,16
    addiu $v1,$v1,24
    mtsab $zero,8
.Lupload_copy:
    /* Three groups of 26 quadwords: 78 middle outputs in native order. */
    .irp offset,0,32,64,96,128,160,192,224,256,288,320,352,384
    lq $9,\offset($v1)
    qfsrv $s1,$9,$8
    sq $s1,\offset($s0)
    lq $8,\offset+16($v1)
    qfsrv $s1,$8,$9
    sq $s1,\offset+16($s0)
    .endr
    addiu $s0,$s0,416
    addiu $v1,$v1,416
    addiu $a0,$a0,-1
    bnez $a0,.Lupload_copy
    nop
    ld $9,0($v1)
    qfsrv $s1,$9,$8
    sq $s1,0($s0)
    addiu $s0,$s0,16
    addiu $v1,$v1,8
    sw $v1,32($sp)
    dsra32 $v0,$9,0
    lq $8,0($sp)
    lq $9,16($sp)
    addiu $sp,$sp,32
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
