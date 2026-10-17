/* Capture RAM is four 1 KiB buffers sharing a 512-sample cursor. POPS
 * advances that cursor even when CD output is muted, but misses its IRQ.
 * Check the half-open interval about to advance, then use the existing
 * SPU latch/IRQ dispatcher. Voice, transfer and acknowledgement stay native.
 * Included in the inactive trace ring; no other resident addresses move. */
#if NP_SPU_CAPTURE_IRQ != 1
#error "SPU capture IRQ adapter accepts a boolean build flag only"
#endif
    .balign 16
    .globl np_spu_capture_irq,np_spu_capture_reset,np_spu_capture_end
np_spu_capture_irq:
    /* This is an inline native mixer site, not a C ABI call. Preserve all
     * live 128-bit GPRs and both packed HI/LO pairs across IRQ dispatch.
     * Leave argument space below the saves for the native callee. */
    addiu $sp,$sp,-576
    /* Most chunks need only classification. Delay the remaining wide
     * saves and packed HI/LO until the native dispatcher is required. */
    sq $v0,64($sp)
    sq $a0,96($sp)
    sq $a1,112($sp)
    /* Native mix chunks are 1..32 samples; reject unexpected state. */
    addiu $v0,$fp,-1
    sltiu $v0,$v0,32
    beqz $v0,.Lcapture_bypass
    nop
    lw $a0,0x26c($v1)
    sltiu $v0,$a0,512
    beqz $v0,.Lcapture_bypass
    nop
    lhu $v0,0x1aa($v1)
    andi $v0,$v0,0x8040
    ori $a1,$zero,0x8040
    bne $v0,$a1,.Lcapture_bypass
    nop
    lhu $v0,0x1a4($v1)
    sltiu $a1,$v0,0x200
    beqz $a1,.Lcapture_bypass
    nop
    /* IRQ units are eight bytes. Buffer selection does not change the
     * shared sample position: (address % 1024) / sizeof(int16_t). */
    andi $v0,$v0,0x7f
    sll $v0,$v0,2
    subu $v0,$v0,$a0
    andi $v0,$v0,0x1ff
    sltu $v0,$v0,$fp
    beqz $v0,.Lcapture_bypass
    nop
    .irp reg,1,3,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,30,31
    sq $\reg,(32+16*\reg)($sp)
    .endr
    pmfhi $v0
    sq $v0,544($sp)
    pmflo $v0
    sq $v0,560($sp)
    /* Native 0x210b38 overwrites v0 before reading it. Arguments and
     * all other live registers retain the original dispatch state. */
    jal 0x210b38
    nop
np_spu_capture_finish:
    lq $v0,544($sp)
    pmthi $v0
    lq $v0,560($sp)
    pmtlo $v0
    .irp reg,1,3,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,30,31
    lq $\reg,(32+16*\reg)($sp)
    .endr
.Lcapture_bypass:
    lq $v0,64($sp)
    lq $a0,96($sp)
    lq $a1,112($sp)
    addiu $sp,$sp,576
    /* Replay both displaced instructions. The native mask/store follow. */
    lw $v0,0x26c($v1)
    j 0x21146c
    addu $v0,$v0,$fp
    .balign 16
np_spu_capture_reset:
    /* Resetting the native event pool invalidates every timer handle. POPS
     * leaves the SPU handle pointing at a recycled event, so its subsequent
     * SPU initialization can skip enrolling the mixer. Clear that owner at
     * the pool reset, before the native SPU initializer runs. */
    sw $zero,-32200($gp)
    daddu $a0,$zero,$zero
    j 0x200ddc
    sw $zero,-30680($gp)
np_spu_capture_end:
