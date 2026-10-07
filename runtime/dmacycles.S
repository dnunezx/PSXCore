/* Shared POPS DMA accounting: keep raster cost in the native worker budget, but
 * exclude it from linked GPU DMA completion accounting. All entries execute
 * inside the native interrupt-masked counter updates. No title RAM is read. */
    .set noreorder
    .set noat
    .balign 16
    .macro DMA_RASTER name,cost,result,base,tail
    .globl \name
\name:
    addiu $sp,$sp,-64
    sq $8,0($sp)
    sq $9,16($sp)
    sq $10,32($sp)
    sq $11,48($sp)
    move $8,\cost
    la $9,1f
    b np_dma_collect_raster
    nop
1:
    lq $8,0($sp)
    lq $9,16($sp)
    lq $10,32($sp)
    lq $11,48($sp)
    addiu $sp,$sp,64
    addu \result,\result,\cost
    sw \result,0x2260(\base)
    j \tail
    nop
    .endm
    DMA_RASTER np_dma_fill_cost,$v0,$a0,$a1,0x21d5a0
    DMA_RASTER np_dma_sprite_cost,$v1,$v0,$a0,0x21eb3c
    DMA_RASTER np_dma_polygon_cost,$v1,$v0,$a0,0x21f9ec
    DMA_RASTER np_dma_line_cost,$v1,$v0,$a0,0x22065c
    DMA_RASTER np_dma_copy_cost,$v1,$v0,$a0,0x225f1c
np_dma_collect_raster:
    lui $10,0x0050
    lw $10,0x6c30($10)
    li $11,0x01000601
    and $10,$10,$11
    li $11,0x01000401
    bne $10,$11,2f
    nop
    la $10,np_dma_raster_cost
    lw $11,0($10)
    addu $11,$11,$8
    sw $11,0($10)
2:
    jr $9
    nop

    .macro DMA_RESET name,base,tail
    .globl \name
\name:
    addiu $sp,$sp,-16
    sq $8,0($sp)
    la $8,np_dma_raster_cost
    sw $zero,0($8)
    lq $8,0($sp)
    addiu $sp,$sp,16
    sw $zero,0x2260(\base)
    j \tail
    nop
    .endm
    /* The original following quota store stays in the redirect delay slot. */
    DMA_RESET np_dma_reset_linked,$v0,0x21ae48
    DMA_RESET np_dma_reset_block,$v1,0x21ae70

    .globl np_dma_harvest_cost
np_dma_harvest_cost:
    /* Native v1 already includes the full word+raster budget deduction. */
    sw $v1,0x226c($a1)
    addiu $sp,$sp,-32
    sq $8,0($sp)
    sq $9,16($sp)
    la $8,np_dma_raster_cost
    lw $9,0($8)
    sw $zero,0($8)
    sltu $8,$v0,$9
    bnez $8,3f
    nop
    subu $v0,$v0,$9
3:
    lq $8,0($sp)
    lq $9,16($sp)
    addiu $sp,$sp,32
    jr $ra
    ei
    .balign 16
    .globl np_dma_raster_cost
np_dma_raster_cost:
    .word 0
