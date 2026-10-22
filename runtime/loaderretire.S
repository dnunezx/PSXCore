/* Rayman 2 USA executable handoff: retire old POPS translations once.
 * Installed only for SLUS_012.35 with exact Code Validation disabled.
 * All other source PCs retain the original native header generator. */
    .section ._psxcore_loader_retire.text,"ax",@progbits
    .set noreorder
    .set noat
    .balign 4
    .globl np_loader_retire_compile,np_loader_retire
np_loader_retire_compile:
    lui $v0,0x801e
    ori $v0,$v0,0x4000
    bne $a0,$v0,loader_native
    nop
    /* Pin the actual BIOS A0/43 wrapper before specializing this source PC.
     * Check only at translation time; do not scan game code during play. */
    lui $v0,0x011e
    lw $v1,0x4000($v0)
    lui $a2,0x240a
    ori $a2,$a2,0x00a0
    bne $v1,$a2,loader_native
    nop
    lw $v1,0x4004($v0)
    lui $a2,0x0140
    ori $a2,$a2,8
    bne $v1,$a2,loader_native
    nop
    lw $v1,0x4008($v0)
    lui $a2,0x2409
    ori $a2,$a2,0x0043
    bne $v1,$a2,loader_native
    nop
    lw $v1,0x400c($v0)
    bnez $v1,loader_native
    nop
    /* The pinned native emitter is a leaf and clobbers v0/v1, a0-a3,
     * t0-t3 only. Preserve its caller's full-width return address. */
    addiu $sp,$sp,-16
    sq $ra,0($sp)
    jal 0x208568
    nop
    lq $ra,0($sp)
    addiu $sp,$sp,16
    /* Append: load body continuation in a1; jump to one-shot retirement.
     * The installer reserves these additional 16 bytes before compilation. */
    addiu $a1,$v0,16
    srl $a0,$a1,16
    lui $v1,0x3c05
    or $a0,$a0,$v1
    sw $a0,0($v0)
    andi $a0,$a1,0xffff
    lui $v1,0x34a5
    or $a0,$a0,$v1
    sw $a0,4($v0)
    la $a0,np_loader_retire
    srl $a0,$a0,2
    lui $v1,0x0800
    or $a0,$a0,$v1
    sw $a0,8($v0)
    sw $zero,12($v0)
    jr $ra
    addiu $v0,$v0,16
loader_native:
    j 0x208568
    nop
np_loader_retire:
    /* Native reason 6 clears translations and retries the current source PC.
     * The resident latch lets that retry enter the new body exactly once. */
    la $a0,np_loader_retire_retry
    lw $v0,0($a0)
    bnez $v0,loader_retry
    li $v0,1
    sw $v0,0($a0)
    j 0x2081f0
    li $v0,6
loader_retry:
    sw $zero,0($a0)
    jr $a1
    nop
    .section ._psxcore_loader_retire.data,"aw",@progbits
    .balign 4
    .globl np_loader_retire_retry
np_loader_retire_retry:
    .word 0
