/* Exact source validation for POPS's decoded RAM instructions. */
    .section ._psxcore_code_guard,"ax",@progbits
    .set noreorder
    .set noat
    .balign 4
    .globl np_code_guard_begin,np_code_guard_compile
np_code_guard_begin:
np_code_guard_compile:
    /* The native emitter predates the EE C ABI: its live compiler temps
     * include registers that C may freely overwrite. Preserve them here. */
    addiu $sp,$sp,-192
    sq $8,0($sp)
    sq $9,16($sp)
    sq $10,32($sp)
    sq $11,48($sp)
    sq $12,64($sp)
    sq $13,80($sp)
    sq $14,96($sp)
    sq $15,112($sp)
    sq $24,128($sp)
    sq $25,144($sp)
    sq $ra,160($sp)
    mfhi $v0
    sd $v0,176($sp)
    mflo $v0
    sd $v0,184($sp)
    jal np_code_guard_emit
    move $a2,$s6
    ld $v1,176($sp)
    mthi $v1
    ld $v1,184($sp)
    mtlo $v1
    lq $8,0($sp)
    lq $9,16($sp)
    lq $10,32($sp)
    lq $11,48($sp)
    lq $12,64($sp)
    lq $13,80($sp)
    lq $14,96($sp)
    lq $15,112($sp)
    lq $24,128($sp)
    lq $25,144($sp)
    lq $ra,160($sp)
    beqz $v0,np_code_guard_reject
    addiu $sp,$sp,192
    jr $ra
    nop
    .globl np_code_guard_reject
np_code_guard_reject:
    b np_code_guard_reject
    nop
    /* Inline comparisons use only native guard scratch registers. Mismatch
     * exits retain POPS's original invalidation reason and handler. */
    .globl np_code_guard_validate
np_code_guard_validate:
    j 0x2081f0
    li $v0,6
    /* The optimizer can replace the raw opcode in a descriptor. Capture
     * the original decoder read before that happens, indexed by its slot. */
    .globl np_code_guard_capture
np_code_guard_capture:
    addiu $sp,$sp,-64
    sq $v0,0($sp)
    sq $v1,16($sp)
    sq $a0,32($sp)
    mfhi $v0
    sd $v0,48($sp)
    mflo $v0
    sd $v0,56($sp)
    lui $v0,0x0050
    addiu $v0,$v0,0x3b20
    subu $v1,$s0,$v0
    li $v0,48
    divu $zero,$v1,$v0
    mfhi $v0
    bnez $v0,np_code_guard_reject
    mflo $a0
    sltiu $v0,$a0,258
    beqz $v0,np_code_guard_reject
    sll $a0,$a0,2
    la $v0,np_code_guard_originals
    addu $v0,$v0,$a0
    sw $a1,0($v0)
    ld $v0,48($sp)
    mthi $v0
    ld $v0,56($sp)
    mtlo $v0
    lq $a0,32($sp)
    lq $v1,16($sp)
    lq $v0,0($sp)
    addiu $sp,$sp,64
    j 0x2041ac
    sw $s1,28($s0)
    .globl np_code_guard_asm_end
np_code_guard_asm_end:
    .section ._psxcore_code_guard_data,"aw",@progbits
    .balign 4
    .globl np_code_guard_originals
np_code_guard_originals:
    .space 258*4
