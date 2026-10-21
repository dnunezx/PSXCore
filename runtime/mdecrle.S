/* General RLE work reduction with six-bit zero runs. Runs past the block
 * terminate without a coefficient store; preserve native padding handling. */
    .set noreorder
    .set noat
    .balign 16
    .globl np_mdec_rle_general,np_mdec_rle_general_end
np_mdec_rle_general:
    li $v0,0x597940
    bne $a1,$v0,.Lrle_general_original
    andi $v0,$a0,1
    bnez $v0,.Lrle_general_original
    nop
    .irp offset,0,16,32,48,64,80,96,112
    sq $zero,\offset($a1)
    .endr
    lhu $v1,0($a0)
    addiu $v0,$a0,2
    srl $9,$v1,10
    sw $9,0($a2)
    sll $9,$v1,22
    sra $9,$9,22
    sh $9,0($a1)
    li $a2,0x4f9a18
    ori $8,$zero,0xfe00
    li $a3,1
.Lrle_general_token:
    lhu $a0,0($v0)
    addiu $v0,$v0,2
    beq $a0,$8,.Lrle_general_padding
    srl $9,$a0,10
    /* LHU followed by SRL 10 already yields the complete six-bit field. */
    addu $a3,$a3,$9
    sltiu $9,$a3,64
    beqz $9,.Lrle_general_padding
    addu $12,$a2,$a3
    lbu $12,0($12)
    sll $10,$a0,22
    sra $10,$10,22
    sll $12,$12,1
    addu $12,$a1,$12
    addiu $a3,$a3,1
    sltiu $9,$a3,64
    bnez $9,.Lrle_general_token
    sh $10,0($12)
.Lrle_general_padding:
    lhu $9,0($v0)
    bne $9,$8,.Lrle_general_return
    nop
    b .Lrle_general_padding
    addiu $v0,$v0,2
.Lrle_general_return:
    jr $ra
    nop
.Lrle_general_original:
    addiu $sp,$sp,-32
    j 0x20d2b0
    sd $s2,16($sp)
    /* Keep following resident helper addresses stable after shortening the
     * executable path. These bytes follow an unconditional native redirect. */
    .space 8
    .balign 16
np_mdec_rle_general_end:
