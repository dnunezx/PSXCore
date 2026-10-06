/* Original opt-in PCM gap-edge concealment, included inside the unused
 * 4096-byte diagnostic ring. All prior resident addresses remain fixed.
 * Steady audio bypasses unchanged. Native underflow/mute/recovery decisions,
 * ring ownership, resampling, sample rate, voice state and clocks stay intact.
 * A partial block tapers its last <=64 real frames to zero. The first complete
 * block after a gap ramps its first 64 frames from zero to full amplitude.
 * This softens discontinuities; it cannot remedy sustained underproduction. */
#if NP_AUDIO_GAP_DECLICK < 0 || NP_AUDIO_GAP_DECLICK > 1
#error "Audio gap adapter accepts a boolean build flag only"
#endif
    .balign 16
    .globl np_audio_begin,np_audio_gap_declick,np_audio_end,np_audio_records
np_audio_begin:
np_audio_gap_declick:
    addiu $sp,$sp,-192
    sq $8,0($sp)
    sq $9,16($sp)
    sq $10,32($sp)
    sq $11,48($sp)
    sq $12,64($sp)
    sq $13,80($sp)
    sq $14,96($sp)
    sq $15,112($sp)
    sq $v0,128($sp)
    sq $v1,144($sp)
    pmfhi $v0
    sq $v0,160($sp)
    pmflo $v0
    sq $v0,176($sp)
    la $8,np_audio_records
    lw $9,0($8)
    beqz $9,np_audio_finish
    nop
    li $9,0x850000
    bne $s5,$9,np_audio_finish
    nop
    sltiu $9,$s2,252
    beqz $9,np_audio_finish
    nop
    li $9,0x5028bc
    lw $9,0($9)
    beqz $9,np_audio_not_muted
    nop
    li $10,1
    bne $9,$10,np_audio_finish
    nop
    /* The native path already filled this block with zeros. */
    sw $10,4($8)
    lw $9,20($8)
    addiu $9,$9,1
    b np_audio_finish
    sw $9,20($8)
np_audio_not_muted:
    li $9,251
    beq $s2,$9,np_audio_complete
    nop
    li $9,1
    sw $9,4($8)
    lw $9,16($8)
    addiu $9,$9,1
    sw $9,16($8)
    beqz $s2,np_audio_finish
    nop
    li $9,64
    sltu $10,$s2,$9
    beqz $10,1f
    nop
    move $9,$s2
1:
    move $15,$9
    addiu $12,$9,-1
    li $11,-1
    subu $10,$s2,$9
    sll $10,$10,2
    addu $10,$10,$s5
    addiu $10,$10,0x580
    lw $13,8($8)
    addiu $13,$13,1
    b np_audio_ramp
    sw $13,8($8)
np_audio_complete:
    lw $9,24($8)
    addiu $9,$9,1
    sw $9,24($8)
    lw $9,4($8)
    beqz $9,np_audio_finish
    nop
    sw $zero,4($8)
    lw $9,12($8)
    addiu $9,$9,1
    sw $9,12($8)
    li $9,64
    li $15,63
    move $12,$zero
    li $11,1
    addiu $10,$s5,0x580
np_audio_ramp:
    /* Signed 16-bit samples times a bounded gain fit signed 32-bit products.
     * Signed division rounds toward zero, never clipping or wrapping samples. */
    lh $13,0($10)
    mult $14,$13,$12
    div $zero,$14,$15
    mflo $14
    sh $14,0($10)
    lh $13,2($10)
    mult $14,$13,$12
    div $zero,$14,$15
    mflo $14
    sh $14,2($10)
    addu $12,$12,$11
    addiu $9,$9,-1
    bnez $9,np_audio_ramp
    addiu $10,$10,4
np_audio_finish:
    lq $v0,160($sp)
    pmthi $v0
    lq $v0,176($sp)
    pmtlo $v0
    lq $v0,128($sp)
    lq $v1,144($sp)
    lq $8,0($sp)
    lq $9,16($sp)
    lq $10,32($sp)
    lq $11,48($sp)
    lq $12,64($sp)
    lq $13,80($sp)
    lq $14,96($sp)
    lq $15,112($sp)
    addiu $sp,$sp,192
    /* Tail-call the original resampler. Original RA and delay-slot arguments
     * are intact; only one JAL word changes at the callback's existing call. */
    j 0x2145b0
    nop
    .balign 16
np_audio_records:
    /* Single output-callback writer: enable, pending fade-in, fade-out/in
     * blocks, short blocks, native muted blocks, full blocks, reserved. */
    .word NP_AUDIO_GAP_DECLICK
    .space 28
np_audio_end:
