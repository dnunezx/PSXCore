/* Exact polygon cost division for the pinned native C call sites. Only the
 * signed-32 numerator/divisor-100 case is specialized; other operands retain
 * the original signed-64 helper. No raster charges or drawing state change.
 * The callers consume v0 and treat v1/HI/LO as caller-clobbered. */
#if NP_RASTER_COST_FASTPATH != 1
#error "Raster cost fast path accepts a boolean build flag only"
#endif
    .balign 16
    .globl np_raster_div100,np_raster_cost_end
np_raster_div100:
    dsll32 $v0,$a0,0
    dsra32 $v0,$v0,0
    bne $v0,$a0,np_raster_div100_native
    li $v1,100
    bne $a1,$v1,np_raster_div100_native
    nop
    div $zero,$a0,$a1
    mflo $v0
    jr $ra
    nop
np_raster_div100_native:
    j 0x23c068
    nop
np_raster_cost_end:
