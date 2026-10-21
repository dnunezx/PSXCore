/* Construct the pinned POPS base polygon packet with aligned constant copies.
 * Preserve geometry, GS/VIF commands, costs and the native chain finalizer.
 * Runs after palette refresh, bounds publication and all graphics-state work.
 */
#ifndef NP_GPU_PACKET_FASTPATH
#define NP_GPU_PACKET_FASTPATH 1
#endif
#if NP_GPU_PACKET_FASTPATH != 0 && NP_GPU_PACKET_FASTPATH != 1
#error "Polygon packet fastpath accepts a boolean build flag only"
#endif
#if NP_GPU_PACKET_FASTPATH
    .set noreorder
    .set noat
    .section ._packet_fast.text,"ax",@progbits
    .balign 16
    .globl np_packet_fast,np_packet_fast_end,np_packet_fast_template,np_packet_fast_count
np_packet_fast:
    addiu $9,$20,0x330c
    lw $2,0($9)
    andi $3,$2,15
    /* Native MSCAL construction uses t8 as its relocation base. Only the
     * checked zero-base entry is suitable for the constant command below. */
    or $3,$3,$24
    bnez $3,packet_native
    nop
    move $11,$2
    lw $8,12($9)
    subu $2,$11,$8
    addiu $2,$2,-4
    sra $2,$2,2
    srl $2,$2,2
    lw $3,0($8)
    addu $3,$3,$2
    sw $3,0($8)
    addiu $10,$11,160
    sw $10,0($9)
    sw $zero,12($9)
    move $16,$9
    /* Save live vertices before using s2 for the constant vectors. The native
     * epilogue restores s2 at full width; no HI/LO, SA or coprocessor use. */
    sq $15,80($11)
    sq $13,96($11)
    sq $12,112($11)
    sq $18,128($11)
    lui $6,%hi(np_packet_fast_template)
    addiu $6,$6,%lo(np_packet_fast_template)
    lq $18,0($6)
    sq $18,0($11)
    lq $18,32($6)
    sq $18,32($11)
    lq $18,64($6)
    sq $18,64($11)
    lq $18,144($6)
    sq $18,144($11)
    ld $2,8($sp)
    sd $2,40($11)
    ld $2,16($sp)
    sd $2,48($11)
    ld $2,24($sp)
    sd $2,56($11)
    ld $2,32($sp)
    sd $2,64($11)
    /* Only flags 4, 5, 8 and 12 affect this GIF tag/register list. Pack
     * those bits into a bounded index and copy the exact native format. */
    srl $4,$21,4
    andi $4,$4,3
    srl $5,$21,6
    andi $5,$5,4
    or $4,$4,$5
    srl $5,$21,9
    andi $5,$5,8
    or $4,$4,$5
    sll $4,$4,4
    lui $5,%hi(packet_formats)
    addu $4,$4,$5
    lq $18,%lo(packet_formats)($4)
    sq $18,16($11)
    lui $2,%hi(np_packet_fast_count)
    lw $3,%lo(np_packet_fast_count)($2)
    addiu $3,$3,1
    sw $3,%lo(np_packet_fast_count)($2)
    /* Aligned packet end has its next native header at end+12. */
    addiu $9,$10,12
    j 0x21f7d4
    addiu $8,$zero,1
packet_native:
    srl $17,$21,8
    j 0x21f638
    addiu $9,$20,0x330c
    .balign 16
    /* Index order: blending, texture, Gouraud shading, triangle. */
    .macro packet_format blend,texture,shade,triangle
    .dword (0x3000400000008004 | ((0x104 | (\shade << 3) | (\texture << 4) | (\blend << 6)) << 47)) - \triangle
    .dword (0x5f0 - (\shade * 0xe0)) | (15 - (\texture * 12))
    .endm
packet_formats:
    packet_format 0,0,0,0
    packet_format 1,0,0,0
    packet_format 0,1,0,0
    packet_format 1,1,0,0
    packet_format 0,0,1,0
    packet_format 1,0,1,0
    packet_format 0,1,1,0
    packet_format 1,1,1,0
    packet_format 0,0,0,1
    packet_format 1,0,0,1
    packet_format 0,1,0,1
    packet_format 1,1,0,1
    packet_format 0,0,1,1
    packet_format 1,0,1,1
    packet_format 0,1,1,1
    packet_format 1,1,1,1
    .purgem packet_format
np_packet_fast_template:
    .word 0,0x11000000,0x01000101,0x6c010000
    .dword 0x3000400000008004,0
    .word 0x01000203,0x6e084001,0,0
    .dword 0,0
    .dword 0
    .word 0x01000103,0x6c040003
    .space 64
    .word 0x15000000,0x11000000,0,0
np_packet_fast_count:
    .word 0
np_packet_fast_end:
#endif
