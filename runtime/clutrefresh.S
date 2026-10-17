/* Refresh an indexed texture's CLUT before publishing the native TEX0 pair.
 * A valid temporary CSM1/PSMCT32 load precedes the original CSM2/PSMCT16
 * load. No vertex is emitted between them. Keep the native palette address,
 * texture, colors, draw order and final state; direct-color paths pass through.
 * The native GIF/DMA finalizers derive their lengths from the advanced cursor.
 */
    .set noreorder
    .set noat
    .section ._psxcore_clut_refresh,"ax",@progbits
    .balign 16
    .globl np_clut_refresh_start,np_clut_refresh_end
    .globl np_clut_refresh_polygon,np_clut_refresh_sprite
np_clut_refresh_start:
    .macro NP_CLUT_REFRESH tex,tmp
    dsrl \tmp,\tex,20
    andi \tmp,\tmp,63
    addiu \tmp,\tmp,-19
    sltiu \tmp,\tmp,2
    beqz \tmp,9f
    nop
    /* CPSM=PSMCT16, CSM=CSM2. Other palette representations stay native. */
    dsrl32 \tmp,\tex,19
    andi \tmp,\tmp,31
    xori \tmp,\tmp,18
    bnez \tmp,9f
    nop
    dsrl32 \tmp,\tex,29
    xori \tmp,\tmp,1
    bnez \tmp,9f
    nop
    li \tmp,9
    dsll32 \tmp,\tmp,20
    xor \tmp,\tex,\tmp
    sd \tmp,0($v0)
    li \tmp,6
    sd \tmp,8($v0)
    addiu $v0,$v0,16
9:
    .endm
np_clut_refresh_polygon:
    /* v1 is replaced by the displaced cursor calculation. */
    NP_CLUT_REFRESH $a3,$v1
    addiu $v1,$v0,32
    j 0x21ec20
    li $a0,7
np_clut_refresh_sprite:
    /* a2 is replaced by the displaced cursor calculation. */
    NP_CLUT_REFRESH $11,$a2
    addiu $a2,$v0,48
    j 0x21fb14
    dsrl32 $v1,$v1,0
np_clut_refresh_end:
