/* Automatic POPS adapter for direct PS24 scanout on both supported platforms.
 * Select the native buffered RGB24 converter; it owns readback, transfer,
 * drawing and display publication. Interlaced output marks the native display
 * descriptor dirty for publication. Pixels, packets, semaphores and cycles
 * stay native, and every other wide GPR is preserved.
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
#if NP_INTERLACE_BUFFERED_PRESENT
    bnez $v1,np_interlace_buffered
#else
    bnez $v1,rgb_buffered_exit
#endif
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
#if NP_INTERLACE_BUFFERED_PRESENT
    b np_interlace_dispatch
    nop
#else
    lw $a0,0x264c($s0)
    beqz $a0,rgb_dispatch_exit
#endif
    nop
rgb_dispatch_buffer_continue:
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
#if NP_INTERLACE_BUFFERED_PRESENT
    /* Presentation workaround for the demonstrated NTSC PS15/480
     * tuple. It renders both source fields into native buffered output rather
     * than exposing partially cleared VRAM. This is not cycle-accurate PS1
     * interlaced rendering; broader game and physical-console tests remain pending.
     * Status, control queues, clocks, options and native buffer ownership stay
     * intact. Nonmatching modes retain the existing selectors and masks. */
    .section ._psxcore_interlace_scanout,"ax",@progbits
    .balign 16
    .macro NP_INTERLACE_TUPLE reg,fallback
    lui \reg,0x7000
    lw \reg,0x32cc(\reg)
    xori \reg,\reg,2
    bnez \reg,\fallback
    nop
    lui \reg,0x7000
    lw \reg,0x3304(\reg)
    xori \reg,\reg,1
    bnez \reg,\fallback
    nop
    lui \reg,0x7000
    lw \reg,0x3308(\reg)
    bnez \reg,\fallback
    nop
    lui \reg,0x7000
    lw \reg,0x3300(\reg)
    xori \reg,\reg,2
    bnez \reg,\fallback
    nop
    lui \reg,0x7000
    lw \reg,0x32c0(\reg)
    xori \reg,\reg,480
    bnez \reg,\fallback
    nop
    .endm
    .globl np_interlace_buffered
np_interlace_buffered:
    addiu $sp,$sp,-16
    sq $a0,0($sp)
    NP_INTERLACE_TUPLE $a0,interlace_buffered_exit
    ori $v0,$v0,2
interlace_buffered_exit:
    lq $a0,0($sp)
    addiu $sp,$sp,16
    j rgb_buffered_exit
    nop
    .globl np_interlace_dispatch
np_interlace_dispatch:
    NP_INTERLACE_TUPLE $a0,interlace_dispatch_native
    /* The converter changes the buffered display descriptor on each field.
     * Publish that descriptor through the native dirty-state path after its
     * existing GS completion wait, before the next field's uploads begin. */
    ori $a0,$v1,0x20
    sw $a0,0x22ac($s0)
    j rgb_dispatch_select
    nop
interlace_dispatch_native:
    lw $a0,0x264c($s0)
    beqz $a0,rgb_dispatch_exit
    nop
    j rgb_dispatch_buffer_continue
    nop
    .macro NP_INTERLACE_MASK name,base,dest,back
    .globl \name
\name:
    lw $v0,0x2268(\base)
    ori \dest,$v0,2
    addiu $sp,$sp,-16
    sq $8,0($sp)
    NP_INTERLACE_TUPLE $8,9f
    /* Change only the GS drawing mask once native buffered output is active.
     * Other displaced outputs retain their native field value. */
    lui $8,0x7000
    lw $8,0x373c($8)
    xori $8,$8,1
    bnez $8,9f
    nop
    move \dest,$zero
9:
    lq $8,0($sp)
    addiu $sp,$sp,16
    j \back
    nop
    .endm
    NP_INTERLACE_MASK np_interlace_fill,$v1,$v0,0x21ce90
    NP_INTERLACE_MASK np_interlace_flat,$v1,$a0,0x21daf8
    NP_INTERLACE_MASK np_interlace_textured,$a2,$v1,0x21e3e0
    NP_INTERLACE_MASK np_interlace_rectangle,$v1,$v0,0x21fa24
    NP_INTERLACE_MASK np_interlace_line,$v1,$s4,0x22069c
    .globl np_interlace_end
np_interlace_end:
#endif
#endif
