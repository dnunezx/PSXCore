/* Default-off diagnostic: defer native confirmed quit before pad/audio/kernel
 * teardown. No new stack, kernel wait, save wake, input decoder or ELF return.
 * The original handler has already clobbered caller scratch registers. The
 * continuation reloads its loop state; only t0..t2 are used here. */
#if NP_IGR_QUIT_PROBE
    .section ._psxcore_igr_quit,"ax",@progbits
    .set noreorder
    .set noat
    .balign 16
    .globl np_igr_quit_begin,np_igr_quit_probe
np_igr_quit_begin:
np_igr_quit_probe:
    beqz $v0,1f
    nop
    la $t0,np_igr_quit_observation
    lui $t1,0x2000
    or $t0,$t0,$t1
    lw $t1,8($t0)
    addiu $t1,$t1,1
    sw $t1,8($t0)
    sync
    lw $t2,12($t0)
    addiu $t2,$t2,1
    sw $t2,12($t0)
    sw $sp,16($t0)
    li $t2,0x507920
    lw $t2,0($t2)
    sw $t2,20($t0)
    li $t2,0x527930
    lw $t2,0($t2)
    sw $t2,24($t0)
    lw $t2,-30612($gp)
    sw $t2,28($t0)
    sync
    addiu $t1,$t1,1
    sw $t1,8($t0)
1:
    /* Original branch delay-slot lw v1,0(sp) remains at the patched site.
     * Continue exactly where the original no-quit branch would have gone. */
    j 0x20c0d4
    nop
    .balign 16
    .globl np_igr_quit_observation
np_igr_quit_observation:
    .word 0x5147504e,1
    .space 24
    .globl np_igr_quit_end
np_igr_quit_end:
#endif
