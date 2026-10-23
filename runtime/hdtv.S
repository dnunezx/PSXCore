/* Output-only interlaced presentation. Installed only for NP_FIX_HDTV.
 * Original descriptors and PS1 GP1/status/field/timing state stay untouched.
 * The native GS builder consumes a private copy with INT=1, FFMD=FRAME:
 * native PAL/NTSC positioning and twice-height DISPLAY, same source pixels.
 * No per-primitive work, polling, allocation, or pixel conversion is added. */
    .set noreorder
    .set noat
    .section ._psxcore_hdtv,"ax",@progbits
    .balign 16
    .globl np_hdtv_begin,np_hdtv_display,np_hdtv_crt,np_hdtv_height,np_hdtv_end
np_hdtv_begin:
    .macro NP_HDTV_ENTRY name,crt
\name:
    addiu $sp,$sp,-224
    sq $ra,16($sp)
    sq $a0,32($sp)
    sq $v0,48($sp)
    sq $v1,64($sp)
    sq $8,80($sp)
    li $8,\crt
    b hdtv_prepare
    sw $8,96($sp)
    .endm
    NP_HDTV_ENTRY np_hdtv_display,0
    NP_HDTV_ENTRY np_hdtv_crt,1
hdtv_prepare:
    lw $v0,0x64($a0)
    bnez $v0,hdtv_dispatch
    nop
    lw $v0,0x60($a0)
    addiu $v0,$v0,-2
    sltiu $v0,$v0,2
    beqz $v0,hdtv_dispatch
    nop
    /* Descriptors are word aligned; buffered descriptors need not be
     * quadword aligned. Copy all 0x70 bytes using safe word accesses. */
    li $v1,108
hdtv_copy:
    addu $8,$a0,$v1
    lw $v0,0($8)
    addu $8,$sp,$v1
    sw $v0,112($8)
    addiu $v1,$v1,-4
    bgez $v1,hdtv_copy
    nop
    li $v0,1
    sw $v0,212($sp) /* copy + 0x64: output interlace */
    sw $v0,216($sp) /* copy + 0x68: read every source line */
    addiu $a0,$sp,112
hdtv_dispatch:
    lw $8,96($sp)
    bnez $8,hdtv_reset
    nop
    lq $v0,48($sp)
    lq $v1,64($sp)
    lq $8,80($sp)
    jal 0x219df8
    nop
    b hdtv_return
    nop
hdtv_reset:
    lq $v0,48($sp)
    lq $v1,64($sp)
    lq $8,80($sp)
    jal 0x21a490
    nop
hdtv_return:
    lq $a0,32($sp)
    lq $ra,16($sp)
    jr $ra
    addiu $sp,$sp,224
np_hdtv_height:
    addiu $v0,$v0,-1
    sllv $v0,$v0,$a0
    addiu $sp,$sp,-16
    sq $v1,0($sp)
    /* Pinned 0x219df8 saves 32 bytes. Our private copy is 112 bytes
     * above its caller's sp: native sp + 144 (plus this 16-byte save).
     * Native scratchpad descriptors never match this stack identity. */
    addiu $v1,$sp,160
    bne $s1,$v1,1f
    nop
    ori $v0,$v0,1
1:
    lq $v1,0($sp)
    addiu $sp,$sp,16
    j 0x21a060
    nop
np_hdtv_end:
