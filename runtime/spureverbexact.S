    .pushsection ._psxcore_reverb_bridge,"ax",@progbits
/* Preserve native ABI and route unsupported counts through the original leaf. */
    .set noreorder
    .set noat
    .balign 16
    .globl np_reverb_recompile,np_reverb_recompile_end
np_reverb_recompile:
    addiu $v0,$a0,-1
    sltiu $v0,$v0,32
    beqz $v0,1f
    nop
    addiu $sp,$sp,-32
    sq $1,0($sp)
    sq $ra,16($sp)
    jal np_reverb_exact32
    nop
    bnez $v0,np_spu_reverb_tail
    nop
    lq $1,0($sp)
    lq $ra,16($sp)
    jr $ra
    addiu $sp,$sp,32
1:
    addiu $sp,$sp,-32
    sq $1,0($sp)
    j np_spu_reverb_call+8
    nop
    .balign 16
np_reverb_recompile_end:
    .popsection
