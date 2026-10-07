/* Automatic POPS adapter for direct PS24 scanout on both supported platforms.
 * Select the native buffered RGB24 converter; it owns readback, transfer,
 * drawing and display publication. The adapter writes no guest display state,
 * pixels, packets, semaphores or cycles, and preserves every other wide GPR.
 * Upload/copy/fill updates select the existing converter for RGB24 and retain
 * the native incremental blitter otherwise. No new pixel processing is used. */
    .set noreorder
    .set noat
#if NP_RGB24_SCANOUT_ADAPTER
    .section ._psxcore_rgb24,"ax",@progbits
    .balign 16
    .globl np_rgb24_buffered
np_rgb24_buffered:
    /* Native buffer option is tested after dispatch. Select it only for PS24;
     * leave the actual options word intact for the transition back. */
    addiu $sp,$sp,-16
    sq $v1,0($sp)
    lw $v1,0x21dc($s0)
    lw $v0,0x22a0($s0)
    xori $v1,$v1,0x12
    bnez $v1,rgb_buffered_exit
    nop
    ori $v0,$v0,2
rgb_buffered_exit:
    lq $v1,0($sp)
    addiu $sp,$sp,16
    j 0x222288
    andi $v0,$v0,2

    .globl np_rgb24_dispatch
np_rgb24_dispatch:
    /* Direct scanout bypasses the converter before its option test. Select
     * native conversion for PS24 frames and for a pending return from our
     * buffered output. Retain the native arguments and dirty flags. */
    addiu $sp,$sp,-16
    sq $a0,0($sp)
    lui $v0,1
    and $v0,$v1,$v0
    lw $a0,0x21dc($s0)
    xori $a0,$a0,0x12
    beqz $a0,rgb_dispatch_select
    nop
    lw $a0,0x264c($s0)
    beqz $a0,rgb_dispatch_exit
    nop
    lw $a0,0x22a0($s0)
    andi $a0,$a0,2
    bnez $a0,rgb_dispatch_exit
    nop
    andi $a0,$v1,0x100
    beqz $a0,rgb_dispatch_exit
    nop
rgb_dispatch_select:
    li $v0,1
rgb_dispatch_exit:
    lq $a0,0($sp)
    addiu $sp,$sp,16
    j 0x221cec
    nop

    .globl np_rgb24_update
np_rgb24_update:
    /* GP0 fill/upload/copy callers pass a rectangle in 16-bit VRAM words to
     * the ordinary buffered blitter. RGB24 packs three bytes per pixel, so
     * refresh the requested display through the native RGB24 converter.
     * Tail delegation retains the caller's link, stack and callee-saved GPRs.
     * Native conversion owns flushing/readback and the current output bank. */
    lui $v0,0x7000
    lw $v0,0x32cc($v0)
    xori $v0,$v0,0x12
    bnez $v0,rgb_update_ordinary
    nop
    lui $v0,0x7000
    lw $a0,0x32b4($v0)
    lw $a1,0x32b8($v0)
    lw $a2,0x32bc($v0)
    lw $a3,0x32c0($v0)
    lw $8,0x377c($v0)
    j 0x222e98
    nop
rgb_update_ordinary:
    j 0x222658
    nop
    .globl np_rgb24_end
np_rgb24_end:
#endif
