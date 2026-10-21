/* Apply GP1(04) only while the native worker is parked at the checked
 * acquisition wait. An interrupt-masked lease, empty native queues, matching
 * parked status and interrupt-safe semaphore queries establish ownership.
 * Mirror the completed native control transaction without changing commands,
 * clock charges or completion policy. All other cases retain the native path.
 * The lease spans no live GPUSTAT read-modify-write across the worker wait. */
#ifndef NP_GP1_EARLY_REJECT
#define NP_GP1_EARLY_REJECT 1
#endif
#if NP_GP1_EARLY_REJECT < 0 || NP_GP1_EARLY_REJECT > 1
#error "GP1 early rejection accepts a boolean build flag only"
#endif
#if NP_GP1_IDLE_CONTROL && !NP_MEMORY_ACCESS_TRACE
.set noreorder
.set noat
.section ._psxcore_gp1_idle,"ax",@progbits
.balign 16
.globl np_gp1_idle_begin,np_gp1_idle_control,np_gp1_idle_park,np_gp1_idle_lease,np_gp1_idle_end
np_gp1_idle_begin:
np_gp1_idle_control:
 addiu $sp,$sp,-352
#if NP_GP1_EARLY_REJECT
 /* Preserve one temporary before filtering. Its original frame slot also
  * serves the unchanged ownership transaction when both predicates pass. */
 sq $8,112($sp)
 srl $8,$a0,24
 xori $8,$8,4
 bnez $8,.Lgp1_early_native
 nop
 li $8,0x20b520
 bne $ra,$8,.Lgp1_early_native
 nop
#endif
 sq $ra,0($sp)
 .irp reg,2,3,4,5,6,7,8,9,10,11,12,13,14,15,24,25
#if NP_GP1_EARLY_REJECT
 .if \reg != 8
#endif
 .if \reg < 16
 sq $\reg,((\reg-1)*16)($sp)
 .else
 sq $\reg,((\reg-9)*16)($sp)
 .endif
#if NP_GP1_EARLY_REJECT
 .endif
#endif
 .endr
#if !NP_GP1_EARLY_REJECT
 srl $8,$a0,24
 li $9,4
 bne $8,$9,.Lgp1_native
 nop
 li $8,0x20b520
 bne $ra,$8,.Lgp1_native
 nop
#endif
 mfc0 $v0,$12
 sw $v0,336($sp)
.Lgp1_di:
 di
 sync.p
 mfc0 $v0,$12
 lui $v1,1
 and $v0,$v0,$v1
 bnez $v0,.Lgp1_di
 nop
 la $8,np_gp1_idle_lease
 lw $9,0($8)
 li $8,1
 bne $9,$8,.Lgp1_fail_lease
 nop
 lui $10,0x7000
 lw $8,0x336c($10)
 lw $9,0x3378($10)
 or $8,$8,$9
 lw $9,0x3354($10)
 or $8,$8,$9
 lw $9,0x339c($10)
 or $8,$8,$9
 bnez $8,.Lgp1_fail_queues
 nop
 lw $8,0x3360($10)
 li $9,0xffffff
 bne $8,$9,.Lgp1_fail_queues
 nop
 lw $8,0x33ac($10)
 li $9,-1
 beq $8,$9,.Lgp1_command_ok
 lui $v1,0x1400
 li $9,0xa0
 bne $8,$9,.Lgp1_fail_active
 nop
 lui $v1,0x1000
.Lgp1_command_ok:
 lw $8,0x332c($10)
 lui $9,0x1400
 and $8,$8,$9
 bne $8,$v1,.Lgp1_fail_status
 nop
 lw $8,0x3370($10)
 lw $9,0x3374($10)
 bne $8,$9,.Lgp1_fail_queues
 nop
 sltiu $8,$8,32
 beqz $8,.Lgp1_fail_queues
 nop
 /* Both queries are the public interrupt-safe API, syscall -72. */
 lw $a0,0x3c04($10)
 addiu $a1,$sp,272
 li $v1,-72
 syscall
 bltz $v0,.Lgp1_fail_worker
 nop
 lw $8,272($sp)
 bnez $8,.Lgp1_fail_worker
 nop
 lw $8,276($sp)
 li $9,1
 bne $8,$9,.Lgp1_fail_worker
 nop
 lw $8,284($sp)
 bne $8,$9,.Lgp1_fail_worker
 nop
 lui $10,0x7000
 lw $a0,0x3c08($10)
 addiu $a1,$sp,304
 li $v1,-72
 syscall
 bltz $v0,.Lgp1_fail_ack
 nop
 lw $8,304($sp)
 li $9,1
 bne $8,$9,.Lgp1_fail_ack
 nop
 lw $8,308($sp)
 bne $8,$9,.Lgp1_fail_ack
 nop
 lw $8,316($sp)
 bnez $8,.Lgp1_fail_ack
 nop
 mfc0 $8,$12
 lui $9,1
 and $8,$8,$9
 bnez $8,.Lgp1_fail_status
 nop
 la $8,np_gp1_idle_lease
 lw $9,0($8)
 li $8,1
 bne $9,$8,.Lgp1_fail_lease
 nop
 /* Completed control-ring transaction, matching native producer+worker. */
 lui $10,0x7000
 lw $8,48($sp)
 lw $9,0x3374($10)
 sll $11,$9,2
 addu $11,$11,$10
 sw $8,0x3130($11)
 addiu $9,$9,1
 andi $9,$9,31
 sw $9,0x3374($10)
 sw $9,0x3370($10)
 li $9,4
 sw $9,0x33b4($10)
 /* GP1(04)'s entire native handler: direction and DMA-request status. */
 lw $v0,0x332c($10)
 li $v1,0x9fffffff
 and $v0,$v0,$v1
 andi $9,$8,3
 sll $11,$9,29
 or $v0,$v0,$11
 beqz $9,.Lgp1_direction_zero
 nop
 lui $v1,0x200
 b .Lgp1_status_done
 or $v0,$v0,$v1
.Lgp1_direction_zero:
 li $v1,0xfdffffff
 and $v0,$v0,$v1
.Lgp1_status_done:
 sw $v0,0x332c($10)
 ei
 b .Lgp1_fast_return
 nop
.Lgp1_fail_lease:
.Lgp1_fail_queues:
.Lgp1_fail_worker:
.Lgp1_fail_ack:
.Lgp1_fail_active:
.Lgp1_fail_status:
.Lgp1_failure:
 di
 sync.p
 mfc0 $v0,$12
 lui $v1,1
 and $v0,$v0,$v1
 bnez $v0,.Lgp1_failure
 nop
 lw $v0,336($sp)
 lui $v1,1
 and $v0,$v0,$v1
 beqz $v0,.Lgp1_native
 nop
 ei
.Lgp1_native:
 .irp reg,2,3,4,5,6,7,8,9,10,11,12,13,14,15,24,25
#if NP_GP1_EARLY_REJECT
 .if \reg != 8
#endif
 .if \reg < 16
 lq $\reg,((\reg-1)*16)($sp)
 .else
 lq $\reg,((\reg-9)*16)($sp)
 .endif
#if NP_GP1_EARLY_REJECT
 .endif
#endif
 .endr
 lq $ra,0($sp)
#if NP_GP1_EARLY_REJECT
.Lgp1_early_native:
 lq $8,112($sp)
#endif
 addiu $sp,$sp,352
 move $s1,$a0
 srl $v1,$s1,24
 j 0x21a578
 nop
.Lgp1_fast_return:
 .irp reg,2,3,4,5,6,7,8,9,10,11,12,13,14,15,24,25
 .if \reg < 16
 lq $\reg,((\reg-1)*16)($sp)
 .else
 lq $\reg,((\reg-9)*16)($sp)
 .endif
 .endr
 lq $ra,0($sp)
 addiu $sp,$sp,352
 ld $s1,8($sp)
 move $v0,$zero
 jr $ra
 addiu $sp,$sp,32
 .balign 16
np_gp1_idle_park:
 addiu $sp,$sp,-32
 sq $ra,0($sp)
 sq $8,16($sp)
 la $8,np_gp1_idle_lease
 li $ra,1
 sw $ra,0($8)
 jal 0x22e860
 nop
 la $8,np_gp1_idle_lease
 sw $zero,0($8)
 lq $8,16($sp)
 lq $ra,0($sp)
 jr $ra
 addiu $sp,$sp,32


 .balign 4
np_gp1_idle_lease:
 .word 0
np_gp1_idle_end:
#endif
