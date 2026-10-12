/* Cooperative-wait primitive for the pinned POPS runtime; dormant by default.
 * NP_IGR_PROBE optionally calls once after native event dispatch.
 * Delay uses native alarm units, not milliseconds. No pause hook calls this.
 * An uncertain wait/delete failure quarantines resources until cold restart.
 * Never reuse the save worker semaphore, cancel its alarm, or call bootstrap C. */
/* The native alarm adapter enters user mode on its own stack. Reuse its checked
 * callback; never redirect it into low resident code/data or switch its stack.
 * Keep the probe disabled outside isolated diagnostics until live validation. */
#if NP_IGR_WAIT != 0 && NP_IGR_WAIT != 1
#error "NP_IGR_WAIT must be 0 or 1"
#endif
#if NP_IGR_PROBE != 0 && NP_IGR_PROBE != 1
#error "NP_IGR_PROBE must be 0 or 1"
#endif
#if NP_IGR_PROBE && (!NP_IGR_WAIT || !NP_IGR_OBSERVE || NP_CD_OBSERVE)
#error "IGR probe requires WAIT/IGR observations and excludes CD observations"
#endif
#if NP_IGR_WAIT
    .section ._psxcore_igr_wait,"ax",@progbits
    .set noreorder
    .set noat
    .balign 16
    .globl np_igr_wait_once,np_igr_wait_begin
np_igr_wait_begin:
#if NP_IGR_PROBE
    /* Main writer and interrupt callback publish separate generations. */
    .macro PROBE_BEGIN
    la $t0,np_igr_probe_observation
    lui $t1,0x2000
    or $t0,$t0,$t1
    lw $t1,8($t0)
    addiu $t1,$t1,1
    sw $t1,8($t0)
    sync
    .endm
    .macro PROBE_ASYNC rpc,file,valid
    la $t2,np_igr_probe_async_observation
    lui $t1,0x2000
    or $t2,$t2,$t1
    lw $t3,8($t2)
    lw $a3,16($t2)
    sync
    lw $t1,12($t2)
    sw $t1,\rpc($t0)
    lw $t1,20($t2)
    sw $t1,\file($t0)
    sync
    sw $zero,\valid($t0)
    lw $t1,8($t2)
    bne $t1,$t3,91f
    lw $t1,16($t2)
    bne $t1,$a3,91f
    or $t1,$t3,$a3
    andi $t1,$t1,1
    bnez $t1,91f
    li $t1,1
    sw $t1,\valid($t0)
91:
    .endm
    .macro PROBE_END phase
    li $t1,\phase
    sw $t1,12($t0)
    sync
    lw $t1,8($t0)
    addiu $t1,$t1,1
    sw $t1,8($t0)
    .endm
    /* A torn worker snapshot is recorded once and rejected, never retried. */
    .macro PROBE_WORKER waits,wakes,generation,valid
    lw $t3,56($t2)
    sync
    lw $t1,60($t2)
    sw $t1,\waits($t0)
    lw $t1,64($t2)
    sw $t1,\wakes($t0)
    sync
    lw $a3,56($t2)
    sw $t3,\generation($t0)
    sw $zero,\valid($t0)
    bne $t3,$a3,90f
    andi $t1,$t3,1
    bnez $t1,90f
    li $t1,1
    sw $t1,\valid($t0)
90:
    .endm
#endif
np_igr_wait_once:
    addiu $sp,$sp,-80
    sq $s0,32($sp)
    sq $s1,48($sp)
    sq $ra,64($sp)
    sll $s1,$a0,0
    la $s0,np_igr_wait_quarantined
    lui $v0,0x2000
    or $s0,$s0,$v0
    lw $v0,0($s0)
    bnez $v0,8f
    addiu $v0,$s1,-1
    li $v1,65535
    sltu $v0,$v0,$v1
    beqz $v0,8f
    li $v0,1
    /* Fully initialized private semaphore: max=1, initial=0. */
    sw $zero,0($sp)
    sw $v0,4($sp)
    sw $zero,8($sp)
    sw $zero,12($sp)
    sw $zero,16($sp)
    sw $zero,20($sp)
    jal 0x22e820
    move $a0,$sp
    sll $v0,$v0,0
#if NP_IGR_PROBE
    PROBE_BEGIN
    sw $v0,24($t0)
    sw $v0,32($t0)
    PROBE_END 2
#endif
    bltz $v0,9f
    nop
    move $s0,$v0
    move $a0,$s1
    li $a1,0x238468
    jal 0x22e5a0
    move $a2,$s0
    sll $v0,$v0,0
#if NP_IGR_PROBE
    PROBE_BEGIN
    sw $v0,28($t0)
    sw $v0,36($t0)
    PROBE_END 3
#endif
    bltz $v0,5f
    move $s1,$v0
#if NP_IGR_PROBE
    PROBE_BEGIN
    la $t2,np_igr_observation
    lui $t1,0x2000
    or $t2,$t2,$t1
    PROBE_WORKER 64,68,112,116
    PROBE_ASYNC 144,148,160
    mfc0 $t1,$9
    sw $t1,96($t0)
    PROBE_END 4
#endif
    jal 0x22e860
    move $a0,$s0
    sll $v0,$v0,0
#if NP_IGR_PROBE
    PROBE_BEGIN
    sw $v0,40($t0)
    mfc0 $t1,$9
    sw $t1,100($t0)
    la $t2,np_igr_observation
    lui $t1,0x2000
    or $t2,$t2,$t1
    PROBE_WORKER 72,76,120,124
    PROBE_ASYNC 152,156,164
    PROBE_END 5
#endif
    bltz $v0,7f
    nop
    /* The private semaphore has one owner and the checked native callback is
     * its only signaler. A successful native wait consumes that signal. This
     * does not establish callback epilogue completion or full teardown safety. */
    jal 0x22e830
    move $a0,$s0
    sll $v0,$v0,0
#if NP_IGR_PROBE
    PROBE_BEGIN
    sw $v0,44($t0)
    PROBE_END 6
#endif
    bltz $v0,7f
    nop
    b 9f
    move $v0,$zero
5:
    /* No alarm was accepted: delete the unused semaphore, preserve error. */
    jal 0x22e830
    move $a0,$s0
    sll $v0,$v0,0
#if NP_IGR_PROBE
    PROBE_BEGIN
    sw $v0,44($t0)
    PROBE_END 6
#endif
    bltz $v0,7f
    nop
    b 9f
    move $v0,$s1
7:
    /* An accepted alarm may still reference the private semaphore. Retain it
     * rather than cancel/delete/reuse after an uncertain wait completion. */
    la $t0,np_igr_wait_quarantined
    lui $t1,0x2000
    or $t0,$t0,$t1
    li $t1,1
    sw $t1,0($t0)
    sync
    b 9f
    nop
8:
    li $v0,-1
9:
    lq $s0,32($sp)
    lq $s1,48($sp)
    lq $ra,64($sp)
    jr $ra
    addiu $sp,$sp,80
    .balign 16
    .globl np_igr_wait_quarantined
np_igr_wait_quarantined:
    .word 0
#if NP_IGR_PROBE
    .balign 16
    .globl np_igr_probe_once
np_igr_probe_once:
    la $t0,np_igr_probe_observation
    lui $t1,0x2000
    or $t0,$t0,$t1
    lw $t1,16($t0)
    bnez $t1,12f
    nop
    la $t2,np_igr_observation
    lui $t1,0x2000
    or $t2,$t2,$t1
    lw $t1,12($t2)
    sltiu $t1,$t1,120
    bnez $t1,12f
    nop
    /* One main-thread owner. The separate asserted reservation includes the
     * native kernel's 640-byte context below the deepest 112-byte guest frame.
     * No added frame is written on the incoming native stack. */
    move $t1,$sp
    la $sp,np_igr_probe_stack_end
    addiu $sp,$sp,-32
    sw $t1,0($sp)
    sq $ra,16($sp)
    PROBE_BEGIN
    li $t1,1
    sw $t1,16($t0)
    lw $t1,0($sp)
    sw $t1,108($t0)
    sw $sp,136($t0)
    li $t1,1
    sw $t1,140($t0)
    li $t1,256
    sw $t1,20($t0)
    mfc0 $t1,$9
    sw $t1,56($t0)
    PROBE_END 1
    jal np_igr_wait_once
    li $a0,256
    PROBE_BEGIN
    sw $v0,48($t0)
    mfc0 $t1,$9
    sw $t1,60($t0)
    la $t2,np_igr_wait_quarantined
    lui $t1,0x2000
    or $t2,$t2,$t1
    lw $t1,0($t2)
    sw $t1,52($t0)
    PROBE_END 7
    lq $ra,16($sp)
    lw $sp,0($sp)
12:
    jr $ra
    nop
    /* Native handlers have restored their own frames before these tail leaves.
     * No added stack, native call, return-register or job/buffer access. Each
     * counter has one native completion-family writer; nesting is unsupported. */
    .macro PROBE_COMPLETION name,generation,calls
    .balign 16
    .globl \name
\name:
    la $t0,np_igr_probe_async_observation
    lui $t1,0x2000
    or $t0,$t0,$t1
    lw $t1,\generation($t0)
    addiu $t1,$t1,1
    sw $t1,\generation($t0)
    sync
    lw $t2,\calls($t0)
    addiu $t2,$t2,1
    sw $t2,\calls($t0)
    sync
    addiu $t1,$t1,1
    sw $t1,\generation($t0)
    jr $ra
    nop
    .endm
    PROBE_COMPLETION np_igr_probe_rpc_complete,8,12
    PROBE_COMPLETION np_igr_probe_file_complete,16,20
    .balign 16
    .globl np_igr_probe_observation
np_igr_probe_observation:
    .word 0x5957504e,6
    .space 16
    .word -1,-1,-1,-1,-1,-1
    .space 64
    .space 32
    .space 24
    .balign 16
    .globl np_igr_probe_async_observation
np_igr_probe_async_observation:
    .word 0x4147504e,1
    .space 24
#endif
    .globl np_igr_wait_end
np_igr_wait_end:
#if NP_IGR_PROBE
    /* 16-byte guard + 640-byte kernel context + 112-byte probe/wait frames.
     * Keep this disjoint from code/records, within the existing scanout bound. */
    .section ._psxcore_igr_probe_stack,"aw",@progbits
    .balign 16
    .globl np_igr_probe_stack,np_igr_probe_stack_end
np_igr_probe_stack:
    .word 0x5354504e,0x5354504e,0x5354504e,0x5354504e
    .space 752
np_igr_probe_stack_end:
#endif
#endif
