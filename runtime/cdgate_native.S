/* Original transparent entry/return shims. No production hook is installed. */
#if NP_CD_GATE
    .section ._psxcore_cd_gate.entry,"ax",@progbits
    .set noreorder
    .set noat
    .balign 16
    .globl np_cd_gate_begin
np_cd_gate_begin:
    /* Preserve full GPRs and both packed HI/LO banks across C service. The
     * native scheduler's original return delay restores its frame first. */
    .macro SAVE
    addiu $sp,$sp,-480
    .irp reg,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25
    sq $\reg,16*(\reg-1)($sp)
    .endr
    sq $gp,400($sp)
    sq $fp,416($sp)
    sq $ra,432($sp)
    pmfhi $8
    sq $8,448($sp)
    pmflo $8
    sq $8,464($sp)
    .endm
    .macro RESTORE native=0,mask=0
    lq $8,448($sp)
    pmthi $8
    lq $8,464($sp)
    pmtlo $8
    .irp reg,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25
    lq $\reg,16*(\reg-1)($sp)
    .endr
    lq $gp,400($sp)
    lq $fp,416($sp)
    lq $ra,432($sp)
    .if \native
    .if \mask
    addiu $a1,$a1,-64
    .endif
    j np_cd_gate_native_write
    .else
    jr $ra
    .endif
    addiu $sp,$sp,480
    .endm
    .globl np_cd_gate_write_entry
np_cd_gate_write_entry:
    SAVE
    jal np_cd_gate_write
    nop
    beqz $v0,9f
    nop
    li $at,2
    beq $v0,$at,8f
    nop
    RESTORE
8:
    RESTORE 1,1
9:
    RESTORE 1,0
    .globl np_cd_gate_tail
np_cd_gate_tail:
    SAVE
    jal np_cd_gate_service
    nop
    RESTORE
    /* Replay only the replaced native entry prologue; the original writer
     * retains bank decoding, cycle charging, effects and IRQ/response paths. */
    .globl np_cd_gate_native_write
np_cd_gate_native_write:
    addiu $sp,$sp,-16
    lw $a2,-32608($gp)
    j 0x20f9c8
    nop
    .balign 16
    .section ._psxcore_cd_gate.code_end,"a",@progbits
    .globl np_cd_gate_code_end
np_cd_gate_code_end:
    .section ._psxcore_cd_gate.end,"a",@progbits
    .globl np_cd_gate_end
np_cd_gate_end:
#endif
