/* Original shortcuts for the checksum-pinned POPS MDEC decoder.
 * These change computation cost only: scheduling, DMA and clocks stay native.
 * Shortcuts guard their qualifying inputs; general paths retain native read
 * and store order. Unsupported inputs replay the native prologue. No tracing
 * or mutable state. */
    .balign 16
    .globl np_mdec_fast_begin
np_mdec_fast_begin:
/* Exact DC-only IDCT shortcut for the pinned POPS decoder.
 * Every AC coefficient must be zero. Otherwise replay the native prologue.
 * The native two-pass transform reduces to (DC * quant[0]) >> 6;
 * preserve its signed arithmetic and final halfword truncation. */
    .set noreorder
    .set noat
    .balign 16
    .globl np_mdec_dc,np_mdec_dc_end
np_mdec_dc:
    or $v0,$a0,$a2
    andi $v0,$v0,7
    bnez $v0,np_mdec_dc_native
    nop
    ld $t0,0($a0)
    dsrl $t0,$t0,16
    ld $t1,8($a0)
    or $t0,$t0,$t1
    bnez $t0,np_mdec_dc_native
    nop
    .irp off,16,24,32,40
    ld $t1,\off($a0)
    or $t0,$t0,$t1
    .endr
    bnez $t0,np_mdec_dc_native
    nop
    .irp off,48,56,64,72
    ld $t1,\off($a0)
    or $t0,$t0,$t1
    .endr
    bnez $t0,np_mdec_dc_native
    nop
    .irp off,80,88,96,104,112,120
    ld $t1,\off($a0)
    or $t0,$t0,$t1
    .endr
    bnez $t0,np_mdec_dc_native
    nop
    lh $v0,0($a0)
    lh $v1,0($a1)
    mult $v0,$v0,$v1
    sra $v0,$v0,6
    andi $v0,$v0,0xffff
    dsll $v1,$v0,16
    or $v0,$v0,$v1
    dsll32 $v1,$v0,0
    or $v0,$v0,$v1
    .irp off,0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120
    sd $v0,\off($a2)
    .endr
    jr $ra
    nop
np_mdec_dc_native:
    addiu $sp,$sp,-336
    j 0x20d3c0
    sd $s0,256($sp)
    .balign 16
np_mdec_dc_end:

/* Constant Cr/Cb and four constant Y blocks retain the native lookup and
 * separately rounded green products. Bound values so lookup reads cannot
 * alias either the YUV source or RGB destination while quadrants are filled.
 * RGB repeats every three bytes: three doublewords encode eight pixels. */
    .set noreorder
    .set noat
    .balign 16
    .globl np_mdec_flat,np_mdec_flat_end
np_mdec_flat:
    lui $v0,0x59
    ori $v0,$v0,0x79c0
    bne $a0,$v0,np_mdec_flat_native
    andi $v0,$a1,7
    bnez $v0,np_mdec_flat_native
    lui $v0,0x54
    ori $v0,$v0,0x7940
    sltu $v0,$a1,$v0
    bnez $v0,np_mdec_flat_native
    lui $v0,0x59
    ori $v0,$v0,0x76c0
    sltu $v0,$v0,$a1
    bnez $v0,np_mdec_flat_native
    move $v0,$a0
    addiu $a2,$a0,768
np_mdec_flat_check:
    lh $8,0($v0)
    addiu $9,$8,1024
    sltiu $9,$9,2048
    beqz $9,np_mdec_flat_native
    andi $8,$8,0xffff
    dsll $9,$8,16
    or $8,$8,$9
    dsll32 $9,$8,0
    or $8,$8,$9
    .irp off,0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120
    ld $9,\off($v0)
    bne $8,$9,np_mdec_flat_native
    nop
    .endr
    addiu $v0,$v0,128
    bne $v0,$a2,np_mdec_flat_check
    nop
    lh $8,0($a0)
    lh $9,128($a0)
    li $10,7258
    mult $12,$8,$10
    sra $12,$12,12
    li $10,-1407
    mult $13,$8,$10
    sra $13,$13,12
    li $10,-2925
    mult $11,$9,$10
    sra $11,$11,12
    addu $13,$13,$11
    li $10,5742
    mult $14,$9,$10
    sra $14,$14,12
    lui $15,0x59
    ori $15,$15,0xc080
    addu $12,$12,$15
    addu $13,$13,$15
    addu $14,$14,$15
    addiu $a3,$a0,256
    move $v1,$zero
np_mdec_flat_quadrant:
    lh $8,0($a3)
    addu $9,$8,$12
    addu $10,$8,$13
    addu $11,$8,$14
    lbu $8,0($9)
    lbu $9,0($10)
    lbu $10,0($11)
    sll $9,$9,8
    sll $10,$10,16
    or $8,$8,$9
    or $8,$8,$10
    dsll $9,$8,24
    or $9,$9,$8
    dsll32 $10,$8,16
    or $9,$9,$10
    dsrl $10,$8,16
    dsll $11,$8,8
    or $10,$10,$11
    dsll32 $11,$8,0
    or $10,$10,$11
    dsll32 $11,$8,24
    or $10,$10,$11
    dsrl $11,$8,8
    dsll $15,$8,16
    or $11,$11,$15
    dsll32 $15,$8,8
    or $11,$11,$15
    andi $15,$v1,1
    sll $24,$15,4
    sll $15,$15,3
    addu $24,$24,$15
    andi $15,$v1,2
    sll $25,$15,7
    sll $15,$15,6
    addu $24,$24,$25
    addu $24,$24,$15
    addu $24,$24,$a1
    li $25,8
np_mdec_flat_row:
    sd $9,0($24)
    sd $10,8($24)
    sd $11,16($24)
    addiu $25,$25,-1
    bnez $25,np_mdec_flat_row
    addiu $24,$24,48
    addiu $v1,$v1,1
    sltiu $8,$v1,4
    bnez $8,np_mdec_flat_quadrant
    addiu $a3,$a3,128
    jr $ra
    nop
/* Achromatic macroblocks need one lookup per pixel. Keep the destination
 * below every possible signed-halfword lookup address, so repeated native
 * lookups cannot observe our output stores. Input and output stay disjoint. */
np_mdec_flat_native:
    j np_mdec_chroma_select
    nop
    nop
    andi $v0,$a1,7
    bnez $v0,np_mdec_flat_original
    lui $v0,0x54
    ori $v0,$v0,0x7940
    sltu $v0,$a1,$v0
    bnez $v0,np_mdec_flat_original
    lui $v0,0x59
    ori $v0,$v0,0x3d00
    sltu $v0,$v0,$a1
    bnez $v0,np_mdec_flat_original
    move $8,$zero
    .irp off,0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120,128,136,144,152,160,168,176,184,192,200,208,216,224,232,240,248
    ld $9,\off($a0)
    or $8,$8,$9
    .endr
    bnez $8,np_mdec_flat_original
    lui $9,0x59
    ori $9,$9,0xc080
    addiu $a3,$a0,256
    move $v1,$zero
np_mdec_gray_quadrant:
    andi $10,$v1,1
    sll $11,$10,4
    sll $10,$10,3
    addu $11,$11,$10
    andi $10,$v1,2
    sll $12,$10,7
    sll $10,$10,6
    addu $11,$11,$12
    addu $11,$11,$10
    addu $11,$11,$a1
    li $10,8
np_mdec_gray_row:
    .irp col,0,1,2,3,4,5,6,7
    lh $8,2*\col($a3)
    addu $8,$8,$9
    lbu $v0,0($8)
    sb $v0,3*\col($11)
    sb $v0,3*\col+1($11)
    sb $v0,3*\col+2($11)
    .endr
    addiu $a3,$a3,16
    addiu $10,$10,-1
    bnez $10,np_mdec_gray_row
    addiu $11,$11,48
    addiu $v1,$v1,1
    sltiu $8,$v1,4
    bnez $8,np_mdec_gray_quadrant
    nop
    jr $ra
    nop
np_mdec_flat_original:
    j np_mdec_color_fast
    nop
    nop
    .balign 16
np_mdec_flat_end:

/* The native RLE caller clears this exact coefficient workspace. */
    .balign 16
    .globl np_mdec_clear
np_mdec_clear:
    lui $v0,0x59
    ori $v0,$v0,0x7940
    bne $a0,$v0,np_mdec_clear_native
    addiu $v0,$a1,-128
    bnez $v0,np_mdec_clear_native
    nop
    .irp off,0,16,32,48,64,80,96,112
    sq $zero,\off($a0)
    .endr
    jr $ra
    nop
np_mdec_clear_native:
    j 0x209ef0
    nop

/* DC-only RLE blocks need no zigzag walk. Read the qualifying tokens only
 * when they cannot overlap the workspace the native parser clears first.
 * Preserve the quantizer store, signed ten-bit DC and padding-token skip. */
    .balign 16
    .globl np_mdec_rle_dc
np_mdec_rle_dc:
    lui $v0,0x59
    ori $v0,$v0,0x7940
    bne $a1,$v0,np_mdec_rle_native
    andi $v1,$a0,1
    bnez $v1,np_mdec_rle_native
    addiu $v1,$a0,4
    sltu $v1,$v0,$v1
    beqz $v1,np_mdec_rle_tokens
    addiu $v0,$v0,128
    sltu $v1,$a0,$v0
    bnez $v1,np_mdec_rle_native
    nop
np_mdec_rle_tokens:
    lhu $v0,2($a0)
    ori $v1,$zero,0xfe00
    bne $v0,$v1,np_mdec_rle_native
    nop
    .irp off,0,16,32,48,64,80,96,112
    sq $zero,\off($a1)
    .endr
    lhu $v0,0($a0)
    srl $v1,$v0,10
    sw $v1,0($a2)
    sll $v0,$v0,22
    sra $v0,$v0,22
    sh $v0,0($a1)
    addiu $v0,$a0,4
    ori $v1,$zero,0xfe00
np_mdec_rle_padding:
    lhu $8,0($v0)
    bne $8,$v1,np_mdec_rle_return
    nop
    b np_mdec_rle_padding
    addiu $v0,$v0,2
np_mdec_rle_return:
    jr $ra
    nop
np_mdec_rle_native:
    j np_mdec_rle_general
    nop
    nop
    .balign 16
    .globl np_mdec_fast_end
np_mdec_fast_end:
