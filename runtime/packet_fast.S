.section ._packet_fast.text,"ax",@progbits
.set noreorder
.set noat
.balign 16
.globl np_packet_fast,np_packet_fast_end,np_packet_fast_count,np_packet_fast_template
np_packet_fast:
    addiu $9,$20,0x330c
    lw $2,0($9)
    andi $3,$2,15
    bnez $3,packet_native
    nop
    /* No original live vertex temporary is overwritten before its store. */
    move $11,$2
    lw $8,12($9)
    subu $2,$11,$8
    addiu $2,$2,-4
    srl $2,$2,4
    lw $3,0($8)
    addu $3,$3,$2
    sw $3,0($8)
    addiu $10,$11,160
    sw $10,0($9)
    sw $zero,12($9)
    move $16,$9
    sq $15,80($11)
    sq $13,96($11)
    sq $12,112($11)
    sq $18,128($11)
    /* s2 is restored at full width by the original epilogue. */
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
    /* Rebuild the exact native PRIM and register list from final flags. */
    srl $5,$21,8
    andi $5,$5,1
    sll $4,$5,3
    srl $7,$21,5
    andi $7,$7,1
    sll $2,$7,4
    or $4,$4,$2
    sll $2,$21,2
    andi $2,$2,64
    or $4,$4,$2
    ori $4,$4,0x104
    dsll32 $4,$4,15
    ld $3,16($6)
    or $4,$4,$3
    srl $2,$21,12
    andi $2,$2,1
    dsubu $4,$4,$2
    sd $4,16($11)
    addiu $3,$zero,0x510
    addiu $2,$zero,0x5f0
    movz $3,$2,$5
    addiu $2,$zero,3
    addiu $5,$zero,15
    movz $2,$5,$7
    or $3,$3,$2
    sd $3,24($11)
    lui $2,%hi(np_packet_fast_count)
    lw $3,%lo(np_packet_fast_count)($2)
    addiu $3,$3,1
    sw $3,%lo(np_packet_fast_count)($2)
    /* Aligned base +160 has the native next-header target at end+12. */
    addiu $9,$10,12
    j 0x21f7d4
    addiu $8,$zero,1
packet_native:
    srl $17,$21,8
    j 0x21f638
    addiu $9,$20,0x330c
.balign 16
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
