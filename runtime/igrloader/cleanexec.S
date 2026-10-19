/* Adapted from LUNA/Neutrino ee_core/src/asm.S, CleanExecPS2/InitRegsExecPS2.
 * Copyright 2010, jimmikaelkael <jimmikaelkael@wanadoo.fr>
 * LUNA 2026. Licensed under Academic Free License version 3.0.
 * Changes: independent resident stack and a retained ExecPS2 failure path.
 */
    .text
    .set noreorder
    .set noat
    .globl np_igr_clean_exec
    .ent np_igr_clean_exec
np_igr_clean_exec:
    /* a0..a3 carry entry, GP, argc and argv into ExecPS2. */
    por $at,$zero,$zero
    por $v0,$zero,$zero
    por $v1,$zero,$zero
    por $8,$zero,$zero
    por $9,$zero,$zero
    por $10,$zero,$zero
    por $11,$zero,$zero
    por $12,$zero,$zero
    por $13,$zero,$zero
    por $14,$zero,$zero
    por $15,$zero,$zero
    por $16,$zero,$zero
    por $17,$zero,$zero
    por $18,$zero,$zero
    por $19,$zero,$zero
    por $20,$zero,$zero
    por $21,$zero,$zero
    por $22,$zero,$zero
    por $23,$zero,$zero
    por $24,$zero,$zero
    por $25,$zero,$zero
    por $gp,$zero,$zero
    lui $sp,0x000e
    ori $sp,$sp,0x8000
    /* The private stack is 0xe0000..0xe8000, below the disc-swap arenas. */
    por $fp,$zero,$zero
    mthi $zero
    mthi1 $zero
    mtlo $zero
    mtlo1 $zero
    mtsah $zero,0
    .irp reg,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    mtc1 $zero,$f\reg
    .endr
    adda.s $f0,$f1
    ctc1 $zero,$31
    sync.p
    /* This is a terminal handoff, so no caller registers need restoring. */
    lui $ra,%hi(np_igr_exec_failed)
    addiu $ra,$ra,%lo(np_igr_exec_failed)
    j ExecPS2
    nop
    .end np_igr_clean_exec
