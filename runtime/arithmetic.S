/* Compiler and ABI bridges for guest arithmetic. Retain full 128-bit callee
 * registers, gp and link; C uses no loader globals or small-data addressing. */
    .set noreorder
    .set noat
    .section ._psxcore_arithmetic,"ax",@progbits
    .balign 16
    .globl np_arithmetic_compile
np_arithmetic_compile:
    andi $v0,$s0,63
    li $v1,1
    beq $v0,$v1,.Lemit
    li $v1,6
    beq $v0,$v1,.Lemit
    li $v1,0x11
    beq $v0,$v1,.Lemit
    li $v1,0x12
    beq $v0,$v1,.Lemit
    li $v1,0x2d
    beq $v0,$v1,.Lemit
    li $v1,0x2e
    beq $v0,$v1,.Lemit
    li $v1,0x30
    beq $v0,$v1,.Lemit
    li $v1,0x3d
    beq $v0,$v1,.Lemit
    li $v1,0x3e
    beq $v0,$v1,.Lemit
    nop
    j 0x2067dc
    addiu $v0,$v0,-1
.Lemit:
    srl $v0,$s0,16
    lui $v1,0x3c05
    or $v0,$v0,$v1
    sw $v0,0($s1)
    andi $v0,$s0,65535
    lui $v1,0x34a5
    or $v0,$v0,$v1
    sw $v0,4($s1)
    addiu $s1,$s1,8
    la $v0,np_gte_arithmetic
    srl $v0,$v0,2
    lui $v1,0x0c00
    or $v0,$v0,$v1
    li $v1,0x278401b0
    j 0x206c80
    nop

    .macro NP_SAVE
    addiu $sp,$sp,-272
    sq $1,16($sp)
    sq $16,32($sp)
    sq $17,48($sp)
    sq $18,64($sp)
    sq $19,80($sp)
    sq $20,96($sp)
    sq $21,112($sp)
    sq $22,128($sp)
    sq $23,144($sp)
    sq $gp,160($sp)
    sq $fp,176($sp)
    sq $ra,192($sp)
    .endm
    .macro NP_RESTORE
    lq $1,16($sp)
    lq $16,32($sp)
    lq $17,48($sp)
    lq $18,64($sp)
    lq $19,80($sp)
    lq $20,96($sp)
    lq $21,112($sp)
    lq $22,128($sp)
    lq $23,144($sp)
    lq $gp,160($sp)
    lq $fp,176($sp)
    lq $ra,192($sp)
    addiu $sp,$sp,272
    .endm
    .balign 16
    .globl np_gte_arithmetic
np_gte_arithmetic:
    NP_SAVE
    jal np_gte_arithmetic_c
    nop
    NP_RESTORE
    jr $ra
    nop
    .balign 16
    .globl np_mdec_arithmetic
np_mdec_arithmetic:
    NP_SAVE
    sq $a0,208($sp)
    sq $a1,224($sp)
    sq $a2,240($sp)
    jal np_mdec_arithmetic_c
    nop
    lq $a1,224($sp)
    lq $a2,240($sp)
    lq $a0,208($sp)
    move $3,$v0
    NP_RESTORE
    bnez $3,.Lmdec_done
    nop
#if NP_MDEC_BLOCK_FASTPATH
    j np_mdec_dc
    nop
#else
    addiu $sp,$sp,-336
    j 0x20d3c0
    sd $s0,256($sp)
#endif
.Lmdec_done:
    jr $ra
    nop
