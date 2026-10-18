/* Original, opt-in POPS producer adapter. Never buffers or edits commands.
 * Defer only incomplete fixed drawing packets or A0 upload headers in the
 * native buffer. Completed headers, pixel data, DMA, polylines and unknown
 * states always wake natively.
 * All callers except the pinned CPU MMIO single-word path retain native calls.
 * That caller discards the writer result. Native capacity/backpressure, parser,
 * interrupt state and guest cycle accounting are untouched. */
    .set noreorder
    .set noat
#if NP_GP0_PACKET_COALESCE < 0 || NP_GP0_PACKET_COALESCE > 1 || NP_GP0_SCALAR_WRITE < 0 || NP_GP0_SCALAR_WRITE > 1
#error "GP0 adapters accept boolean build flags only"
#endif
#if NP_GP0_PACKET_COALESCE || NP_GP0_PACKET_TRACE || NP_GP0_SCALAR_WRITE
    .section ._psxcore_gp0,"ax",@progbits
    .balign 16
    .globl np_gp0_publish,np_gp0_begin,np_gp0_end,np_gp0_records
    .globl np_gp0_read_barrier,np_gp0_status_barrier
    .globl np_gp0_control_barrier,np_gp0_video_barrier
np_gp0_begin:
np_gp0_publish:
    /* Enter via J at 0x21a974, after its original semaphore-load delay slot.
     * The producer's native 48-byte frame remains beneath this scratch frame. */
    addiu $sp,$sp,-48
    sq $8,0($sp)
    sq $9,16($sp)
    sq $10,32($sp)
    la $8,np_gp0_records
#if NP_GP0_PACKET_TRACE
    lw $9,4($8)
    addiu $9,$9,1
    sw $9,4($8)
    mfc0 $9,$9
    sw $9,16($8)
#else
    /* Keep resident entry addresses stable when omitting diagnostic work. */
    .space 20
#endif
#if NP_GP0_PACKET_COALESCE
    lw $9,20($8)
    beqz $9,np_gp0_native
    nop
    ld $9,88($sp)
    li $10,0x20b510
    bne $9,$10,np_gp0_native
    nop
    li $9,1
    bne $s2,$9,np_gp0_native
    nop
    lw $9,0x2270($s0)
    li $10,0xffffff
    bne $9,$10,np_gp0_native
    nop
    lw $9,0x227c($s0)
    bnez $9,np_gp0_native
    nop
    lw $9,0x2264($s0)
    bnez $9,np_gp0_native
    nop
    lw $9,0x22bc($s0)
    li $10,-1
    bne $9,$10,np_gp0_native
    nop
    lw $9,0x228c($s0)
    bnez $9,np_gp0_native
    nop
    lw $9,0x2140($s0)
    addiu $10,$s0,0x20c0
    bne $9,$10,np_gp0_native
    nop
    lw $9,0($9)
    srl $9,$9,24
    /* A0 has a fixed three-word header in the pinned native table. Once its
     * third word is present, publish immediately: the native worker owns all
     * pixel acquisition, transfer completion and buffer reuse. This also
     * excludes data words while the worker's active-command field is set. */
    addiu $10,$9,-0xa0
    beqz $10,np_gp0_fixed
    nop
    /* Fixed polygons 20..3f, lines 40..47/50..57, rectangles 60..7f.
     * Exclude variable polylines despite their table's fixed initial length. */
    addiu $10,$9,-0x20
    sltiu $10,$10,0x28
    bnez $10,np_gp0_fixed
    nop
    addiu $10,$9,-0x50
    sltiu $10,$10,8
    bnez $10,np_gp0_fixed
    nop
    addiu $10,$9,-0x60
    sltiu $10,$10,0x20
    beqz $10,np_gp0_native
    nop
np_gp0_fixed:
    sll $9,$9,3
    li $10,0x4ff5c0
    addu $9,$9,$10
    lw $9,0($9)
    addiu $10,$9,-2
    sltiu $10,$10,11
    beqz $10,np_gp0_native
    nop
    lw $10,0x2288($s0)
    beqz $10,np_gp0_native
    nop
    sltu $9,$10,$9
    beqz $9,np_gp0_native
    nop
np_gp0_defer:
    /* Fewer than 12 words of an incomplete command. A complete command is
     * never withheld. Capacity and explicit barriers publish through native
     * calls. No busy-wait, command buffering or unbounded delay is introduced. */
    li $9,1
    sw $9,0($8)
#if NP_GP0_PACKET_TRACE
    lw $9,8($8)
    addiu $9,$9,1
    sw $9,8($8)
#else
    .space 12
#endif
    lq $8,0($sp)
    lq $9,16($sp)
    lq $10,32($sp)
    addiu $sp,$sp,48
    move $v0,$a0
    ld $s1,8($sp)
    ld $s0,0($sp)
    ld $s2,16($sp)
    ld $s3,24($sp)
    ld $s4,32($sp)
    ld $ra,40($sp)
    jr $ra
    addiu $sp,$sp,48

#endif
np_gp0_native:
    sw $zero,0($8)
    lq $8,0($sp)
    lq $9,16($sp)
    lq $10,32($sp)
    addiu $sp,$sp,48
    /* Replay PollSema, then the intact native epilogue and SignalSema. */
    li $ra,0x21a97c
    j 0x22e870
    nop

#if NP_GP0_SCALAR_WRITE
    .globl np_gp0_scalar_write
np_gp0_scalar_write:
    /* Pinned CPU GP0 caller: a0 points to its copied stack word, a1 is one.
     * Match native fast admission. Copy a single word directly, then rejoin
     * the original interrupt-protected available-count publication. Preserve
     * the original producer frame, epilogue and fallback for a full/foreign
     * buffer or active DMA. No queue index/extent or cycle policy is changed. */
    addiu $sp,$sp,-48
    sd $s0,0($sp)
    sd $s1,8($sp)
    sd $s2,16($sp)
    sd $s3,24($sp)
    sd $s4,32($sp)
    sd $ra,40($sp)
    li $s0,0x700010f0
    la $v0,np_gp0_records
    lw $v0,24($v0)
    beqz $v0,np_gp0_scalar_fallback
    nop
    lw $v0,0x2270($s0)
    li $v1,0xffffff
    bne $v0,$v1,np_gp0_scalar_fallback
    nop
    lw $v0,0x2288($s0)
    beqz $v0,np_gp0_scalar_empty
    nop
    lw $v0,0x2140($s0)
    addiu $v1,$s0,0x20c0
    bne $v0,$v1,np_gp0_scalar_fallback
    nop
    lw $v0,0x2290($s0)
    sltiu $v1,$v0,31
    beqz $v1,np_gp0_scalar_fallback
    nop
    b np_gp0_scalar_copy
    nop
np_gp0_scalar_empty:
    sw $zero,0x228c($s0)
    sw $zero,0x2290($s0)
    addiu $v1,$s0,0x20c0
    sw $v1,0x2140($s0)
    move $v0,$zero
np_gp0_scalar_copy:
    sll $v1,$v0,2
    addu $v1,$v1,$s0
    lw $s1,0($a0)
    sw $s1,0x20c0($v1)
    addiu $v0,$v0,1
    sw $v0,0x2290($s0)
    li $s2,1
    lui $s3,0x7000
    move $s4,$a0
    j 0x21a92c
    nop
np_gp0_scalar_fallback:
    ld $s0,0($sp)
    ld $s1,8($sp)
    ld $s2,16($sp)
    ld $s3,24($sp)
    ld $s4,32($sp)
    ld $ra,40($sp)
    addiu $sp,$sp,48
    j 0x21a808
    nop
#endif

np_gp0_read_barrier:
    addiu $sp,$sp,-16
    sq $ra,0($sp)
    jal np_gp0_flush
    nop
    lq $ra,0($sp)
    addiu $sp,$sp,16
    j 0x21a698
    nop
np_gp0_status_barrier:
    addiu $sp,$sp,-16
    sq $ra,0($sp)
    jal np_gp0_flush
    nop
    lq $ra,0($sp)
    addiu $sp,$sp,16
    j 0x21ac30
    nop
np_gp0_control_barrier:
    addiu $sp,$sp,-16
    sq $ra,0($sp)
    jal np_gp0_flush
    nop
    lq $ra,0($sp)
    addiu $sp,$sp,16
    /* Replay the checked entry pair with its original native stack width. */
    addiu $sp,$sp,-32
    sd $s1,8($sp)
    j 0x21a570
    nop
np_gp0_video_barrier:
    addiu $sp,$sp,-16
    sq $ra,0($sp)
    jal np_gp0_flush
    nop
    lq $ra,0($sp)
    addiu $sp,$sp,16
    addiu $sp,$sp,-16
    lui $v0,0x7000
    j 0x21aeb8
    nop
np_gp0_flush:
    /* Inspect the deferred flag using only two saved wide temporaries. Keep
     * the original frame layout; an empty flush needs no syscall context. */
    addiu $sp,$sp,-96
    sq $8,64($sp)
    sq $9,80($sp)
    la $8,np_gp0_records
    lw $9,0($8)
    beqz $9,np_gp0_flush_done
    nop
    /* Native syscalls may change v0/v1. Save them, their argument and RA only
     * for a pending flush, before any clear, trace update or semaphore call. */
    sq $ra,0($sp)
    sq $v0,16($sp)
    sq $v1,32($sp)
    sq $a0,48($sp)
    sw $zero,0($8)
#if NP_GP0_PACKET_TRACE
    lw $9,12($8)
    addiu $9,$9,1
    sw $9,12($8)
#else
    .space 12
#endif
    lui $8,0x7000
    jal 0x22e870
    lw $a0,0x3c04($8)
    lui $8,0x7000
    jal 0x22e840
    lw $a0,0x3c04($8)
    lq $ra,0($sp)
    lq $v0,16($sp)
    lq $v1,32($sp)
    lq $a0,48($sp)
np_gp0_flush_done:
    lq $8,64($sp)
    lq $9,80($sp)
    jr $ra
    addiu $sp,$sp,96
    .balign 16
np_gp0_records:
    /* Sole CPU-producer writer: deferred flag, calls, deferred wakes,
     * barrier flushes, last EE Count (read-only, wraps; never adjusts timing),
     * experiment enable, scalar-write and reserved. */
    .space 20
    .word NP_GP0_PACKET_COALESCE
    .word NP_GP0_SCALAR_WRITE
    .word 0
np_gp0_end:
#endif
