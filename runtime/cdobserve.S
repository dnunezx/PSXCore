/* Opt-in POPS telemetry. No waits, serial output, media writes or new RPCs.
 * Each checked call retains its original delay slot. The RPC's ninth stack
 * argument and all eight wide register arguments reach the native callee.
 * Records describe call boundaries, never an exclusive IO barrier. */
#if NP_CD_OBSERVE
    .section ._psxcore_cd_observe,"ax",@progbits
    .set noreorder
    .set noat
    .balign 16
    .globl np_cd_observe_begin
np_cd_observe_begin:
    .macro CD_STATE row
    la $t0,np_cd_observation+16+40*\row
    lui $t1,0x2000
    or $t0,$t0,$t1
    .endm
    .macro CD_BEGIN
    lw $t1,0($t0)
    addiu $t1,$t1,1
    sw $t1,0($t0)
    sync
    .endm
    .macro CD_END
    sync
    lw $t1,0($t0)
    addiu $t1,$t1,1
    sw $t1,0($t0)
    .endm
    .macro CD_COUNT offset
    lw $t1,\offset($t0)
    li $t2,65535
    sltu $t2,$t1,$t2
    beqz $t2,1f
    nop
    addiu $t1,$t1,1
    b 2f
    sw $t1,\offset($t0)
1:
    li $t1,1
    sw $t1,32($t0)
2:
    .endm
    .macro CD_CALL name,row,target,rpc=0,dynamic=0
    .globl \name
\name:
    addiu $sp,$sp,-80
    sq $ra,16($sp)
    sq $t0,32($sp)
    sq $t1,48($sp)
    sq $t2,64($sp)
    .if \rpc
    lw $t0,80($sp)
    sw $t0,0($sp)
    .endif
    CD_STATE \row
    CD_BEGIN
    CD_COUNT 4
    sw $a0,16($t0)
    sw $a1,20($t0)
    sw $a2,24($t0)
    sw $a3,28($t0)
    CD_END
    lq $t0,32($sp)
    lq $t1,48($sp)
    lq $t2,64($sp)
    .if \dynamic
    jalr $a1
    .else
    jal \target
    .endif
    nop
    CD_STATE \row
    CD_BEGIN
    CD_COUNT 8
    sw $v0,12($t0)
    CD_END
    lq $ra,16($sp)
    jr $ra
    addiu $sp,$sp,80
    .endm
    CD_CALL np_cd_read_rpc,0,0x230330,1
    CD_CALL np_cd_prefetch_rpc,1,0x230330,1
    CD_CALL np_cd_prefetch_callback,2,0x22e850
    /* The native object fields have already been cleared and the original
     * epilogue delay has restored sp. Preserve every register we touch. */
    .macro CD_RETIRE name,row
    .globl \name
\name:
    addiu $sp,$sp,-48
    sq $t0,0($sp)
    sq $t1,16($sp)
    sq $t2,32($sp)
    CD_STATE \row
    CD_BEGIN
    CD_COUNT 4
    CD_COUNT 8
    sw $v0,12($t0)
    sw $a0,16($t0)
    sw $a1,20($t0)
    sw $a2,24($t0)
    sw $a3,28($t0)
    CD_END
    lq $t0,0($sp)
    lq $t1,16($sp)
    lq $t2,32($sp)
    jr $ra
    addiu $sp,$sp,48
    .endm
    CD_RETIRE np_cd_response_retire,3
    CD_RETIRE np_cd_getstat_retire,4
    CD_CALL np_cd_deferred_callback,5,0,0,1
    CD_CALL np_cd_ack_irq,6,0x20e638
    CD_CALL np_cd_worker_read,7,0x210090
    .balign 16
    .globl np_cd_observation
np_cd_observation:
    .word 0x4f434450,2,8,40
    .space 320
    .globl np_cd_observe_end
np_cd_observe_end:
#endif
