/* Original guard calls for POPS-generated scalar memory accesses. Existing
 * native I/O handlers retain register preservation and guest cycle charges.
 * No exception synthesis, guest clock change or interpreter fallback. */
    .section ._psxcore_hooks,"ax",@progbits
    .set noreorder
    .set noat
    .globl np_memory_emit_finalize
np_memory_emit_finalize:
    /* At the pinned epilogue, its saved caller RA is still on the native stack.
     * Preserve our return and the output cursor; the native epilogue restores
     * the compiler's callee registers. Its original delay slot loaded s1. */
    addiu $sp,$sp,-48
    sq $ra,16($sp)
    sq $12,32($sp) /* native output cursor; EE ABI names register 12 t0 */
    sll $a0,$a0,0 /* normalize the native word to the EE C scalar ABI */
    move $a1,$s4
    /* The checked 0x205d34 redirect captured the PS1 target in a2 before
     * native s0 restoration. Merge accesses do not select return helpers. */
    addiu $a3,$12,-12
    la $8,np_memory_helpers
    la $9,np_memory_return_helpers
    jal np_memory_emit_finalize_rewrite
    nop
    bltz $v0,np_memory_plan_failed
    nop
    beqz $v0,1f
    nop
    la $v1,np_memory_emitted
    lw $v0,0($v1)
    addiu $v0,$v0,1
    sw $v0,0($v1)
1:
    lq $12,32($sp)
    lq $ra,16($sp)
    ld $s0,64($sp) /* original 16(native sp), including the SP-base branch */
    move $v0,$12 /* displaced final cursor result */
    jr $ra
    addiu $sp,$sp,48
np_memory_plan_failed:
    la $a0,np_memory_plan_error
    jal np_hook_string
    nop
2:
    b 2b
    nop

    .globl np_memory_group_finalize
np_memory_group_finalize:
    /* Group epilogue: s1=end, s4=count, s5=PS1 base, s7=descriptors.
     * The displaced delay slot already restored native s0. Allocated EE
     * registers occupy the first eight words of the original native stack. */
    addiu $sp,$sp,-48
    sq $ra,16($sp)
    sq $s1,32($sp)
    move $a0,$s1
    move $a1,$s4
    move $a2,$s5
    move $a3,$s7
    addiu $8,$sp,48
    la $9,np_memory_helpers
    jal np_memory_group_rewrite
    nop
    bltz $v0,np_memory_group_plan_failed
    nop
    la $v1,np_memory_group_emitted
    lw $a0,0($v1)
    addu $a0,$a0,$v0
    sw $a0,0($v1)
    lq $s1,32($sp)
    lq $ra,16($sp)
    move $v0,$s1
    jr $ra
    addiu $sp,$sp,48
np_memory_group_plan_failed:
    move $s1,$v0
    la $a0,np_memory_group_error
    jal np_hook_string
    nop
    move $a0,$s1
    jal np_hook_hex
    nop
    la $a0,np_memory_group_context
    jal np_hook_string
    nop
    move $a0,$s4
    jal np_hook_hex
    nop
    move $a0,$s5
    jal np_hook_hex
    nop
    move $a0,$s7
    jal np_hook_hex
    nop
    addiu $a0,$sp,48
    jal np_hook_hex
    nop
    la $a0,np_memory_error_newline
    jal np_hook_string
    nop
4:
    b 4b
    nop

    .macro NP_MEMORY_GUARD name,instruction,native,store=0
    .globl \name
\name:
    addu $v0,$a0,$a3
    and $v1,$v0,$at
#if NP_MEMORY_RAM_FASTPATH
    /* RAM/mirrors cannot overlap native MMIO or expansion registers. Check
     * this common case before those cold paths without changing any access,
     * helper entry address, register contract or guest cycle charge. */
    lui $a2,0x0080
    sltu $a2,$v1,$a2
    bnez $a2,\name\()_ram
    nop
#endif
    lui $a2,0x1f80
    ori $a2,$a2,0x1000
    subu $a2,$v1,$a2
    sltiu $a2,$a2,0x2000
    bnez $a2,\name\()_io
    nop
    /* Native expansion-register reads return -1; their writes disappear. */
    lui $a2,0x1f00
    ori $a2,$a2,4
    beq $v1,$a2,\name\()_expansion
    nop
    ori $a2,$a2,0x80
    beq $v1,$a2,\name\()_expansion
    nop
    .if \store
    lui $a2,0x1ffe
    ori $a2,$a2,0x130
    beq $v1,$a2,\name\()_expansion
    nop
    .endif
    /* Only RAM/mirrors, scratchpad and BIOS may reach a direct access.
     * Unknown addresses park instead of silently accepting a skipped fault. */
#if !NP_MEMORY_RAM_FASTPATH
    lui $a2,0x0080
    sltu $a2,$v1,$a2
    bnez $a2,\name\()_ram
    nop
#endif
    lui $a2,0x1fc0
    subu $a2,$v1,$a2
    lui $a3,0x0008
    sltu $a2,$a2,$a3
    bnez $a2,\name\()_ram
    nop
    lui $a2,0x1f80
    subu $a2,$v1,$a2
    sltiu $a2,$a2,0x400
    .if \store
    bnez $a2,\name\()_ram
    nop
    b np_memory_unknown_address
    nop
    .else
    /* The pinned emitter charges four RAM read wait states even when a
     * calculated address lands in scratchpad. Its constant-scratch path
     * already refunds four. Apply that same refund only to dynamic reads;
     * preserve instruction charges, stores and all other mapped regions.
     * Reuse the original four words so resident entry addresses stay fixed. */
    beqz $a2,np_memory_unknown_address
    nop
    b \name\()_ram
    addiu $k1,$k1,4
    .endif
\name\()_ram:
    .if \store
    \instruction $a1,0($v0)
    .else
    \instruction $v0,0($v0)
    .endif
    jr $ra
    nop
\name\()_io:
    j \native
    nop
\name\()_expansion:
    jr $ra
    .if \store
    nop
    .else
    li $v0,-1
    .endif
    .endm

    .macro NP_VIDEO_HANDSHAKE
    lui $a2,0x7000
    addiu $a2,$a2,0x10f0
    jal np_video_command_context
    nop
    beqz $a3,.Lhandshake_done\@
    nop
    jal np_video_producer_record
    nop
.Lhandshake_done\@:
    .endm

    NP_MEMORY_GUARD np_memory_lb,lb,0x207ce0
    NP_MEMORY_GUARD np_memory_lh,lh,0x207cf8
    NP_MEMORY_GUARD np_memory_lw,lw,0x207d10
    NP_MEMORY_GUARD np_memory_lbu,lbu,0x207ce0
    NP_MEMORY_GUARD np_memory_lhu,lhu,0x207cf8
    NP_MEMORY_GUARD np_memory_sb,sb,0x207c98,1
    NP_MEMORY_GUARD np_memory_sh,sh,0x207cb0,1
    NP_MEMORY_GUARD np_memory_sw,sw,0x207cc8,1
np_memory_unknown_address:
    /* Diagnostic only on a rejected address; never log on the hot path. */
    move $a1,$v0
    la $a0,np_memory_address_error
    jal np_hook_string
    nop
    move $a0,$a1
    jal np_hook_hex
    nop
3:
    b 3b
    nop

    /* Opt-in guest synchronization observation. Each row contains entry and
     * return counts, the semaphore ID and the original caller. No logging,
     * queue changes or syscall replacement occurs. */
    .macro NP_MEMORY_WAIT name,index
    .globl \name
\name:
    addiu $sp,$sp,-48
    sq $ra,0($sp)
    sq $12,16($sp)
    sq $13,32($sp)
    la $12,np_memory_wait_records+(\index*16)
    lw $13,0($12)
    addiu $13,$13,1
    sw $13,0($12)
    sw $a0,8($12)
    sw $ra,12($12)
    jal 0x22e860
    nop
    .if \index == 4
    /* Dedicated video-wait context; retain the complete native result. */
    la $12,np_video_wait_result
    sq $v0,0($12)
    .endif
    la $12,np_memory_wait_records+(\index*16)
    lw $13,4($12)
    addiu $13,$13,1
    sw $13,4($12)
    lq $ra,0($sp)
    lq $12,16($sp)
    lq $13,32($sp)
    jr $ra
    addiu $sp,$sp,48
    .endm
    NP_MEMORY_WAIT np_memory_wait_writer,0
    NP_MEMORY_WAIT np_memory_wait_writer_tail,1
    NP_MEMORY_WAIT np_memory_wait_dma,2
    NP_MEMORY_WAIT np_memory_wait_reader,3
    NP_MEMORY_WAIT np_memory_wait_video,4
    NP_MEMORY_WAIT np_memory_wait_worker,5

    /* Both redirects keep their original delay slots. Save every register
     * touched by the recorder, including the 128-bit native result. */
    .macro NP_VIDEO_SAVE
    addiu $sp,$sp,-160
    sq $v0,0($sp)
    sq $v1,16($sp)
    sq $a0,32($sp)
    sq $a1,48($sp)
    sq $a2,64($sp)
    sq $a3,80($sp)
    sq $8,96($sp)
    sq $9,112($sp)
    sq $10,128($sp)
    sq $ra,144($sp)
    .endm

    /* The semaphore wait at 0x21aec8 has already returned. During the
     * demonstrated pending NTSC transition, test the cached native CRT's
     * interlace state rather than the uncommitted requested state. All
     * other tuples retain the native predicate and branch-likely semantics.
     * No guest field, semaphore, queue or cycle budget is changed. */
    .globl np_video_interlace_applied
np_video_interlace_applied:
    addiu $sp,$sp,-48
    sq $v1,0($sp)
    sq $a0,16($sp)
    sq $a1,32($sp)
    lw $v0,0x2214($s0)
    li $v1,1
    bne $v0,$v1,9f
    nop
    lw $a1,0x22a4($s0)
    bne $a1,$v1,9f
    nop
    lw $a1,0x264c($s0)
    bnez $a1,9f
    nop
    lw $a1,0x2218($s0)
    bnez $a1,9f
    nop
    li $v1,2
    lw $a1,0x2210($s0)
    bne $a1,$v1,9f
    nop
    lui $a0,0x30
    addiu $a0,$a0,-4144
    lw $a1,0($a0)
    bnez $a1,9f
    nop
    lw $a1,4($a0)
    bne $a1,$v1,9f
    nop
    lw $a1,8($a0)
    li $v1,1
    bne $a1,$v1,9f
    nop
    la $v1,np_video_transition_selected
    lw $a1,0($v1)
    addiu $a1,$a1,1
    sw $a1,0($v1)
    lw $v0,0($a0)
9:
    lq $a1,32($sp)
    lq $a0,16($sp)
    lq $v1,0($sp)
    addiu $sp,$sp,48
    beqz $v0,1f
    nop
    j 0x21aee8
    nop
1:
    j 0x21af0c
    ld $s0,0($sp)

    .macro NP_VIDEO_RESTORE
    lq $ra,144($sp)
    lq $10,128($sp)
    lq $9,112($sp)
    lq $8,96($sp)
    lq $a3,80($sp)
    lq $a2,64($sp)
    lq $a1,48($sp)
    lq $a0,32($sp)
    lq $v1,16($sp)
    lq $v0,0($sp)
    addiu $sp,$sp,160
    .endm

    /* Entry snapshot uses the otherwise idle main-thread producer ring.
     * The original caller is still saved by POPS at 8(original sp). */
    .globl np_video_trace_entry
np_video_trace_entry:
    addiu $s0,$v0,0x10f0
    NP_VIDEO_SAVE
    la $8,np_video_trace_entries
    lw $v0,0($8)
    addiu $v0,$v0,1
    sw $v0,0($8)
    la $9,np_video_trace_caller
    lw $v1,168($sp)
    sw $v1,0($9)
    dsll32 $a1,$v0,0
    dsrl32 $a1,$a1,0
    dsll32 $v1,$v1,0
    or $a1,$a1,$v1
    li $a0,4
    move $a2,$s0
    jal np_video_command_context
    nop
    beqz $a3,1f
    nop
    jal np_video_producer_record
    nop
1:
    NP_VIDEO_RESTORE
    jr $ra
    nop

    .globl np_video_trace_decision
np_video_trace_decision:
    NP_VIDEO_SAVE
    move $a2,$v1
    move $a1,$a0
    move $a0,$zero
    jal np_video_trace_record
    nop
    NP_VIDEO_RESTORE
    bne $a0,$v1,1f
    nop
    j 0x21aec8
    nop
1:
    j 0x21af08
    nop

    .globl np_video_trace_exit
np_video_trace_exit:
    NP_VIDEO_SAVE
    move $a2,$v1
    move $a1,$a0
    li $a0,3
    jal np_video_trace_record
    nop
    NP_VIDEO_RESTORE
    j 0x21af14
    lui $a0,0x8000

/* One video-context writer. A record is complete only when end=start+1.
 * kind: 1=loop back, 2=branch releases, 3=common routine exit. */
np_video_trace_record:
    la $8,np_video_trace_written
    lw $9,0($8)
    addiu $10,$9,1
    sw $10,0($8)
    andi $9,$9,63
    sll $9,$9,6
    la $8,np_video_trace_records
    addu $8,$8,$9
    sw $zero,60($8)
    sll $10,$10,1
    ori $10,$10,1
    sw $10,0($8)
    bnez $a0,2f
    nop
    xor $v0,$a1,$a2
    sltiu $v0,$v0,1
    addiu $a0,$v0,2
    beqz $v0,2f
    nop
    li $a0,1
    la $9,np_video_trace_loops
    lw $v1,0($9)
    addiu $v1,$v1,1
    sw $v1,0($9)
    b 2f
    nop
2:
    li $v0,3
    bne $a0,$v0,3f
    nop
    la $9,np_video_trace_exits
    lw $v1,0($9)
    addiu $v1,$v1,1
    sw $v1,0($9)
3:
    sw $a0,4($8)
    la $9,np_video_trace_entries
    lw $v0,0($9)
    sw $v0,8($8)
    la $9,np_memory_wait_records+68
    lw $v0,0($9)
    sw $v0,12($8)
    la $9,np_video_wait_result
    lw $v0,0($9)
    lw $v1,4($9)
    sw $v0,16($8)
    sw $v1,56($8)
    la $9,np_video_trace_caller
    lw $v0,0($9)
    sw $v0,20($8)
    sw $a1,24($8)
    sw $a2,28($8)
    lui $9,0x7000
    addiu $9,$9,0x10f0
    lw $v0,0x264c($9)
    sw $v0,32($8)
    lw $v0,0x2214($9)
    sw $v0,36($8)
    lw $v0,0x21c8($9)
    sw $v0,40($8)
    lw $v0,0x225c($9)
    sw $v0,44($8)
    lw $v0,0x2268($9)
    sw $v0,48($8)
    lw $v0,0x2258($9)
    sw $v0,52($8)
    addiu $10,$10,1
    sw $10,60($8)
    jr $ra
    nop
    /* Separate interrupt, main-thread and worker rings prevent cross-context writers.
     * CSR is the original single read; replay its shift after recording it. */
    .globl np_video_trace_csr
np_video_trace_csr:
    ld $v0,0($v1)
    NP_VIDEO_SAVE
    li $a0,1
    move $a1,$v0
    move $a2,$s0
    la $a3,np_video_csr_written
    dsrl $v0,$a1,13
    andi $v0,$v0,1
    sll $v0,$v0,2
    addu $9,$a3,$v0
    lw $v0,4($9)
    addiu $v0,$v0,1
    sw $v0,4($9)
    sw $a1,12($a3)
    jal np_video_producer_record
    nop
    NP_VIDEO_RESTORE
    j 0x21cd94
    dsll $v0,$v0,19

    /* Original sd s0,0(sp) remains in the redirect's delay slot. */
    .globl np_video_trace_command
np_video_trace_command:
    NP_VIDEO_SAVE
    li $a0,2
    move $a1,$s2
    move $a2,$s1
    jal np_video_command_context
    nop
    beqz $a3,1f
    nop
    jal np_video_producer_record
    nop
1:
    NP_VIDEO_RESTORE
    j 0x221730
    srl $v1,$s2,24

    /* The original branch delay slot has already published the latch. */
    .globl np_video_trace_latch
np_video_trace_latch:
    NP_VIDEO_SAVE
    li $a0,3
    move $a1,$s2
    move $a2,$s1
    jal np_video_command_context
    nop
    beqz $a3,1f
    nop
    jal np_video_producer_record
    nop
1:
    NP_VIDEO_RESTORE
    j 0x221c48
    nop

    /* Redirect the original tail jumps, retaining their stack-pop delay
     * slots. Each wrapper calls the original target exactly once. */
    .globl np_video_trace_reset
np_video_trace_reset:
    NP_VIDEO_SAVE
    li $a0,5
    ld $a1,32($sp)
    NP_VIDEO_HANDSHAKE
    NP_VIDEO_RESTORE
    addiu $sp,$sp,-16
    sq $ra,0($sp)
    jal 0x21ca60
    nop
    lq $ra,0($sp)
    addiu $sp,$sp,16
    NP_VIDEO_SAVE
    li $a0,6
    move $a1,$v0
    NP_VIDEO_HANDSHAKE
    NP_VIDEO_RESTORE
    jr $ra
    nop

    .globl np_video_trace_crt
np_video_trace_crt:
    NP_VIDEO_SAVE
    lw $a1,32($sp)
    andi $a1,$a1,255
    lw $v0,48($sp)
    andi $v0,$v0,255
    sll $v0,$v0,8
    or $a1,$a1,$v0
    lw $v0,64($sp)
    andi $v0,$v0,255
    sll $v0,$v0,16
    or $a1,$a1,$v0
    li $a0,7
    NP_VIDEO_HANDSHAKE
    NP_VIDEO_RESTORE
    addiu $sp,$sp,-16
    sq $ra,0($sp)
    jal 0x22e420
    nop
    lq $ra,0($sp)
    addiu $sp,$sp,16
    NP_VIDEO_SAVE
    li $a0,8
    move $a1,$v0
    NP_VIDEO_HANDSHAKE
    NP_VIDEO_RESTORE
    jr $ra
    nop

/* Pinned graphics-worker stack: 0x852980 + 0x8000. Pinned main-thread setup
 * requests the top 1 MiB of the checked 32 MiB EE RAM. Unknown stacks produce
 * a sticky diagnostic flag, never a shared fallback ring or a native call. */
np_video_command_context:
    addiu $9,$sp,160
    li $8,0x852980
    sltu $v0,$9,$8
    bnez $v0,1f
    nop
    li $8,0x85a980
    sltu $v0,$9,$8
    beqz $v0,1f
    nop
    la $a3,np_video_worker_written
    jr $ra
    nop
1:
    li $8,0x1f00000
    sltu $v0,$9,$8
    bnez $v0,2f
    nop
    li $8,0x2000000
    sltu $v0,$9,$8
    beqz $v0,2f
    nop
    la $a3,np_video_command_written
    jr $ra
    nop
2:
    la $8,np_video_unknown_context
    li $v0,1
    sw $v0,0($8)
    jr $ra
    move $a3,$zero

/* Per-context 32-record ring, 16 words each. Only record framing is coherent;
 * the guest fields remain sequential samples, including interrupt changes. */
np_video_producer_record:
    lw $9,0($a3)
    addiu $10,$9,1
    sw $10,0($a3)
    andi $9,$9,31
    sll $9,$9,6
    addiu $8,$a3,16
    addu $8,$8,$9
    sw $zero,60($8)
    sll $10,$10,1
    ori $10,$10,1
    sw $10,0($8)
    sw $a0,4($8)
    sw $a1,8($8)
    dsrl $v0,$a1,32
    sw $v0,12($8)
    sltiu $v0,$a0,4
    beqz $v0,np_video_handshake_fields
    nop
    lw $v0,0x225c($a2)
    sw $v0,16($8)
    lw $v0,0x2268($a2)
    sw $v0,20($8)
    lw $v0,0x21c8($a2)
    sw $v0,24($8)
    lw $v0,0x264c($a2)
    sw $v0,28($8)
    lw $v0,0x2214($a2)
    sw $v0,32($8)
    lw $v0,0x2258($a2)
    sw $v0,36($8)
    lw $v0,0x223c($a2)
    sw $v0,40($8)
    lw $v0,0x22c4($a2)
    sw $v0,44($8)
    lw $v0,0x22a8($a2)
    sw $v0,48($8)
    lw $v0,0x22a4($a2)
    sw $v0,52($8)
    addiu $v0,$sp,160
    sw $v0,56($8)
np_video_producer_finish:
    addiu $10,$10,1
    sw $10,60($8)
    jr $ra
    nop

/* Kinds 4..8 reuse the 16-word producer record: entry, reset begin/end,
 * actual CRT syscall begin/end. Each counter retains its original sole
 * context writer. Bounds bracket field reads, not an atomic guest snapshot. */
np_video_handshake_fields:
    la $9,np_video_worker_written
    lw $v0,0($9)
    sw $v0,44($8)
    la $9,np_video_csr_written
    lw $v0,0($9)
    sw $v0,52($8)
    lw $v0,0x225c($a2)
    sw $v0,16($8)
    lw $v0,0x2268($a2)
    sw $v0,20($8)
    lw $v0,0x21c8($a2)
    sw $v0,24($8)
    li $v0,4
    bne $a0,$v0,1f
    nop
    dsrl32 $v0,$a1,0
    b 2f
    nop
1:
    lw $v0,144($sp)
2:
    sw $v0,32($8)
    lw $v0,0x2258($a2)
    sw $v0,36($8)
    lw $v0,0x2214($a2)
    andi $v1,$v0,1
    lw $v0,0x264c($a2)
    sltu $v0,$zero,$v0
    sll $v0,$v0,1
    or $v1,$v1,$v0
    lw $v0,0x22a4($a2)
    sltu $v0,$zero,$v0
    sll $v0,$v0,2
    or $v1,$v1,$v0
    lw $v0,0x2218($a2)
    andi $v0,$v0,1
    sll $v0,$v0,3
    or $v1,$v1,$v0
    lw $v0,0x2210($a2)
    andi $v0,$v0,255
    sll $v0,$v0,8
    or $v1,$v1,$v0
    sw $v1,28($8)
    lui $9,0x30
    addiu $9,$9,-4144
    lw $v0,0($9)
    andi $v1,$v0,255
    lw $v0,4($9)
    andi $v0,$v0,255
    sll $v0,$v0,8
    or $v1,$v1,$v0
    lw $v0,8($9)
    andi $v0,$v0,255
    sll $v0,$v0,16
    or $v1,$v1,$v0
    sw $v1,40($8)
    la $9,np_video_worker_written
    lw $v0,0($9)
    sw $v0,48($8)
    la $9,np_video_csr_written
    lw $v0,0($9)
    b np_video_producer_finish
    sw $v0,56($8)
    .balign 4
    .globl np_memory_code_end
np_memory_code_end:
    .globl np_memory_helpers
np_memory_helpers:
    .word np_memory_lb,np_memory_lh,0,np_memory_lw
    .word np_memory_lbu,np_memory_lhu,0,0
    .word np_memory_sb,np_memory_sh,0,np_memory_sw
    .word 0,0,0,0
    .globl np_memory_emitted
np_memory_emitted:
    .word 0
    .globl np_memory_group_emitted
np_memory_group_emitted:
    .word 0
    .globl np_video_transition_selected
np_video_transition_selected:
    .word 0
    .globl np_memory_wait_records
np_memory_wait_records:
    .space 96
    .balign 16
    .globl np_video_trace_entries,np_video_trace_loops,np_video_trace_exits
    .globl np_video_trace_written,np_video_trace_records
np_video_trace_entries:
    .word 0
np_video_trace_loops:
    .word 0
np_video_trace_exits:
    .word 0
np_video_trace_written:
    .word 0
np_video_trace_caller:
    .word 0
    .balign 16
np_video_wait_result:
    .space 16
    .balign 16
np_video_trace_records:
#if NP_AUDIO_GAP_DECLICK || NP_SPU_CAPTURE_IRQ || NP_SPU_REVERB_READONLY
#if NP_MEMORY_ACCESS_TRACE
#error "SPU adapters reuse the inactive video trace ring; disable tracing"
#endif
#if NP_AUDIO_GAP_DECLICK
#include "audio.S"
#endif
#if NP_SPU_CAPTURE_IRQ
#include "spucapture.S"
#endif
#if NP_SPU_REVERB_READONLY
#include "spureverb.S"
#endif
    .space 4096-(.-np_video_trace_records)
#else
    .space 4096
#endif
    .balign 16
    .globl np_video_csr_written,np_video_csr_records
np_video_csr_written:
    .word 0
    .globl np_video_csr_zero,np_video_csr_one,np_video_csr_last
np_video_csr_zero:
    .word 0
np_video_csr_one:
    .word 0
np_video_csr_last:
    .word 0
np_video_csr_records:
#if NP_GPU_DMA_RASTER_SPLIT
#if NP_MEMORY_ACCESS_TRACE
#error "DMA accounting owns the inactive CSR trace area"
#endif
#include "dmacycles.S"
    .space 2048-(.-np_video_csr_records)
#else
    .space 2048
#endif
    .globl np_video_command_written,np_video_command_records
np_video_command_written:
    .space 16
np_video_command_records:
    .space 2048
    .globl np_video_worker_written,np_video_worker_records,np_video_unknown_context
np_video_worker_written:
    .space 16
np_video_worker_records:
    .space 2048
np_video_unknown_context:
    .word 0
np_memory_plan_error:
    .asciz "PSXCORE rejected=memory-emitter-reservation\n"
np_memory_group_error:
    .asciz "PSXCORE rejected=memory-group-reservation stage="
np_memory_group_context:
    .asciz " count/base/records/frame="
np_memory_error_newline:
    .asciz "\n"
np_memory_address_error:
    .asciz "PSXCORE rejected=memory-address value="
    .balign 4
    .globl np_memory_end
np_memory_end:
