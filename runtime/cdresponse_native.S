/* Original reserved-node entry shim for the checked POPS scheduler.
 * No patch is installed. The caller has exclusively removed a native free
 * node before entry. Preserve the native frame/clock/ordering ABI and reuse
 * its insertion tail WITHOUT its allocator or free-head write.
 * a0=delay, a1=callback, a2=argument, a3=reserved event; gp=checked POPS gp. */
#if NP_CD_RESPONSE
    .section ._psxcore_cd_response.text,"ax",@progbits
    .set noreorder
    .set noat
    .balign 16
    .globl np_cd_response_schedule_reserved
np_cd_response_schedule_reserved:
    addiu $sp,$sp,-48
    sd $s0,0($sp)
    sd $s1,8($sp)
    sd $s2,16($sp)
    sd $s3,24($sp)
    sd $ra,32($sp)
    move $s0,$a3
    move $s1,$a0
    move $s2,$a2
    move $s3,$a1
    lw $v1,-30676($gp)
    subu $v0,$zero,$v1
    slt $v0,$s1,$v0
    addu $v1,$s1,$v1
    movz $s1,$v1,$v0
    jal 0x201008
    nop
    j 0x201104
    nop
#endif
