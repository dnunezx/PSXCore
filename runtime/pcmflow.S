/* Output-only buffer feedback for the pinned 44.1-to-48 kHz resampler.
 * The guest SPU, XA decoder, timers and DMA keep their native clocks. Normal
 * queue levels retain the exact native step. Below 1024 frames, reduce the
 * output resampler's input advance proportionally, bounded to ten percent,
 * to absorb transient guest production delays without mute/recovery cycles.
 * Unknown resampler steps bypass the adapter. No separate mutable state. */
    .set noreorder
    .set noat
    .balign 16
    .globl np_pcm_buffer_rate,np_pcm_buffer_rate_end
np_pcm_buffer_rate:
    lw $s2,8($a1)
    lui $8,0x50
    ori $8,$8,0x2f28
    lw $9,0($8)
    addiu $10,$9,-3387
    sltiu $10,$10,377
    beqz $10,np_pcm_buffer_rate_finish
    li $10,3763
    sltiu $11,$s2,1024
    beqz $11,np_pcm_buffer_rate_store
    li $9,1024
    subu $9,$9,$s2
    nop
    sltiu $11,$9,377
    bnez $11,np_pcm_buffer_rate_adjust
    nop
    li $9,376
np_pcm_buffer_rate_adjust:
    subu $10,$10,$9
np_pcm_buffer_rate_store:
    sw $10,0($8)
np_pcm_buffer_rate_finish:
    sltiu $v0,$s2,252
    j 0x213f44
    nop
np_pcm_buffer_rate_end:
