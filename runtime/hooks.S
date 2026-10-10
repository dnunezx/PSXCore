/* Original emulator-only interface tracing. No displaced runtime instructions. */
    .section ._psxcore_hooks,"ax",@progbits
    .set noreorder
    .set noat
    .globl np_mount_hook
np_mount_hook:
    addiu $sp,$sp,-128
    sq $a0,32($sp)
    sq $a1,48($sp)
    sq $ra,96($sp)
    sw $zero,112($sp)
    jal 0x00233650
    nop
    sq $v0,64($sp)
    sq $v1,80($sp)
    la $a0,mount_prefix
    jal np_hook_string
    nop
    lw $a0,32($sp)
    jal np_hook_string
    nop
    la $a0,source_prefix
    jal np_hook_string
    nop
    lw $a0,48($sp)
    jal np_hook_string
    nop
    b np_hook_result
    nop
/* Only the first, version-checked pfs1: mount call uses this entry point.
 * Resident storage survives low-memory runtime loading. No shared-card fallback. */
    .globl np_save_mount_hook
np_save_mount_hook:
    la $a1,np_save_partition
    b np_mount_hook
    nop
    .globl np_disc_hook
np_disc_hook:
    addiu $sp,$sp,-128
    sq $a0,32($sp)
    sq $ra,96($sp)
    li $t0,1
    sw $t0,112($sp)
    jal 0x00210540
    nop
    sq $v0,64($sp)
    sq $v1,80($sp)
    la $a0,disc_prefix
    jal np_hook_string
    nop
    lw $a0,32($sp)
    jal np_hook_string
    nop
    b np_hook_result
    nop
    .globl np_card_hook
np_card_hook:
    la $t0,np_native_request_enabled
    lw $t0,0($t0)
    beqz $t0,40f
    nop
    la $t0,np_save_ready
    lw $t0,0($t0)
    beqz $t0,np_native_stop
    nop
40:
    addiu $sp,$sp,-128
    sq $ra,96($sp)
    li $t0,2
    sw $t0,112($sp)
    jal 0x0020cf00
    nop
    sq $v0,64($sp)
    sq $v1,80($sp)
    la $a0,card_prefix
    jal np_hook_string
    nop
np_hook_result:
    la $a0,result_prefix
    jal np_hook_string
    nop
    lw $a0,64($sp)
    jal np_hook_hex
    nop
    la $a0,newline
    jal np_hook_string
    nop
    la $t0,np_native_request_enabled
    lw $t0,0($t0)
    beqz $t0,41f
    nop
    lw $t0,112($sp)
    lw $t1,64($sp)
    bnez $t0,42f
    nop
    bnez $t1,np_native_stop /* mount success must be zero */
    nop
    lw $t0,32($sp)
    lbu $t0,3($t0) /* checked native mount destination: pfs1: or pfs0: */
    li $t1,49
    bne $t0,$t1,41f
    nop
    la $t0,np_save_ready
    li $t1,1
    sw $t1,0($t0)
    b 41f
    nop
42:
    li $t0,1
    bne $t1,$t0,np_native_stop /* disc initializer success; card return is insufficient */
    nop
    lw $t0,112($sp)
    li $t1,2
    bne $t0,$t1,41f
    nop
    /* The pinned native card initializer returns one on failure too. Each
     * slot's bit zero is set only after its 128 KiB read/header validation.
     * Slot stride is 0x20010; the runtime hash guards these state addresses. */
    li $t0,0x00507920
    lw $t1,0($t0)
    andi $t1,$t1,1
    beqz $t1,np_native_stop
    nop
    li $t0,0x00527930
    lw $t1,0($t0)
    andi $t1,$t1,1
    beqz $t1,np_native_stop
    nop
    la $a0,cards_ready
    .globl np_card_ready_notify
np_card_ready_notify:
    jal np_hook_string
    nop
41:
    lw $t0,112($sp)
    li $t1,1
    bne $t0,$t1,4f
    nop
    la $t0,np_disc_probe_enabled
    lw $t0,0($t0)
    bnez $t0,np_disc_probe
    nop
4:
    lq $v0,64($sp)
    lq $v1,80($sp)
    lq $ra,96($sp)
    jr $ra
    addiu $sp,$sp,128
np_native_stop:
    la $t0,np_trace_enabled
    li $t1,1
    sw $t1,0($t0)
    la $a0,native_failed
    jal np_hook_string
    nop
43:
    b 43b
    nop
/* Native save calls have at most three arguments. Reserve their ABI home
 * area; keep exact wide v0/v1/ra and the original write length across calls.
 * A failing guard never returns to the worker's dirty-bit clearance. */
    .macro NP_SAVE_CHECK name,target,stage,write=0
    .globl \name
\name:
    addiu $sp,$sp,-96
    sw $a2,32($sp)
    sq $ra,48($sp)
#if NP_IGR_OBSERVE
    .if \stage == 9
    sq $t0,0($sp)
    sq $t1,16($sp)
    jal np_igr_wait_enter
    nop
    lq $t0,0($sp)
    lq $t1,16($sp)
    .endif
#endif
    jal \target
    nop
    sq $v0,64($sp)
    sq $v1,80($sp)
#if NP_IGR_OBSERVE
    .if \stage == 9
    jal np_igr_wait_exit
    nop
    .endif
#endif
    la $t1,np_native_request_enabled
    lw $t1,0($t1)
    beqz $t1,1f
    nop
    sll $t0,$v0,0 /* sign-extend the native 32-bit result */
    .if \write
    lw $t1,32($sp)
    beq $t0,$t1,2f
    nop
    .else
    bgez $t0,2f
    nop
    .endif
    li $a0,\stage
    move $a1,$t0
    b np_save_failed
    nop
2:
    .if \stage == 3
    /* Flush the complete, closed backup before allowing its replacement.
     * Preserve the close call's exact results for the original caller. */
    la $a0,np_save_sync_mount
    move $a1,$zero
    jal 0x2334b8
    nop
    sll $t0,$v0,0
    bgez $t0,1f
    nop
    li $a0,10 /* backup pre-replacement flush */
    move $a1,$t0
    b np_save_failed
    nop
    .endif
    .if \stage == 5
    la $t0,np_save_flush_count
    lw $t1,0($t0)
    addiu $t1,$t1,1
    sw $t1,0($t0)
    la $a0,save_flushed
    jal np_hook_string
    nop
    .endif
1:
    lq $v0,64($sp)
    lq $v1,80($sp)
    lq $ra,48($sp)
    jr $ra
    addiu $sp,$sp,96
    .endm
    NP_SAVE_CHECK np_save_open_hook,0x231358,1
    NP_SAVE_CHECK np_save_write_hook,0x231c08,2,1
    NP_SAVE_CHECK np_save_close_hook,0x2315e0,3
    NP_SAVE_CHECK np_save_rename_hook,0x2332a8,4
    NP_SAVE_CHECK np_save_sync_hook,0x2334b8,5
    NP_SAVE_CHECK np_save_sema_hook,0x22e820,6
    NP_SAVE_CHECK np_save_thread_hook,0x22e620,7
    NP_SAVE_CHECK np_save_start_hook,0x22e640,8
    NP_SAVE_CHECK np_save_wait_hook,0x22e860,9
np_save_failed:
    la $t0,np_save_error_stage
    sw $a0,0($t0)
    la $t0,np_save_error_result
    sw $a1,0($t0)
    la $t0,np_trace_enabled
    li $t1,1
    sw $t1,0($t0)
    la $a0,save_failed
    jal np_hook_string
    nop
    la $t0,np_save_error_stage
    lw $a0,0($t0)
    jal np_hook_hex
    nop
    la $a0,result_prefix
    jal np_hook_string
    nop
    la $t0,np_save_error_result
    lw $a0,0($t0)
    jal np_hook_hex
    nop
    la $a0,save_stopped
    jal np_hook_string
    nop
    /* Parking the save worker alone would let gameplay continue. Freeze EE
     * scheduling after the synchronous failed operation/diagnostic completes.
     * This is a fatal guard, not a recovery or graceful-shutdown procedure. */
    di
    sync
44:
    b 44b
    nop
/* Bounded strings and fixed eight-digit words through the EE serial FIFO. */
    .globl np_hook_string
np_hook_string:
    la $t0,np_trace_enabled
    lw $t0,0($t0)
    beqz $t0,2f
    nop
    lui $t0,0x1001
    li $t1,128
1:
    lbu $t2,0($a0)
    beqz $t2,2f
    nop
    sb $t2,-3712($t0)
    addiu $a0,$a0,1
    addiu $t1,$t1,-1
    bnez $t1,1b
    nop
2:
    jr $ra
    nop
/* Positive fixture mode stops here before native cards or PS1 execution. */
np_disc_probe:
    lw $t0,64($sp)
    li $t1,1
    bne $t0,$t1,np_probe_fail
    nop
    la $t0,np_disc_probe_enabled
    lw $t0,0($t0)
    li $t1,2
    beq $t0,$t1,np_game_probe
    nop
    lui $a0,0x005b
    addiu $a0,$a0,-29248 /* native TOC returned by the runtime */
    li $a1,1024
    jal np_hook_fnv
    nop
    move $s3,$v0
    la $a0,toc_prefix
    jal np_hook_string
    nop
    move $a0,$s3
    jal np_hook_hex
    nop
    la $a0,newline
    jal np_hook_string
    nop
    li $t0,1868637659 /* FNV-1a of our original 128-sector fixture's TOC */
    bne $s3,$t0,np_probe_fail
    nop
    la $s0,probe_lbas
    li $s1,3
5:
    lw $s2,0($s0)
    move $a0,$s2
    jal 0x00210090 /* native synchronous sector reader, not a replacement */
    nop
    li $s3,-1
    lui $t0,0x0020
    sltu $t0,$v0,$t0
    bnez $t0,7f
    nop
    li $t0,0x00865000
    sltu $t0,$v0,$t0
    beqz $t0,7f
    nop
    move $s3,$zero
    sll $t0,$s2,4
    addu $t0,$t0,$s2
    li $t1,2352
6:
    lbu $t2,0($v0)
    andi $t3,$t0,255
    xor $t2,$t2,$t3
    sltu $t2,$zero,$t2
    addu $s3,$s3,$t2
    addiu $v0,$v0,1
    addiu $t0,$t0,1
    addiu $t1,$t1,-1
    bnez $t1,6b
    nop
7:
    la $a0,sector_prefix
    jal np_hook_string
    nop
    move $a0,$s2
    jal np_hook_hex
    nop
    la $a0,errors_prefix
    jal np_hook_string
    nop
    move $a0,$s3
    jal np_hook_hex
    nop
    la $a0,newline
    jal np_hook_string
    nop
    bnez $s3,np_probe_fail
    nop
    addiu $s0,$s0,4
    addiu $s1,$s1,-1
    bnez $s1,5b
    nop
    la $a0,probe_pass
    jal np_hook_string
    nop
    b np_probe_park
    nop
/* Hash native synchronous reads against independently validated Spyro sectors.
 * Park before cards, stock title checks, or PS1 execution. No payload edits. */
np_game_probe:
    lui $a0,0x005b
    addiu $a0,$a0,-29248
    li $a1,1024
    jal np_hook_fnv
    nop
    move $s3,$v0
    la $a0,toc_prefix
    jal np_hook_string
    nop
    move $a0,$s3
    jal np_hook_hex
    nop
    la $a0,newline
    jal np_hook_string
    nop
    li $t0,285235569
    bne $s3,$t0,np_probe_fail
    nop
    la $s0,game_probe_lbas
    li $s1,7
13:
    lw $s2,0($s0)
    move $a0,$s2
    jal 0x00210090
    nop
    lui $t0,0x0020
    sltu $t0,$v0,$t0
    bnez $t0,np_probe_fail
    nop
    li $t0,0x00865000-2352
    sltu $t0,$v0,$t0
    beqz $t0,np_probe_fail
    nop
    move $a0,$v0
    li $a1,2352
    jal np_hook_fnv
    nop
    move $s3,$v0
    la $a0,game_sector_prefix
    jal np_hook_string
    nop
    move $a0,$s2
    jal np_hook_hex
    nop
    la $a0,fnv_prefix
    jal np_hook_string
    nop
    move $a0,$s3
    jal np_hook_hex
    nop
    la $a0,newline
    jal np_hook_string
    nop
    addiu $s0,$s0,4
    addiu $s1,$s1,-1
    bnez $s1,13b
    nop
    la $a0,game_probe_complete
    jal np_hook_string
    nop
    b np_probe_park
    nop
np_probe_fail:
    la $a0,probe_fail
    jal np_hook_string
    nop
np_probe_park:
    b np_probe_park
    nop
np_hook_fnv:
    li $v0,2166136261
    li $t1,16777619
8:
    lbu $t0,0($a0)
    xor $v0,$v0,$t0
    multu $v0,$t1
    mflo $v0
    addiu $a0,$a0,1
    addiu $a1,$a1,-1
    bnez $a1,8b
    nop
    jr $ra
    nop
    .globl np_hook_hex
np_hook_hex:
    la $t0,np_trace_enabled
    lw $t0,0($t0)
    bnez $t0,40f
    nop
    jr $ra
    nop
40:
    lui $t0,0x1001
    li $t1,28
    la $t2,hex_digits
3:
    srlv $t3,$a0,$t1
    andi $t3,$t3,15
    addu $t3,$t3,$t2
    lbu $t3,0($t3)
    sb $t3,-3712($t0)
    addiu $t1,$t1,-4
    bgez $t1,3b
    nop
    jr $ra
    nop
/* Trace bounded startup interfaces, preserving all eight register arguments. */
    .macro NP_POST name,target,tag
    .globl \name
\name:
    addiu $sp,$sp,-192
    sq $a0,0($sp)
    sq $a1,16($sp)
    sq $a2,32($sp)
    sq $a3,48($sp)
    sq $t0,64($sp)
    sq $t1,80($sp)
    sq $t2,96($sp)
    sq $t3,112($sp)
    sq $ra,128($sp)
    la $a0,\tag
    jal np_hook_string
    nop
    la $a0,enter_suffix
    jal np_hook_string
    nop
    lq $a0,0($sp)
    lq $a1,16($sp)
    lq $a2,32($sp)
    lq $a3,48($sp)
    lq $t0,64($sp)
    lq $t1,80($sp)
    lq $t2,96($sp)
    lq $t3,112($sp)
    jal \target
    nop
    sq $v0,144($sp)
    sq $v1,160($sp)
    la $a0,\tag
    jal np_hook_string
    nop
    la $a0,result_prefix
    jal np_hook_string
    nop
    lw $a0,144($sp)
    jal np_hook_hex
    nop
    la $a0,newline
    jal np_hook_string
    nop
    lq $v0,144($sp)
    lq $v1,160($sp)
    lq $ra,128($sp)
    jr $ra
    addiu $sp,$sp,192
    .endm
    NP_POST np_post_0,0x22d2e0,post_0
    NP_POST np_post_1,0x22d480,post_1
    NP_POST np_post_2,0x22dc30,post_2
    NP_POST np_post_3,0x214418,post_3
    NP_POST np_post_4,0x20c750,post_4
    NP_POST np_post_5,0x200940,post_5
    NP_POST np_post_6,0x200d10,post_6
/* No-argument core iteration: emit only the first three entry/return pairs. */
    .globl np_core_hook
np_core_hook:
    addiu $sp,$sp,-64
    sq $ra,0($sp)
    la $t0,core_count
    lw $t1,0($t0)
    sltiu $t2,$t1,3
    sw $t2,48($sp)
    beqz $t2,9f
    nop
    addiu $t1,$t1,1
    sw $t1,0($t0)
    la $a0,core_enter
    jal np_hook_string
    nop
9:
    jal 0x200d50
    nop
    sq $v0,16($sp)
    sq $v1,32($sp)
    lw $t0,48($sp)
    beqz $t0,10f
    nop
    la $a0,core_return
    jal np_hook_string
    nop
10:
    lq $ra,0($sp)
    lq $v0,16($sp)
    lq $v1,32($sp)
    jr $ra
    addiu $sp,$sp,64
/* Preserve the eight register arguments and ninth stack argument to SifCallRpc.
 * Log the first sixteen native read requests; never touch the data payload. */
    .globl np_sector_rpc_hook
np_sector_rpc_hook:
    addiu $sp,$sp,-208
    sq $a0,16($sp)
    sq $a1,32($sp)
    sq $a2,48($sp)
    sq $a3,64($sp)
    sq $t0,80($sp)
    sq $t1,96($sp)
    sq $t2,112($sp)
    sq $t3,128($sp)
    sq $ra,144($sp)
    lw $t0,208($sp)
    sw $t0,0($sp)
    la $t0,rpc_count
    lw $t1,0($t0)
    sltiu $t2,$t1,16
    sw $t2,192($sp)
    beqz $t2,11f
    nop
    addiu $t1,$t1,1
    sw $t1,0($t0)
    la $a0,rpc_prefix
    jal np_hook_string
    nop
    lw $t0,64($sp)
    lw $a0,0($t0)
    jal np_hook_hex
    nop
    la $a0,rpc_count_prefix
    jal np_hook_string
    nop
    lw $t0,64($sp)
    lw $a0,4($t0)
    jal np_hook_hex
    nop
    la $a0,rpc_mode_prefix
    jal np_hook_string
    nop
    lw $a0,48($sp)
    jal np_hook_hex
    nop
    la $a0,newline
    jal np_hook_string
    nop
    jal np_cache_snapshot
    nop
11:
    lq $a0,16($sp)
    lq $a1,32($sp)
    lq $a2,48($sp)
    lq $a3,64($sp)
    lq $t0,80($sp)
    lq $t1,96($sp)
    lq $t2,112($sp)
    lq $t3,128($sp)
    jal 0x230330
    nop
    sq $v0,160($sp)
    sq $v1,176($sp)
    lw $t0,192($sp)
    beqz $t0,12f
    nop
    la $a0,rpc_return_prefix
    jal np_hook_string
    nop
    lw $a0,160($sp)
    jal np_hook_hex
    nop
    la $a0,newline
    jal np_hook_string
    nop
    lw $t0,48($sp)
    bnez $t0,12f /* Async submission does not mean its data has arrived. */
    nop
    jal np_cache_snapshot
    nop
12:
    lq $ra,144($sp)
    lq $v0,160($sp)
    lq $v1,176($sp)
    jr $ra
    addiu $sp,$sp,208
/* Trace the native callback's existing semaphore call, leaving the callback
 * pointer and RPC arguments intact. This path has no EE receive buffer. */
    .globl np_rpc_completion_hook
np_rpc_completion_hook:
    addiu $sp,$sp,-64
    sq $ra,0($sp)
    jal 0x22e850
    nop
    sq $v0,16($sp)
    sq $v1,32($sp)
    la $t0,completion_count
    lw $t1,0($t0)
    sltiu $t2,$t1,16
    beqz $t2,16f
    nop
    addiu $t1,$t1,1
    sw $t1,0($t0)
    la $a0,completion_prefix
    jal np_hook_string
    nop
    lw $a0,16($sp)
    jal np_hook_hex
    nop
    la $a0,newline
    jal np_hook_string
    nop
16:
    lq $ra,0($sp)
    lq $v0,16($sp)
    lq $v1,32($sp)
    jr $ra
    addiu $sp,$sp,64
/* Both verified core-facing interfaces take one LBA argument. Log only changed
 * LBA/result pairs, capped at 64 per interface. Hash only returned cache data. */
    .macro NP_READER name,target,tag,state
    .globl \name
\name:
    addiu $sp,$sp,-96
    sq $a0,0($sp)
    sq $ra,16($sp)
    jal \target
    nop
    sq $v0,32($sp)
    sq $v1,48($sp)
    lw $a0,0($sp)
    move $a1,$v0
    jal np_license_adapter
    nop
    lq $v0,32($sp)
    lq $v1,48($sp)
    la $t0,np_asset_trace_enabled
    lw $t0,0($t0)
    beqz $t0,25f
    nop
    /* Spyro's validated files: WAD starts at 37; boot EXE occupies
     * [53875,54079). Keep the bounded budget for actual asset reads. */
    lw $t3,0($sp)
    sltiu $t0,$t3,37
    bnez $t0,19f
    nop
    li $t0,53875
    sltu $t0,$t3,$t0
    bnez $t0,25f
    nop
    li $t0,54079
    sltu $t0,$t3,$t0
    bnez $t0,19f
    nop
25:
    la $t0,\state
    lw $t1,0($t0)
    sltiu $t2,$t1,64
    beqz $t2,19f
    nop
    lw $t2,4($t0)
    lw $t3,0($sp)
    bne $t2,$t3,17f
    nop
    lw $t2,8($t0)
    beq $t2,$v0,19f
    nop
17:
    addiu $t1,$t1,1
    sw $t1,0($t0)
    sw $t3,4($t0)
    sw $v0,8($t0)
    la $a0,\tag
    jal np_hook_string
    nop
    lw $a0,0($sp)
    jal np_hook_hex
    nop
    la $a0,reader_pointer_prefix
    jal np_hook_string
    nop
    lw $a0,32($sp)
    jal np_hook_hex
    nop
    lw $a0,32($sp)
    li $t0,0x0059f694
    sltu $t0,$a0,$t0
    bnez $t0,18f
    nop
    li $t0,0x005a8dc0-2352
    sltu $t0,$a0,$t0
    beqz $t0,18f
    nop
    li $a1,2352
    jal np_hook_fnv
    nop
    sw $v0,64($sp)
    la $a0,reader_fnv_prefix
    jal np_hook_string
    nop
    lw $a0,64($sp)
    jal np_hook_hex
    nop
18:
    jal np_psx_snapshot
    nop
    la $a0,newline
    jal np_hook_string
    nop
19:
    lq $ra,16($sp)
    lq $v0,32($sp)
    lq $v1,48($sp)
    jr $ra
    addiu $sp,$sp,96
    .endm
    NP_READER np_sector_consumer_hook,0x210368,consumer_prefix,consumer_state
    NP_READER np_sector_request_hook,0x210290,request_prefix,request_state
/* Repeat-test path: retain only the guarded adaptation, with no reader, cache,
 * callback or CPU-state logging. Preserve the original pointer/result. */
    .globl np_sector_consumer_quiet
np_sector_consumer_quiet:
    addiu $sp,$sp,-64
    sq $a0,0($sp)
    sq $ra,16($sp)
    jal 0x210368
    nop
    sq $v0,32($sp)
    sq $v1,48($sp)
    lw $a0,0($sp)
    move $a1,$v0
    jal np_license_adapter
    nop
    lq $ra,16($sp)
    lq $v0,32($sp)
    lq $v1,48($sp)
    jr $ra
    addiu $sp,$sp,64
/* Observe the independently verified sector-cache -> two-slot CD FIFO copy.
 * Delegate first, preserve results, and compare only bounded 2352-byte slots. */
    .globl np_asset_copy_hook
np_asset_copy_hook:
    addiu $sp,$sp,-128
    sq $a0,0($sp)
    sq $a1,16($sp)
    sq $a2,32($sp)
    sq $ra,48($sp)
    jal 0x209e00
    nop
    sq $v0,64($sp)
    sq $v1,80($sp)
    lw $t0,32($sp)
    li $t1,2352
    bne $t0,$t1,26f
    nop
    lw $t0,0($sp)
    li $t1,0x0059c228
    beq $t0,$t1,27f
    nop
    li $t1,0x0059c228+2352
    bne $t0,$t1,26f
    nop
27:
    lw $t0,16($sp)
    li $t1,0x0059f694
    sltu $t1,$t0,$t1
    bnez $t1,26f
    nop
    li $t1,0x005a8dc0-2352
    sltu $t1,$t0,$t1
    beqz $t1,26f
    nop
    li $t0,0x0059c204
    lw $t0,0($t0)
    lw $t1,0($sp)
    li $t2,0x0059c228
    subu $t1,$t1,$t2
    sltu $t1,$zero,$t1
    sll $t1,$t1,2
    la $t2,asset_fifo_lbas
    addu $t1,$t1,$t2
    sw $t0,0($t1)
    sltiu $t1,$t0,37
    bnez $t1,26f
    nop
    li $t1,53875
    sltu $t1,$t0,$t1
    bnez $t1,28f
    nop
    li $t1,54079
    sltu $t1,$t0,$t1
    bnez $t1,26f
    nop
28:
    sw $t0,104($sp)
    la $t0,asset_copy_count
    lw $t1,0($t0)
    sltiu $t2,$t1,64
    beqz $t2,26f
    nop
    addiu $t1,$t1,1
    sw $t1,0($t0)
    lw $a0,16($sp)
    li $a1,2352
    jal np_hook_fnv
    nop
    sw $v0,96($sp)
    lw $a0,0($sp)
    li $a1,2352
    jal np_hook_fnv
    nop
    sw $v0,100($sp)
    la $a0,asset_copy_prefix
    jal np_hook_string
    nop
    lw $a0,104($sp)
    jal np_hook_hex
    nop
    la $a0,asset_source_prefix
    jal np_hook_string
    nop
    lw $a0,96($sp)
    jal np_hook_hex
    nop
    la $a0,asset_dest_prefix
    jal np_hook_string
    nop
    lw $a0,100($sp)
    jal np_hook_hex
    nop
    la $a0,newline
    jal np_hook_string
    nop
26:
    lq $ra,48($sp)
    lq $v0,64($sp)
    lq $v1,80($sp)
    jr $ra
    addiu $sp,$sp,128
/* Observe CD DMA's FIFO -> PS1 RAM copy (verified call 0x20e550).
 * Track each FIFO slot's LBA, and hash only one bounded in-slot slice after
 * delegating. No source, destination, length or native return is changed. */
    .globl np_asset_dma_hook
np_asset_dma_hook:
    addiu $sp,$sp,-128
    sq $a0,0($sp)
    sq $a1,16($sp)
    sq $a2,32($sp)
    sq $ra,48($sp)
    jal 0x209e00
    nop
    sq $v0,64($sp)
    sq $v1,80($sp)
    lw $t0,32($sp)
    addiu $t1,$t0,-1
    sltiu $t1,$t1,2352
    beqz $t1,29f
    nop
    lw $t1,0($sp)
    li $t2,0x40000000
    subu $t1,$t1,$t2
    li $t2,0x200000
    sltu $t3,$t1,$t2
    beqz $t3,29f
    nop
    addu $t1,$t1,$t0
    sltu $t3,$t2,$t1
    bnez $t3,29f
    nop
    lw $t1,16($sp)
    li $t2,0x0059c228
    subu $t1,$t1,$t2
    sltiu $t2,$t1,2352
    bnez $t2,30f
    li $t3,0
    addiu $t1,$t1,-2352
    sltiu $t2,$t1,2352
    beqz $t2,29f
    li $t3,4
30:
    addu $t2,$t1,$t0
    sltiu $t2,$t2,2353
    beqz $t2,29f
    nop
    sw $t1,108($sp)
    la $t2,asset_fifo_lbas
    addu $t2,$t2,$t3
    lw $t0,0($t2)
    sltiu $t1,$t0,37
    bnez $t1,29f
    nop
    li $t1,53875
    sltu $t1,$t0,$t1
    bnez $t1,31f
    nop
    li $t1,54079
    sltu $t1,$t0,$t1
    bnez $t1,29f
    nop
31:
    sw $t0,104($sp)
    la $t0,asset_dma_count
    lw $t1,0($t0)
    sltiu $t2,$t1,64
    beqz $t2,29f
    nop
    addiu $t1,$t1,1
    sw $t1,0($t0)
    lw $a0,16($sp)
    lw $a1,32($sp)
    jal np_hook_fnv
    nop
    sw $v0,96($sp)
    lw $a0,0($sp)
    lw $a1,32($sp)
    jal np_hook_fnv
    nop
    sw $v0,100($sp)
    la $a0,asset_dma_prefix
    jal np_hook_string
    nop
    lw $a0,104($sp)
    jal np_hook_hex
    nop
    la $a0,cache_offset_prefix
    jal np_hook_string
    nop
    lw $a0,108($sp)
    jal np_hook_hex
    nop
    la $a0,asset_bytes_prefix
    jal np_hook_string
    nop
    lw $a0,32($sp)
    jal np_hook_hex
    nop
    la $a0,asset_source_prefix
    jal np_hook_string
    nop
    lw $a0,96($sp)
    jal np_hook_hex
    nop
    la $a0,asset_dest_prefix
    jal np_hook_string
    nop
    lw $a0,100($sp)
    jal np_hook_hex
    nop
    la $a0,newline
    jal np_hook_string
    nop
29:
    lq $ra,48($sp)
    lq $v0,64($sp)
    lq $v1,80($sp)
    jr $ra
    addiu $sp,$sp,128
/* Shared GP0/DMA packet staging: report only new maximum copy sizes.
 * No packet payload or graphics state is changed or exported. */
    .globl np_graphics_copy_hook
np_graphics_copy_hook:
    addiu $sp,$sp,-128
    sq $a0,0($sp)
    sq $a1,16($sp)
    sq $a2,32($sp)
    sq $ra,48($sp)
    jal 0x209e00
    nop
    sq $v0,64($sp)
    sq $v1,80($sp)
    lw $t0,32($sp)
    la $t1,graphics_copy_max
    lw $t2,0($t1)
    sltu $t2,$t2,$t0
    beqz $t2,32f
    nop
    sw $t0,0($t1)
    la $t1,graphics_copy_count
    lw $t2,0($t1)
    sltiu $t3,$t2,64
    beqz $t3,32f
    nop
    addiu $t2,$t2,1
    sw $t2,0($t1)
    la $a0,graphics_copy_prefix
    jal np_hook_string
    nop
    lw $a0,32($sp)
    jal np_hook_hex
    nop
    la $a0,graphics_source_prefix
    jal np_hook_string
    nop
    lw $a0,16($sp)
    jal np_hook_hex
    nop
    la $a0,graphics_dest_prefix
    jal np_hook_string
    nop
    lw $a0,0($sp)
    jal np_hook_hex
    nop
    la $a0,newline
    jal np_hook_string
    nop
32:
    lq $ra,48($sp)
    lq $v0,64($sp)
    lq $v1,80($sp)
    jr $ra
    addiu $sp,$sp,128
/* Original image-gather observations. Preserve every volatile GPR at 128 bits,
 * both full HI/LO registers and SA; no FPU/VU state is touched. */
    .macro NP_GPU_SAVE
    addiu $sp,$sp,-320
    sq $v0,0($sp)
    sq $v1,16($sp)
    sq $a0,32($sp)
    sq $a1,48($sp)
    sq $a2,64($sp)
    sq $a3,80($sp)
    sq $t0,96($sp)
    sq $t1,112($sp)
    sq $t2,128($sp)
    sq $t3,144($sp)
    sq $8,160($sp)
    sq $9,176($sp)
    sq $10,192($sp)
    sq $11,208($sp)
    sq $t8,224($sp)
    sq $t9,240($sp)
    sq $ra,256($sp)
    pmfhi $t0
    sq $t0,272($sp)
    pmflo $t0
    sq $t0,288($sp)
    mfsa $t0
    sw $t0,304($sp)
    .endm
    .macro NP_GPU_RESTORE
    lq $t0,272($sp)
    pmthi $t0
    lq $t0,288($sp)
    pmtlo $t0
    lw $t0,304($sp)
    mtsa $t0
    lq $v0,0($sp)
    lq $v1,16($sp)
    lq $a0,32($sp)
    lq $a1,48($sp)
    lq $a2,64($sp)
    lq $a3,80($sp)
    lq $t0,96($sp)
    lq $t1,112($sp)
    lq $t2,128($sp)
    lq $t3,144($sp)
    lq $8,160($sp)
    lq $9,176($sp)
    lq $10,192($sp)
    lq $11,208($sp)
    lq $t8,224($sp)
    lq $t9,240($sp)
    lq $ra,256($sp)
    addiu $sp,$sp,320
    .endm
    .globl np_graphics_pixels_hook
np_graphics_pixels_hook:
    NP_GPU_SAVE
    la $t0,graphics_upload_count
    lw $t0,0($t0)
    sltiu $t0,$t0,16
    beqz $t0,40f
    nop
    la $t0,graphics_input_active
    lw $t1,0($t0)
    bnez $t1,33f
    nop
    li $t1,1
    sw $t1,0($t0)
    la $t0,graphics_input_valid
    sw $t1,0($t0)
    la $t0,graphics_input_bytes
    sw $zero,0($t0)
    la $t0,graphics_input_fnv
    li $t1,2166136261
    sw $t1,0($t0)
33:
    move $t1,$s5
    lw $t2,80($sp) /* native available word count a3 */
    sltu $t3,$t1,$t2
    bnez $t3,34f
    nop
    move $t1,$t2
34:
    lw $a0,320($sp) /* native source pointer in the original stack frame */
    li $t2,4
    bne $s3,$t2,35f
    nop
    andi $t2,$a0,15
    bnez $t2,35f
    nop
    sltiu $t2,$t1,16
    bnez $t2,35f
    nop
    li $t2,-16 /* aligned fast path consumes complete 16-word groups */
    and $t1,$t1,$t2
35:
    sll $a1,$t1,2
    beqz $a1,39f
    nop
    li $t2,0x100000
    sltu $t3,$t2,$a1
    bnez $t3,39f
    nop
    addu $t1,$a0,$a1
    sltu $t3,$t1,$a0
    bnez $t3,39f
    nop
    li $t2,0x40000000
    sltu $t3,$a0,$t2
    bnez $t3,36f
    nop
    li $t2,0x40200000
    sltu $t3,$t2,$t1
    beqz $t3,37f
    nop
36:
    li $t2,0x700031b0
    sltu $t3,$a0,$t2
    bnez $t3,39f
    nop
    li $t2,0x70003230
    sltu $t3,$t2,$t1
    bnez $t3,39f
    nop
37:
    la $t2,graphics_input_bytes
    lw $t3,0($t2)
    addu $t3,$t3,$a1
    sw $t3,0($t2)
    la $t2,graphics_input_fnv
    lw $v0,0($t2)
    jal np_graphics_fnv_continue
    nop
    la $t2,graphics_input_fnv
    sw $v0,0($t2)
    b 40f
    nop
39:
    la $t0,graphics_input_valid
    sw $zero,0($t0)
40:
    NP_GPU_RESTORE
    jr $ra
    sltu $v0,$s5,$a3 /* exactly the displaced comparison */
/* Already gathered buffer, before native GS submission. Hash only logical
 * upload words (including the PS1 packed-word odd-pixel padding). */
    .globl np_graphics_upload_hook
np_graphics_upload_hook:
    NP_GPU_SAVE
    la $t0,graphics_upload_count
    lw $t1,0($t0)
    sltiu $t2,$t1,16
    beqz $t2,44f
    nop
    addiu $t1,$t1,1
    sw $t1,0($t0)
    sw $t1,308($sp)
    multu $s7,$s6
    mflo $t1
    addiu $t1,$t1,1
    srl $t1,$t1,1
    sll $a1,$t1,2
    sw $a1,312($sp)
    li $t0,0x70003384
    lw $a0,0($t0)
    li $t1,0x20000000
    subu $t1,$a0,$t1
    li $t2,0x00200000
    sltu $t3,$t1,$t2
    bnez $t3,43f
    nop
    addu $t1,$t1,$a1
    li $t2,0x02000000
    sltu $t3,$t2,$t1
    bnez $t3,43f
    nop
    li $v0,2166136261
    jal np_graphics_fnv_continue
    nop
    sw $v0,316($sp)
    b 42f
    nop
43:
    la $t0,graphics_input_valid
    sw $zero,0($t0)
    sw $zero,316($sp)
42:
    la $a0,graphics_upload_prefix
    jal np_hook_string
    nop
    lw $a0,308($sp)
    jal np_hook_hex
    nop
    la $a0,graphics_xy_prefix
    jal np_hook_string
    nop
    li $t0,0x700030f4
    lw $a0,0($t0)
    jal np_hook_hex
    nop
    la $a0,graphics_size_prefix
    jal np_hook_string
    nop
    li $t0,0x700030f8
    lw $a0,0($t0)
    jal np_hook_hex
    nop
    la $a0,asset_bytes_prefix
    jal np_hook_string
    nop
    lw $a0,312($sp)
    jal np_hook_hex
    nop
    la $a0,graphics_input_bytes_prefix
    jal np_hook_string
    nop
    la $t0,graphics_input_bytes
    lw $a0,0($t0)
    jal np_hook_hex
    nop
    la $a0,asset_source_prefix
    jal np_hook_string
    nop
    la $t0,graphics_input_fnv
    lw $a0,0($t0)
    jal np_hook_hex
    nop
    la $a0,asset_dest_prefix
    jal np_hook_string
    nop
    lw $a0,316($sp)
    jal np_hook_hex
    nop
    la $a0,graphics_valid_prefix
    jal np_hook_string
    nop
    la $t0,graphics_input_valid
    lw $a0,0($t0)
    jal np_hook_hex
    nop
    la $a0,newline
    jal np_hook_string
    nop
    la $t0,graphics_input_active
    sw $zero,0($t0)
    lw $t0,308($sp)
    li $t1,16
    bne $t0,$t1,44f
    nop
    la $a0,graphics_cap_prefix
    jal np_hook_string
    nop
44:
    NP_GPU_RESTORE
    /* Tail delegate with the original stack, arguments, SA and HI/LO. */
    j 0x21b680
    nop
/* The original interrupt has already drained its two outgoing FIFO qwords.
 * PCSX2's FIFO interpreter also consumes MADR/QWC for those writes. Restore
 * the existing native descriptor here, then execute the displaced DMA start.
 * No descriptor, buffer, packet, vector state or worker schedule is changed. */
    .globl np_graphics_readback_start
np_graphics_readback_start:
    NP_GPU_SAVE
    lui $t0,0x7000
    ori $t0,$t0,0x3534
    lw $t1,0($t0)
    sll $t1,$t1,3
    addu $t0,$t0,$t1
    lw $t1,4($t0)
    lw $t2,8($t0)
    lui $t3,0x1000
    ori $t3,$t3,0x9020
    sw $t1,0($t3)
    sw $t2,-16($t3)
    sw $a3,-32($t3)
    NP_GPU_RESTORE
    jr $ra
    nop
np_graphics_fnv_continue:
    li $t1,16777619
41:
    lbu $t0,0($a0)
    xor $v0,$v0,$t0
    multu $v0,$t1
    mflo $v0
    addiu $a0,$a0,1
    addiu $a1,$a1,-1
    bnez $a1,41b
    nop
    jr $ra
    nop
/* One explicit diagnostic write to an existing firmware license-string slot.
 * Require LBA 4, the exact source-line fingerprint, its NUL terminator and the
 * intact original destination. Neither native sectors nor disc files change. */
np_license_adapter:
    la $t0,np_firmware_license_enabled
    lw $t0,0($t0)
    beqz $t0,21f
    nop
    li $t0,4
    bne $a0,$t0,21f
    nop
    li $t0,0x0059f694
    sltu $t0,$a1,$t0
    bnez $t0,21f
    nop
    li $t0,0x005a8dc0-2352
    sltu $t0,$a1,$t0
    beqz $t0,21f
    nop
    addiu $sp,$sp,-48
    sq $ra,0($sp)
    addiu $a0,$a1,24
    sw $a0,16($sp)
    lbu $t0,70($a0)
    bnez $t0,23f
    nop
    li $a1,70
    jal np_hook_fnv
    nop
    li $t0,0xbe8443cf
    bne $v0,$t0,23f
    nop
    li $a0,0x4003e9f0 /* native PS1 RAM alias independently verified at 0x201f9c */
    li $a1,71
    jal np_hook_fnv
    nop
    li $t0,0xd0abeb59
    bne $v0,$t0,23f
    nop
    lw $t0,16($sp)
    li $t1,0x4003e9f0
    li $t2,71
22:
    lbu $t3,0($t0)
    sb $t3,0($t1)
    addiu $t0,$t0,1
    addiu $t1,$t1,1
    addiu $t2,$t2,-1
    bnez $t2,22b
    nop
    la $a0,license_adapted
    b 24f
    nop
23:
    la $a0,license_rejected
24:
    la $t0,np_firmware_license_enabled
    sw $zero,0($t0)
    jal np_hook_string
    nop
    lq $ra,0($sp)
    addiu $sp,$sp,48
21:
    jr $ra
    nop
/* Read only the independently verified state pointer, PC and PS1 return
 * address. No instruction bytes or game contents are emitted. */
np_psx_snapshot:
    addiu $sp,$sp,-48
    sq $ra,0($sp)
    li $t0,0x00502710
    lw $t0,0($t0)
    li $t1,0x70000000 /* independently verified at native main's 0x200518 */
    bne $t0,$t1,20f
    nop
    lw $t1,0($t0)
    sw $t1,16($sp)
    lw $t1,0x8c($t0)
    sw $t1,20($sp)
    lw $t1,0x18($t0)
    sw $t1,24($sp)
    la $a0,psx_pc_prefix
    jal np_hook_string
    nop
    lw $a0,16($sp)
    jal np_hook_hex
    nop
    la $a0,psx_ra_prefix
    jal np_hook_string
    nop
    lw $a0,20($sp)
    jal np_hook_hex
    nop
    la $a0,psx_v0_prefix
    jal np_hook_string
    nop
    lw $a0,24($sp)
    jal np_hook_hex
    nop
20:
    lq $ra,0($sp)
    jr $ra
    addiu $sp,$sp,48
/* Read-only cache snapshot at RPC boundaries. Bounds are the verified native
 * 0x9740-byte response buffer; fingerprints never export game payload bytes. */
np_cache_snapshot:
    addiu $sp,$sp,-64
    sq $ra,0($sp)
    li $t0,0x0059f680
    lw $a0,0($t0)
    sw $a0,16($sp)
    lw $a1,4($t0)
    sw $a1,20($sp)
    lw $t1,16($t0)
    sw $t1,24($sp)
    la $a0,cache_prefix
    jal np_hook_string
    nop
    lw $a0,16($sp)
    jal np_hook_hex
    nop
    la $a0,rpc_count_prefix
    jal np_hook_string
    nop
    lw $a0,20($sp)
    jal np_hook_hex
    nop
    la $a0,cache_offset_prefix
    jal np_hook_string
    nop
    lw $a0,24($sp)
    jal np_hook_hex
    nop
    lw $t0,20($sp)
    addiu $t1,$t0,-1
    sltiu $t1,$t1,16
    beqz $t1,14f
    nop
    lw $t1,24($sp)
    sltiu $t2,$t1,0x431
    beqz $t2,14f
    nop
    li $a0,0x0059f694
    addu $a0,$a0,$t1
    li $a1,2352
    multu $t0,$a1
    mflo $a1
    jal np_hook_fnv
    nop
    sw $v0,28($sp)
    la $a0,batch_fnv_prefix
    jal np_hook_string
    nop
    lw $a0,28($sp)
    jal np_hook_hex
    nop
14:
    la $a0,newline
    jal np_hook_string
    nop
    lq $ra,0($sp)
    jr $ra
    addiu $sp,$sp,64
    .globl np_hooks_code_end
np_hooks_code_end:
native_failed: .asciz "PSXCORE native=initialization-failed no-save-fallback=yes stopped=yes\n"
cards_ready: .asciz "PSXCORE native=cards-ready slots=2 validated=yes\n"
save_failed: .asciz "PSXCORE save=failed stage="
save_stopped: .asciz " no-save-fallback=yes stopped=yes\n"
save_flushed: .asciz "PSXCORE save=flush-completed\n"
np_save_sync_mount: .asciz "pfs1:"
mount_prefix: .asciz "PSXCORE native=mount target="
source_prefix: .asciz " source="
disc_prefix: .asciz "PSXCORE native=disc path="
card_prefix: .asciz "PSXCORE native=cards"
enter_suffix: .asciz " enter\n"
post_0: .asciz "PSXCORE startup=22d2e0"
post_1: .asciz "PSXCORE startup=22d480"
post_2: .asciz "PSXCORE startup=22dc30"
post_3: .asciz "PSXCORE startup=214418"
post_4: .asciz "PSXCORE startup=20c750"
post_5: .asciz "PSXCORE startup=200940"
post_6: .asciz "PSXCORE startup=200d10"
core_enter: .asciz "PSXCORE core=iteration-enter\n"
core_return: .asciz "PSXCORE core=iteration-return\n"
rpc_prefix: .asciz "PSXCORE native=read-request lba="
rpc_count_prefix: .asciz " sectors="
rpc_mode_prefix: .asciz " async="
cache_prefix: .asciz "PSXCORE native=cache lba="
cache_offset_prefix: .asciz " offset="
batch_fnv_prefix: .asciz " batch-fnv1a="
rpc_return_prefix: .asciz "PSXCORE native=read-rpc result="
completion_prefix: .asciz "PSXCORE native=async-callback signal-result="
consumer_prefix: .asciz "PSXCORE native=consumer lba="
asset_copy_prefix: .asciz "PSXCORE native=asset-copy lba="
asset_dma_prefix: .asciz "PSXCORE native=asset-dma lba="
asset_bytes_prefix: .asciz " bytes="
graphics_copy_prefix: .asciz "PSXCORE graphics=packet-copy-max bytes="
graphics_source_prefix: .asciz " source="
graphics_dest_prefix: .asciz " destination="
graphics_upload_prefix: .asciz "PSXCORE graphics=upload-gather sequence="
graphics_xy_prefix: .asciz " xy="
graphics_size_prefix: .asciz " size="
graphics_input_bytes_prefix: .asciz " input-bytes="
graphics_valid_prefix: .asciz " bounds-valid="
graphics_cap_prefix: .asciz "PSXCORE graphics=upload-budget-exhausted samples=00000010 coverage=bounded\n"
asset_source_prefix: .asciz " source-fnv1a="
asset_dest_prefix: .asciz " destination-fnv1a="
request_prefix: .asciz "PSXCORE native=cache-request lba="
reader_pointer_prefix: .asciz " result="
reader_fnv_prefix: .asciz " sector-fnv1a="
psx_pc_prefix: .asciz " psx-pc="
psx_ra_prefix: .asciz " psx-ra="
psx_v0_prefix: .asciz " psx-v0="
license_adapted: .asciz "PSXCORE compatibility=firmware-license-adapted guards=passed bytes=00000047\n"
license_rejected: .asciz "PSXCORE compatibility=firmware-license-rejected data=unchanged\n"
result_prefix: .asciz " result="
newline: .asciz "\n"
hex_digits: .asciz "0123456789abcdef"
toc_prefix: .asciz "PSXCORE native=toc bytes=00000400 fnv1a="
sector_prefix: .asciz "PSXCORE native=sector lba="
game_sector_prefix: .asciz "PSXCORE native=game-sector lba="
fnv_prefix: .asciz " bytes=00000930 fnv1a="
game_probe_complete: .asciz "PSXCORE native=game-probe-complete before-saves=yes\n"
errors_prefix: .asciz " bytes=00000930 errors="
probe_pass: .asciz "PSXCORE native=probe-complete toc=passed sectors=passed before-saves=yes\n"
probe_fail: .asciz "PSXCORE native=probe-failed before-saves=yes\n"
    .balign 16
probe_lbas: .word 0,1,17
game_probe_lbas: .word 0,4,16,20,22,23,53875
    .globl np_disc_probe_enabled
np_disc_probe_enabled: .word 0
    .globl np_firmware_license_enabled
np_firmware_license_enabled: .word 0
    .globl np_asset_trace_enabled
np_asset_trace_enabled: .word 0
    .globl np_native_request_enabled
np_native_request_enabled: .word 0
    .globl np_trace_enabled
np_trace_enabled: .word 1
np_save_ready: .word 0
    .globl np_save_partition
np_save_partition: .space 40
    .globl np_save_error_stage
np_save_error_stage: .word 0
    .globl np_save_error_result
np_save_error_result: .word 0
    .globl np_save_flush_count
np_save_flush_count: .word 0
core_count: .word 0
rpc_count: .word 0
completion_count: .word 0
asset_copy_count: .word 0
asset_dma_count: .word 0
asset_fifo_lbas: .word 0,0
graphics_copy_max: .word 0
graphics_copy_count: .word 0
graphics_upload_count: .word 0
graphics_input_active: .word 0
graphics_input_valid: .word 0
graphics_input_bytes: .word 0
graphics_input_fnv: .word 0
consumer_state: .word 0,-1,-1
request_state: .word 0,-1,-1
    .balign 16
    .globl np_hooks_end
np_hooks_end:
