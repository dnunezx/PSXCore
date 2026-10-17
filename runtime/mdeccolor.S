/* Exact native RGB24 work reduction. Reuse the three chroma-biased lookup
 * pointers and unroll four adjacent 2x2 cells. Lookup and byte-store order
 * stays B,G,R per pixel, including when a lookup aliases earlier output. */
    .set noreorder
    .set noat
    .balign 16
    .globl np_mdec_color_fast,np_mdec_color_end
np_mdec_color_fast:
    li $v0,0x5979c0
    bne $a0,$v0,.Lcolor_original
    nop
    li $v0,0x547940
    sltu $v0,$a1,$v0
    bnez $v0,.Lcolor_original
    nop
    li $v0,0x5976c0
    sltu $v0,$v0,$a1
    bnez $v0,.Lcolor_original
    nop
    addiu $sp,$sp,-128
    .irp reg,16,17,18,19,20,21,22,23
    sq $\reg,(16*(\reg-16))($sp)
    .endr
    move $s0,$a0
    move $s2,$zero
    li $14,0x59c080
    li $s4,7258
    li $s5,-1407
    li $s6,-2925
    li $s7,5742
.Lcolor_row:
    andi $v0,$s2,3
    sll $v0,$v0,5
    andi $v1,$s2,4
    sll $v1,$v1,6
    addu $s1,$s0,$v0
    addu $s1,$s1,$v1
    addiu $a2,$s1,256
    move $s3,$zero
.Lcolor_half:
    .macro COLOR_PIXEL y,out
    lh $12,\y($a2)
    addu $13,$10,$12
    lbu $13,0($13)
    sb $13,(\out+2)($a1)
    addu $13,$9,$12
    lbu $13,0($13)
    sb $13,(\out+1)($a1)
    addu $13,$8,$12
    lbu $13,0($13)
    sb $13,\out($a1)
    .endm
    .macro COLOR_CELL
    lh $v0,0($a0)
    lh $v1,128($a0)
    mult $10,$v1,$s7
    mult1 $11,$v1,$s6
    mult $9,$v0,$s5
    mult1 $8,$v0,$s4
    sra $10,$10,12
    sra $11,$11,12
    sra $9,$9,12
    sra $8,$8,12
    addu $9,$9,$11
    addu $8,$8,$14
    addu $9,$9,$14
    addu $10,$10,$14
    COLOR_PIXEL 0,0
    COLOR_PIXEL 2,3
    COLOR_PIXEL 16,48
    COLOR_PIXEL 18,51
    addiu $a0,$a0,2
    addiu $a1,$a1,6
    addiu $a2,$a2,4
    .endm
    .rept 4
    COLOR_CELL
    .endr
    addiu $s3,$s3,1
    sltiu $v0,$s3,2
    bnez $v0,.Lcolor_half
    addiu $a2,$a2,112
    addiu $s2,$s2,1
    sltiu $v0,$s2,8
    bnez $v0,.Lcolor_row
    addiu $a1,$a1,48
    .irp reg,16,17,18,19,20,21,22,23
    lq $\reg,(16*(\reg-16))($sp)
    .endr
    jr $ra
    addiu $sp,$sp,128
.Lcolor_original:
    addiu $sp,$sp,-48
    j 0x20da00
    lui $v0,0x5a
    .balign 16
np_mdec_color_end:
