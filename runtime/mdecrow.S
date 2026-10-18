/* Exact horizontal-only IDCT for the checksum-pinned native decoder.
 * The fixed coefficient buffer must have zero vertical AC coefficients.
 * Its column pass then repeats eight dequantized words across all rows:
 * calculate the native AAN horizontal pass once, retaining word wrapping,
 * signed shifts and halfword truncation. Emit all eight rows in native order.
 * Native rotation constants are 5792, 7568, -10703 and 4433. The final multiply
 * operands restore both HI/LO units. The bridge preserves full at and ra;
 * scalar callee saves retain their upper lanes. Unsupported inputs replay
 * the original native prologue. No rate, DMA or event accounting changes.
 * This bounded trace reservation is available only with memory tracing off.
 */
    .set push
    .set push
    .set reorder
    .set at
	.align	2
	.globl	np_mdec_row_exact
	.set	nomips16
	.set	nomicromips
	.ent	np_mdec_row_exact
	.type	np_mdec_row_exact, @function
np_mdec_row_exact:
	.frame	$sp,48,$31		# vars= 48, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	addiu	$sp,$sp,-48
	addiu	$7,$4,16
	sw	$sp,32($sp)
.L2:
 #APP
 # 5 "idctexact.c" 1
	lh $3,0($4)
 # 0 "" 2
 # 5 "idctexact.c" 1
	lh $2,0($5)
 # 0 "" 2
 #NO_APP
	mult	$2,$2,$3
	lw	$3,32($sp)
	addiu	$3,$3,4
	addiu	$4,$4,2
	addiu	$5,$5,2
	sw	$3,32($sp)
	sw	$2,-4($3)
	bne	$4,$7,.L2
	lw	$3,12($sp)
	li	$13,-10703			# 0xffffffffffffd631
	lw	$2,28($sp)
	lw	$9,20($sp)
	lw	$5,4($sp)
	subu	$4,$9,$3
	subu	$10,$5,$2
	addu	$5,$5,$2
	addu	$15,$4,$10
	li	$2,7568			# 0x1d90
	addu	$9,$9,$3
	lw	$8,24($sp)
	mult	$15,$15,$2
	lw	$12,8($sp)
	li	$3,5792			# 0x16a0
	lw	$2,0($sp)
	mult	$4,$4,$13
	subu	$7,$12,$8
	subu	$11,$5,$9
	lw	$24,16($sp)
	li	$13,4433			# 0x1151
	addu	$12,$12,$8
	mult	$7,$7,$3
	sra	$15,$15,12
	sra	$4,$4,12
	mult	$8,$11,$3
	addu	$9,$9,$5
	subu	$3,$2,$24
	mult	$13,$10,$13
	addu	$4,$4,$15
	subu	$14,$9,$4
	sra	$7,$7,12
	subu	$5,$3,$12
	sra	$8,$8,12
	addu	$5,$5,$7
	sra	$13,$13,12
	addu	$8,$8,$14
	addu	$2,$2,$24
	addu	$3,$12,$3
	subu	$13,$13,$15
	subu	$3,$3,$7
	addu	$13,$13,$8
	addu	$7,$12,$2
	addu	$4,$5,$4
	subu	$2,$2,$12
	subu	$4,$4,$9
	addu	$12,$9,$7
	addu	$5,$5,$14
	subu	$7,$7,$9
	addu	$9,$3,$8
	subu	$3,$3,$8
	addu	$8,$13,$2
	subu	$2,$2,$13
	sra	$12,$12,6
	sra	$7,$7,6
	sra	$4,$4,6
	sra	$5,$5,6
	sra	$9,$9,6
	sra	$3,$3,6
	sra	$8,$8,6
	sra	$2,$2,6
	addiu	$13,$6,128
.L3:
	sh	$12,0($6)
	addiu	$6,$6,16
	sh	$7,-2($6)
	sh	$4,-14($6)
	sh	$5,-4($6)
	sh	$9,-12($6)
	sh	$3,-6($6)
	sh	$8,-8($6)
	sh	$2,-10($6)
	bne	$6,$13,.L3
	li	$2,4433			# 0x1151
	li	$3,5792			# 0x16a0
 #APP
 # 38 "idctexact.c" 1
	mult $zero,$10,$2
	mult1 $zero,$11,$3
 # 0 "" 2
 #NO_APP
	.set	noreorder
	.set	nomacro
	jr	$31
	addiu	$sp,$sp,48
	.set	macro
	.set	reorder

	.end	np_mdec_row_exact
	.size	np_mdec_row_exact, .-np_mdec_row_exact
    .set pop

    .set noreorder
    .set noat
    .balign 16
    .globl np_mdec_row_bridge,np_mdec_row_end
np_mdec_row_bridge:
    li $v0,0x597940
    bne $a0,$v0,.Lidct_original
    nop
    lq $8,16($a0)
    lq $9,32($a0)
    por $8,$8,$9
    pcpyud $9,$8,$zero
    or $8,$8,$9
    bnez $8,.Lidct_original
    nop
    lq $8,48($a0)
    lq $9,64($a0)
    por $8,$8,$9
    pcpyud $9,$8,$zero
    or $8,$8,$9
    bnez $8,.Lidct_original
    nop
    lq $8,80($a0)
    lq $9,96($a0)
    por $8,$8,$9
    lq $9,112($a0)
    por $8,$8,$9
    pcpyud $9,$8,$zero
    or $8,$8,$9
    bnez $8,.Lidct_original
    nop
    li $v0,0x597d60
    subu $v0,$a1,$v0
    sltiu $v1,$v0,16384
    beqz $v1,.Lidct_original
    andi $v0,$v0,127
    bnez $v0,.Lidct_original
    nop
    li $v0,0x5979c0
    subu $v0,$a2,$v0
    sltiu $v1,$v0,768
    beqz $v1,.Lidct_original
    andi $v0,$v0,127
    bnez $v0,.Lidct_original
    nop
    addiu $sp,$sp,-32
    sq $1,0($sp)
    sq $ra,16($sp)
    jal np_mdec_row_exact
    nop
    lq $1,0($sp)
    lq $ra,16($sp)
    jr $ra
    addiu $sp,$sp,32
.Lidct_original:
    addiu $sp,$sp,-336
    j 0x20d3c0
    sd $s0,256($sp)
    .balign 16
np_mdec_row_end:

    .set pop
