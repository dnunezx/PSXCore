/* Isolated checksum-pinned native GTE corrections. No C arithmetic,
 * stack frames, shared mode variables or added runtime mode tests.
 * Retained instructions are copied verbatim from the pinned POPS;
 * branch labels below relocate only the dedicated lm=1 variants. */
.set noreorder
.set noat
.section ._psxcore_gte_native_three,"ax",@progbits
.balign 16
.globl np_gte_native_three_begin
np_gte_native_three_begin:
.globl np_gte_compile_rtps
np_gte_compile_rtps:
    j np_gte_select_rtps
    nop
    bnez $v0,1f
    addiu $a0,$a0,%lo(np_gte_rtps_lm1)
    li $a0,0x2088c8
1:  j 0x206c5c
    nop
.globl np_gte_compile_rtpt
np_gte_compile_rtpt:
    j np_gte_select_rtpt
    nop
    bnez $v0,1f
    addiu $a0,$a0,%lo(np_gte_rtpt_lm1)
    li $a0,0x208b68
1:  j 0x206c5c
    nop
.globl np_gte_compile_gpf
np_gte_compile_gpf:
    andi $v0,$s0,0x400
    lui $a0,%hi(np_gte_gpf_lm1)
    bnez $v0,1f
    addiu $a0,$a0,%lo(np_gte_gpf_lm1)
    li $a0,0x208b80
1:  j 0x206c5c
    nop
.globl np_gte_compile_gpl
np_gte_compile_gpl:
    andi $v0,$s0,0x400
    lui $a0,%hi(np_gte_gpl_lm1)
    bnez $v0,1f
    addiu $a0,$a0,%lo(np_gte_gpl_lm1)
    li $a0,0x208b88
1:  j 0x206c5c
    nop
.globl np_gte_compile_intpl
np_gte_compile_intpl:
    j np_gte_select_intpl
    nop
    srl $v1,$s0,19
    andi $v1,$v1,1
    beqz $v1,1f
    nop
    la $a0,np_gte_intpl_lm0
    j 0x206c5c
    nop
1:  la $a0,np_gte_intpl_lm1
    j 0x206c5c
    nop
.globl np_gte_avsz_tail,np_gte_avsz_tail_end
np_gte_avsz_tail:
    mult $8,$8,$25
    ori $v0,$zero,0xffff
    lui $v1,0x8000
    pmfhl.lw $9
    slt $10,$8,$9
    slt $11,$9,$8
    sll $10,$10,16
    sll $11,$11,15
    or $at,$10,$11
    sw $9,96($a0)
    dsra $8,$9,12
    slt $10,$8,$zero
    slt $11,$v0,$8
    movn $8,$zero,$10
    movn $8,$v0,$11
    or $10,$10,$11
    sll $10,$10,18
    or $at,$at,$10
    sh $8,28($a0)
    movz $v1,$zero,$at
    or $at,$at,$v1
    jr $ra
    sw $at,252($a0)
np_gte_avsz_tail_end:
.globl np_gte_gpf_lm1,np_gte_gpl_lm1
np_gte_gpf_lm1:
gp_208b80:
.word 0x10000000 | (((gp_208ba0 - . - 4) / 4) & 0xffff)
gp_208b84:
.word 0x700054a9 /* retained 0x208b84 */
gp_208b88:
.globl np_gte_gpl_lm1
np_gte_gpl_lm1:
.word 0x8c8a0064
gp_208b8c:
.word 0x8c820068 /* retained 0x208b8c */
gp_208b90:
.word 0x8c83006c /* retained 0x208b90 */
gp_208b94:
.word 0x704a5488 /* retained 0x208b94 */
gp_208b98:
.word 0x706a5389 /* retained 0x208b98 */
gp_208b9c:
.word 0x00000000 /* retained 0x208b9c */
gp_208ba0:
.word 0x8c880020 /* retained 0x208ba0 */
gp_208ba4:
.word 0x70000ca9 /* retained 0x208ba4 */
gp_208ba8:
.word 0x71084488 /* retained 0x208ba8 */
gp_208bac:
.word 0x8c890024 /* retained 0x208bac */
gp_208bb0:
.word 0x8c820028 /* retained 0x208bb0 */
gp_208bb4:
.word 0x8c83002c /* retained 0x208bb4 */
gp_208bb8:
.word 0x70494c88 /* retained 0x208bb8 */
gp_208bbc:
.word 0x70694b89 /* retained 0x208bbc */
gp_208bc0:
.word 0x71084488 /* retained 0x208bc0 */
gp_208bc4:
.word 0x8c820054 /* retained 0x208bc4 */
gp_208bc8:
.word 0x8c830058 /* retained 0x208bc8 */
gp_208bcc:
.word 0xac820050 /* retained 0x208bcc */
gp_208bd0:
.word 0xac830054 /* retained 0x208bd0 */
gp_208bd4:
.word 0x71094709 /* retained 0x208bd4 */
gp_208bd8:
.word 0x3c027000 /* retained 0x208bd8 */
gp_208bdc:
pxor $14,$14,$14
gp_208be0:
.word 0x784f0320 /* retained 0x208be0 */
gp_208be4:
.word 0x78580300 /* retained 0x208be4 */
gp_208be8:
.word 0x54a00000 | (((gp_208bf0 - . - 4) / 4) & 0xffff)
gp_208bec:
.word 0x7008433f /* retained 0x208bec */
gp_208bf0:
.word 0x710a4008 /* retained 0x208bf0 */
gp_208bf4:
.word 0x0008103f /* retained 0x208bf4 */
gp_208bf8:
.word 0xac880064 /* retained 0x208bf8 */
gp_208bfc:
.word 0x71081ba9 /* retained 0x208bfc */
gp_208c00:
.word 0xac820068 /* retained 0x208c00 */
gp_208c04:
.word 0xac83006c /* retained 0x208c04 */
gp_208c08:
.word 0x71c81088 /* retained 0x208c08 */
gp_208c0c:
.word 0x71c818c8 /* retained 0x208c0c */
gp_208c10:
.word 0x70585c89 /* retained 0x208c10 */
gp_208c14:
.word 0x706f1088 /* retained 0x208c14 */
gp_208c18:
.word 0x702b0ca9 /* retained 0x208c18 */
gp_208c1c:
.word 0x70585c89 /* retained 0x208c1c */
gp_208c20:
.word 0x706f48e8 /* retained 0x208c20 */
gp_208c24:
.word 0x702b0ca9 /* retained 0x208c24 */
gp_208c28:
.word 0x0009103f /* retained 0x208c28 */
gp_208c2c:
.word 0xac890024 /* retained 0x208c2c */
gp_208c30:
.word 0x71291ba9 /* retained 0x208c30 */
gp_208c34:
.word 0xac820028 /* retained 0x208c34 */
gp_208c38:
.word 0xac83002c /* retained 0x208c38 */
gp_208c3c:
.word 0x7008413f /* retained 0x208c3c */
gp_208c40:
.word 0x3c027000 /* retained 0x208c40 */
gp_208c44:
.word 0x78430330 /* retained 0x208c44 */
gp_208c48:
.word 0x78420340 /* retained 0x208c48 */
gp_208c4c:
.word 0x70085888 /* retained 0x208c4c */
gp_208c50:
.word 0x700860c8 /* retained 0x208c50 */
gp_208c54:
.word 0x71636c89 /* retained 0x208c54 */
gp_208c58:
.word 0x71825888 /* retained 0x208c58 */
gp_208c5c:
.word 0x702d0ca9 /* retained 0x208c5c */
gp_208c60:
.word 0x71636c89 /* retained 0x208c60 */
gp_208c64:
.word 0x718248e8 /* retained 0x208c64 */
gp_208c68:
.word 0x702d0ca9 /* retained 0x208c68 */
gp_208c6c:
.word 0x808b001b /* retained 0x208c6c */
gp_208c70:
.word 0x0009103f /* retained 0x208c70 */
gp_208c74:
.word 0xa0890058 /* retained 0x208c74 */
gp_208c78:
.word 0x71291ba9 /* retained 0x208c78 */
gp_208c7c:
.word 0xa0820059 /* retained 0x208c7c */
gp_208c80:
.word 0xa083005a /* retained 0x208c80 */
gp_208c84:
.word 0xa08b005b /* retained 0x208c84 */
gp_208c88:
.word 0x0001103e /* retained 0x208c88 */
gp_208c8c:
.word 0x70211ca8 /* retained 0x208c8c */
gp_208c90:
.word 0x00220825 /* retained 0x208c90 */
gp_208c94:
.word 0x00230825 /* retained 0x208c94 */
gp_208c98:
.word 0x3c027f87 /* retained 0x208c98 */
gp_208c9c:
.word 0x3442e000 /* retained 0x208c9c */
gp_208ca0:
.word 0x3c038000 /* retained 0x208ca0 */
gp_208ca4:
.word 0x00411024 /* retained 0x208ca4 */
gp_208ca8:
.word 0x0002180a /* retained 0x208ca8 */
gp_208cac:
.word 0x00230825 /* retained 0x208cac */
gp_208cb0:
.word 0x03e00008 /* retained 0x208cb0 */
gp_208cb4:
.word 0xac8100fc /* retained 0x208cb4 */
.globl np_gte_intpl_lm0
np_gte_intpl_lm0:
int0_209770:
.word 0x240b1000 /* retained 0x209770 */
int0_209774:
.word 0x240c1000 /* retained 0x209774 */
int0_209778:
.word 0x240d1000 /* retained 0x209778 */
int0_20977c:
.word 0x8c880024 /* retained 0x20977c */
int0_209780:
.word 0x8c890028 /* retained 0x209780 */
int0_209784:
.word 0x8c8a002c /* retained 0x209784 */
int0_209788:
.word 0x010b4018 /* retained 0x209788 */
int0_20978c:
.word 0x012c4818 /* retained 0x20978c */
int0_209790:
.word 0x714d5018 /* retained 0x209790 */
int0_209794:
pxor $at,$at,$at
int0_209798:
.word 0x8c820054 /* retained 0x209798 */
int0_20979c:
.word 0x8c830058 /* retained 0x20979c */
int0_2097a0:
.word 0xac820050 /* retained 0x2097a0 */
int0_2097a4:
.word 0xac830054 /* retained 0x2097a4 */
int0_2097a8:
.word 0x8c8b00d4 /* retained 0x2097a8 */
int0_2097ac:
.word 0x8c8c00d8 /* retained 0x2097ac */
int0_2097b0:
.word 0x8c8d00dc /* retained 0x2097b0 */
int0_2097b4:
.word 0x000b5b38 /* retained 0x2097b4 */
int0_2097b8:
.word 0x000c6338 /* retained 0x2097b8 */
int0_2097bc:
.word 0x000d6b38 /* retained 0x2097bc */
int0_2097c0:
.word 0x0168582f /* retained 0x2097c0 */
int0_2097c4:
.word 0x0189602f /* retained 0x2097c4 */
int0_2097c8:
.word 0x01aa682f /* retained 0x2097c8 */
int0_2097cc:
.word 0x000b5b3b /* retained 0x2097cc */
int0_2097d0:
.word 0x000c633b /* retained 0x2097d0 */
int0_2097d4:
.word 0x000d6b3b /* retained 0x2097d4 */
int0_2097d8:
.word 0x24028000 /* retained 0x2097d8 */
int0_2097dc:
.word 0x24037fff /* retained 0x2097dc */
int0_2097e0:
.word 0x0162702a /* retained 0x2097e0 */
int0_2097e4:
.word 0x006b782a /* retained 0x2097e4 */
int0_2097e8:
.word 0x004e580b /* retained 0x2097e8 */
int0_2097ec:
.word 0x006f580b /* retained 0x2097ec */
int0_2097f0:
.word 0x01cf7025 /* retained 0x2097f0 */
int0_2097f4:
.word 0x000e7600 /* retained 0x2097f4 */
int0_2097f8:
.word 0x002e0825 /* retained 0x2097f8 */
int0_2097fc:
.word 0x0182702a /* retained 0x2097fc */
int0_209800:
.word 0x006c782a /* retained 0x209800 */
int0_209804:
.word 0x004e600b /* retained 0x209804 */
int0_209808:
.word 0x006f600b /* retained 0x209808 */
int0_20980c:
.word 0x01cf7025 /* retained 0x20980c */
int0_209810:
.word 0x000e75c0 /* retained 0x209810 */
int0_209814:
.word 0x002e0825 /* retained 0x209814 */
int0_209818:
.word 0x01a2702a /* retained 0x209818 */
int0_20981c:
.word 0x006d782a /* retained 0x20981c */
int0_209820:
.word 0x004e680b /* retained 0x209820 */
int0_209824:
.word 0x006f680b /* retained 0x209824 */
int0_209828:
.word 0x01cf7025 /* retained 0x209828 */
int0_20982c:
.word 0x000e7580 /* retained 0x20982c */
int0_209830:
.word 0x002e0825 /* retained 0x209830 */
int0_209834:
.word 0x8c8e0020 /* retained 0x209834 */
int0_209838:
.word 0x016e5818 /* retained 0x209838 */
int0_20983c:
.word 0x018e6018 /* retained 0x20983c */
int0_209840:
.word 0x71ae6818 /* retained 0x209840 */
int0_209844:
.word 0x010b402d /* retained 0x209844 */
int0_209848:
.word 0x012c482d /* retained 0x209848 */
int0_20984c:
.word 0x014d502d /* retained 0x20984c */
int0_209850:
.word 0x0008433b /* retained 0x209850 */
int0_209854:
.word 0x00094b3b /* retained 0x209854 */
int0_209858:
.word 0x000a533b /* retained 0x209858 */
pextlw $8,$9,$8
    pcpyld $8,$10,$8
    lui $v0,0x7000
lq $14,784($v0)
lq $15,800($v0)
lq $24,768($v0)
pub0_208bf4:
.word 0x0008103f /* retained 0x208bf4 */
pub0_208bf8:
.word 0xac880064 /* retained 0x208bf8 */
pub0_208bfc:
.word 0x71081ba9 /* retained 0x208bfc */
pub0_208c00:
.word 0xac820068 /* retained 0x208c00 */
pub0_208c04:
.word 0xac83006c /* retained 0x208c04 */
pub0_208c08:
.word 0x71c81088 /* retained 0x208c08 */
pub0_208c0c:
.word 0x71c818c8 /* retained 0x208c0c */
pub0_208c10:
.word 0x70585c89 /* retained 0x208c10 */
pub0_208c14:
.word 0x706f1088 /* retained 0x208c14 */
pub0_208c18:
.word 0x702b0ca9 /* retained 0x208c18 */
pub0_208c1c:
.word 0x70585c89 /* retained 0x208c1c */
pub0_208c20:
.word 0x706f48e8 /* retained 0x208c20 */
pub0_208c24:
.word 0x702b0ca9 /* retained 0x208c24 */
pub0_208c28:
.word 0x0009103f /* retained 0x208c28 */
pub0_208c2c:
.word 0xac890024 /* retained 0x208c2c */
pub0_208c30:
.word 0x71291ba9 /* retained 0x208c30 */
pub0_208c34:
.word 0xac820028 /* retained 0x208c34 */
pub0_208c38:
.word 0xac83002c /* retained 0x208c38 */
pub0_208c3c:
.word 0x7008413f /* retained 0x208c3c */
pub0_208c40:
.word 0x3c027000 /* retained 0x208c40 */
pub0_208c44:
.word 0x78430330 /* retained 0x208c44 */
pub0_208c48:
.word 0x78420340 /* retained 0x208c48 */
pub0_208c4c:
.word 0x70085888 /* retained 0x208c4c */
pub0_208c50:
.word 0x700860c8 /* retained 0x208c50 */
pub0_208c54:
.word 0x71636c89 /* retained 0x208c54 */
pub0_208c58:
.word 0x71825888 /* retained 0x208c58 */
pub0_208c5c:
.word 0x702d0ca9 /* retained 0x208c5c */
pub0_208c60:
.word 0x71636c89 /* retained 0x208c60 */
pub0_208c64:
.word 0x718248e8 /* retained 0x208c64 */
pub0_208c68:
.word 0x702d0ca9 /* retained 0x208c68 */
pub0_208c6c:
.word 0x808b001b /* retained 0x208c6c */
pub0_208c70:
.word 0x0009103f /* retained 0x208c70 */
pub0_208c74:
.word 0xa0890058 /* retained 0x208c74 */
pub0_208c78:
.word 0x71291ba9 /* retained 0x208c78 */
pub0_208c7c:
.word 0xa0820059 /* retained 0x208c7c */
pub0_208c80:
.word 0xa083005a /* retained 0x208c80 */
pub0_208c84:
.word 0xa08b005b /* retained 0x208c84 */
pub0_208c88:
.word 0x0001103e /* retained 0x208c88 */
pub0_208c8c:
.word 0x70211ca8 /* retained 0x208c8c */
pub0_208c90:
.word 0x00220825 /* retained 0x208c90 */
pub0_208c94:
.word 0x00230825 /* retained 0x208c94 */
pub0_208c98:
.word 0x3c027f87 /* retained 0x208c98 */
pub0_208c9c:
.word 0x3442e000 /* retained 0x208c9c */
pub0_208ca0:
.word 0x3c038000 /* retained 0x208ca0 */
pub0_208ca4:
.word 0x00411024 /* retained 0x208ca4 */
pub0_208ca8:
.word 0x0002180a /* retained 0x208ca8 */
pub0_208cac:
.word 0x00230825 /* retained 0x208cac */
pub0_208cb0:
.word 0x03e00008 /* retained 0x208cb0 */
pub0_208cb4:
.word 0xac8100fc /* retained 0x208cb4 */
.globl np_gte_intpl_lm1
np_gte_intpl_lm1:
int1_209770:
.word 0x240b1000 /* retained 0x209770 */
int1_209774:
.word 0x240c1000 /* retained 0x209774 */
int1_209778:
.word 0x240d1000 /* retained 0x209778 */
int1_20977c:
.word 0x8c880024 /* retained 0x20977c */
int1_209780:
.word 0x8c890028 /* retained 0x209780 */
int1_209784:
.word 0x8c8a002c /* retained 0x209784 */
int1_209788:
.word 0x010b4018 /* retained 0x209788 */
int1_20978c:
.word 0x012c4818 /* retained 0x20978c */
int1_209790:
.word 0x714d5018 /* retained 0x209790 */
int1_209794:
pxor $at,$at,$at
int1_209798:
.word 0x8c820054 /* retained 0x209798 */
int1_20979c:
.word 0x8c830058 /* retained 0x20979c */
int1_2097a0:
.word 0xac820050 /* retained 0x2097a0 */
int1_2097a4:
.word 0xac830054 /* retained 0x2097a4 */
int1_2097a8:
.word 0x8c8b00d4 /* retained 0x2097a8 */
int1_2097ac:
.word 0x8c8c00d8 /* retained 0x2097ac */
int1_2097b0:
.word 0x8c8d00dc /* retained 0x2097b0 */
int1_2097b4:
.word 0x000b5b38 /* retained 0x2097b4 */
int1_2097b8:
.word 0x000c6338 /* retained 0x2097b8 */
int1_2097bc:
.word 0x000d6b38 /* retained 0x2097bc */
int1_2097c0:
.word 0x0168582f /* retained 0x2097c0 */
int1_2097c4:
.word 0x0189602f /* retained 0x2097c4 */
int1_2097c8:
.word 0x01aa682f /* retained 0x2097c8 */
int1_2097cc:
.word 0x000b5b3b /* retained 0x2097cc */
int1_2097d0:
.word 0x000c633b /* retained 0x2097d0 */
int1_2097d4:
.word 0x000d6b3b /* retained 0x2097d4 */
int1_2097d8:
.word 0x24028000 /* retained 0x2097d8 */
int1_2097dc:
.word 0x24037fff /* retained 0x2097dc */
int1_2097e0:
.word 0x0162702a /* retained 0x2097e0 */
int1_2097e4:
.word 0x006b782a /* retained 0x2097e4 */
int1_2097e8:
.word 0x004e580b /* retained 0x2097e8 */
int1_2097ec:
.word 0x006f580b /* retained 0x2097ec */
int1_2097f0:
.word 0x01cf7025 /* retained 0x2097f0 */
int1_2097f4:
.word 0x000e7600 /* retained 0x2097f4 */
int1_2097f8:
.word 0x002e0825 /* retained 0x2097f8 */
int1_2097fc:
.word 0x0182702a /* retained 0x2097fc */
int1_209800:
.word 0x006c782a /* retained 0x209800 */
int1_209804:
.word 0x004e600b /* retained 0x209804 */
int1_209808:
.word 0x006f600b /* retained 0x209808 */
int1_20980c:
.word 0x01cf7025 /* retained 0x20980c */
int1_209810:
.word 0x000e75c0 /* retained 0x209810 */
int1_209814:
.word 0x002e0825 /* retained 0x209814 */
int1_209818:
.word 0x01a2702a /* retained 0x209818 */
int1_20981c:
.word 0x006d782a /* retained 0x20981c */
int1_209820:
.word 0x004e680b /* retained 0x209820 */
int1_209824:
.word 0x006f680b /* retained 0x209824 */
int1_209828:
.word 0x01cf7025 /* retained 0x209828 */
int1_20982c:
.word 0x000e7580 /* retained 0x20982c */
int1_209830:
.word 0x002e0825 /* retained 0x209830 */
int1_209834:
.word 0x8c8e0020 /* retained 0x209834 */
int1_209838:
.word 0x016e5818 /* retained 0x209838 */
int1_20983c:
.word 0x018e6018 /* retained 0x20983c */
int1_209840:
.word 0x71ae6818 /* retained 0x209840 */
int1_209844:
.word 0x010b402d /* retained 0x209844 */
int1_209848:
.word 0x012c482d /* retained 0x209848 */
int1_20984c:
.word 0x014d502d /* retained 0x20984c */
int1_209850:
.word 0x0008433b /* retained 0x209850 */
int1_209854:
.word 0x00094b3b /* retained 0x209854 */
int1_209858:
.word 0x000a533b /* retained 0x209858 */
pextlw $8,$9,$8
    pcpyld $8,$10,$8
    lui $v0,0x7000
pxor $14,$14,$14
lq $15,800($v0)
lq $24,768($v0)
pub1_208bf4:
.word 0x0008103f /* retained 0x208bf4 */
pub1_208bf8:
.word 0xac880064 /* retained 0x208bf8 */
pub1_208bfc:
.word 0x71081ba9 /* retained 0x208bfc */
pub1_208c00:
.word 0xac820068 /* retained 0x208c00 */
pub1_208c04:
.word 0xac83006c /* retained 0x208c04 */
pub1_208c08:
.word 0x71c81088 /* retained 0x208c08 */
pub1_208c0c:
.word 0x71c818c8 /* retained 0x208c0c */
pub1_208c10:
.word 0x70585c89 /* retained 0x208c10 */
pub1_208c14:
.word 0x706f1088 /* retained 0x208c14 */
pub1_208c18:
.word 0x702b0ca9 /* retained 0x208c18 */
pub1_208c1c:
.word 0x70585c89 /* retained 0x208c1c */
pub1_208c20:
.word 0x706f48e8 /* retained 0x208c20 */
pub1_208c24:
.word 0x702b0ca9 /* retained 0x208c24 */
pub1_208c28:
.word 0x0009103f /* retained 0x208c28 */
pub1_208c2c:
.word 0xac890024 /* retained 0x208c2c */
pub1_208c30:
.word 0x71291ba9 /* retained 0x208c30 */
pub1_208c34:
.word 0xac820028 /* retained 0x208c34 */
pub1_208c38:
.word 0xac83002c /* retained 0x208c38 */
pub1_208c3c:
.word 0x7008413f /* retained 0x208c3c */
pub1_208c40:
.word 0x3c027000 /* retained 0x208c40 */
pub1_208c44:
.word 0x78430330 /* retained 0x208c44 */
pub1_208c48:
.word 0x78420340 /* retained 0x208c48 */
pub1_208c4c:
.word 0x70085888 /* retained 0x208c4c */
pub1_208c50:
.word 0x700860c8 /* retained 0x208c50 */
pub1_208c54:
.word 0x71636c89 /* retained 0x208c54 */
pub1_208c58:
.word 0x71825888 /* retained 0x208c58 */
pub1_208c5c:
.word 0x702d0ca9 /* retained 0x208c5c */
pub1_208c60:
.word 0x71636c89 /* retained 0x208c60 */
pub1_208c64:
.word 0x718248e8 /* retained 0x208c64 */
pub1_208c68:
.word 0x702d0ca9 /* retained 0x208c68 */
pub1_208c6c:
.word 0x808b001b /* retained 0x208c6c */
pub1_208c70:
.word 0x0009103f /* retained 0x208c70 */
pub1_208c74:
.word 0xa0890058 /* retained 0x208c74 */
pub1_208c78:
.word 0x71291ba9 /* retained 0x208c78 */
pub1_208c7c:
.word 0xa0820059 /* retained 0x208c7c */
pub1_208c80:
.word 0xa083005a /* retained 0x208c80 */
pub1_208c84:
.word 0xa08b005b /* retained 0x208c84 */
pub1_208c88:
.word 0x0001103e /* retained 0x208c88 */
pub1_208c8c:
.word 0x70211ca8 /* retained 0x208c8c */
pub1_208c90:
.word 0x00220825 /* retained 0x208c90 */
pub1_208c94:
.word 0x00230825 /* retained 0x208c94 */
pub1_208c98:
.word 0x3c027f87 /* retained 0x208c98 */
pub1_208c9c:
.word 0x3442e000 /* retained 0x208c9c */
pub1_208ca0:
.word 0x3c038000 /* retained 0x208ca0 */
pub1_208ca4:
.word 0x00411024 /* retained 0x208ca4 */
pub1_208ca8:
.word 0x0002180a /* retained 0x208ca8 */
pub1_208cac:
.word 0x00230825 /* retained 0x208cac */
pub1_208cb0:
.word 0x03e00008 /* retained 0x208cb0 */
pub1_208cb4:
.word 0xac8100fc /* retained 0x208cb4 */
.globl np_gte_rtps_lm1
np_gte_rtps_lm1:
proj_2088c8:
.word 0x8c820034 /* retained 0x2088c8 */
proj_2088cc:
.word 0x8c830038 /* retained 0x2088cc */
proj_2088d0:
.word 0x94880044 /* retained 0x2088d0 */
proj_2088d4:
.word 0x94890048 /* retained 0x2088d4 */
proj_2088d8:
.word 0x948a004c /* retained 0x2088d8 */
proj_2088dc:
.word 0xac820030 /* retained 0x2088dc */
proj_2088e0:
.word 0xac830034 /* retained 0x2088e0 */
proj_2088e4:
.word 0xa4880040 /* retained 0x2088e4 */
proj_2088e8:
.word 0xa4890044 /* retained 0x2088e8 */
proj_2088ec:
.word 0xa48a0048 /* retained 0x2088ec */
proj_2088f0:
.word 0x24190001 /* retained 0x2088f0 */
proj_2088f4:
.word 0x24850008 /* retained 0x2088f4 */
proj_2088f8:
.word 0x0000082d /* retained 0x2088f8 */
proj_2088fc:
.word 0x0080302d /* retained 0x2088fc */
proj_208900:
.word 0x84ce0000 /* retained 0x208900 */
proj_208904:
.word 0x84cf0002 /* retained 0x208904 */
proj_208908:
.word 0x84d80004 /* retained 0x208908 */
proj_20890c:
.word 0x848b0080 /* retained 0x20890c */
proj_208910:
.word 0x848c0082 /* retained 0x208910 */
proj_208914:
.word 0x016e5818 /* retained 0x208914 */
proj_208918:
.word 0x718f6018 /* retained 0x208918 */
proj_20891c:
.word 0x8c880094 /* retained 0x20891c */
proj_208920:
.word 0x848d0084 /* retained 0x208920 */
proj_208924:
.word 0x00084338 /* retained 0x208924 */
proj_208928:
.word 0x01b86818 /* retained 0x208928 */
proj_20892c:
.word 0x010b402d /* retained 0x20892c */
proj_208930:
.word 0x010c402d /* retained 0x208930 */
proj_208934:
.word 0x010d402d /* retained 0x208934 */
proj_208938:
.word 0x848b0086 /* retained 0x208938 */
proj_20893c:
.word 0x848c0088 /* retained 0x20893c */
proj_208940:
.word 0x016e5818 /* retained 0x208940 */
proj_208944:
.word 0x718f6018 /* retained 0x208944 */
proj_208948:
.word 0x8c890098 /* retained 0x208948 */
proj_20894c:
.word 0x848d008a /* retained 0x20894c */
proj_208950:
.word 0x00094b38 /* retained 0x208950 */
proj_208954:
.word 0x01b86818 /* retained 0x208954 */
proj_208958:
.word 0x012b482d /* retained 0x208958 */
proj_20895c:
.word 0x012c482d /* retained 0x20895c */
proj_208960:
.word 0x012d482d /* retained 0x208960 */
proj_208964:
.word 0x848b008c /* retained 0x208964 */
proj_208968:
.word 0x848c008e /* retained 0x208968 */
proj_20896c:
.word 0x016e5818 /* retained 0x20896c */
proj_208970:
.word 0x718f1818 /* retained 0x208970 */
proj_208974:
.word 0x8c8a009c /* retained 0x208974 */
proj_208978:
.word 0x848d0090 /* retained 0x208978 */
proj_20897c:
.word 0x000a5338 /* retained 0x20897c */
proj_208980:
.word 0x01b86818 /* retained 0x208980 */
proj_208984:
.word 0x014b502d /* retained 0x208984 */
proj_208988:
.word 0x0143502d /* retained 0x208988 */
proj_20898c:
.word 0x014d502d /* retained 0x20898c */
proj_208990:
.word 0x00084538 /* retained 0x208990 */
proj_208994:
.word 0x00094d38 /* retained 0x208994 */
proj_208998:
.word 0x000a5538 /* retained 0x208998 */
proj_20899c:
.word 0x0008403f /* retained 0x20899c */
proj_2089a0:
.word 0x0009483f /* retained 0x2089a0 */
proj_2089a4:
.word 0x000a503f /* retained 0x2089a4 */
proj_2089a8:
.word 0xac880064 /* retained 0x2089a8 */
proj_2089ac:
.word 0xac890068 /* retained 0x2089ac */
proj_2089b0:
.word 0xac8a006c /* retained 0x2089b0 */
proj_2089b4:
.word 0x0140582d /* retained 0x2089b4 */
proj_2089b8:
move $v0,$zero
proj_2089bc:
.word 0x24037fff /* retained 0x2089bc */
proj_2089c0:
.word 0x3c0e8100 /* retained 0x2089c0 */
proj_2089c4:
.word 0x0102782a /* retained 0x2089c4 */
proj_2089c8:
.word 0x004f400b /* retained 0x2089c8 */
proj_2089cc:
.word 0x01cf780b /* retained 0x2089cc */
proj_2089d0:
.word 0x002f0825 /* retained 0x2089d0 */
proj_2089d4:
.word 0x0068782a /* retained 0x2089d4 */
proj_2089d8:
.word 0x006f400b /* retained 0x2089d8 */
proj_2089dc:
.word 0x01cf780b /* retained 0x2089dc */
proj_2089e0:
.word 0x002f0825 /* retained 0x2089e0 */
proj_2089e4:
.word 0x3c0e8080 /* retained 0x2089e4 */
proj_2089e8:
.word 0x0122782a /* retained 0x2089e8 */
proj_2089ec:
.word 0x004f480b /* retained 0x2089ec */
proj_2089f0:
.word 0x01cf780b /* retained 0x2089f0 */
proj_2089f4:
.word 0x002f0825 /* retained 0x2089f4 */
proj_2089f8:
.word 0x0069782a /* retained 0x2089f8 */
proj_2089fc:
.word 0x006f480b /* retained 0x2089fc */
proj_208a00:
.word 0x01cf780b /* retained 0x208a00 */
proj_208a04:
.word 0x002f0825 /* retained 0x208a04 */
proj_208a08:
li $v0,-32768
proj_208a0c:
slt $15,$10,$v0
proj_208a10:
slt $14,$v1,$10
proj_208a14:
or $15,$15,$14
proj_208a18:
sll $15,$15,22
proj_208a1c:
or $at,$at,$15
proj_208a20:
movn $10,$v1,$14
proj_208a24:
slt $15,$10,$zero
proj_208a28:
movn $10,$zero,$15
proj_208a2c:
.word 0xac880024 /* retained 0x208a2c */
proj_208a30:
.word 0xac890028 /* retained 0x208a30 */
proj_208a34:
.word 0xac8a002c /* retained 0x208a34 */
proj_208a38:
.word 0x3402ffff /* retained 0x208a38 */
proj_208a3c:
.word 0x3c038006 /* retained 0x208a3c */
proj_208a40:
.word 0x0160702a /* retained 0x208a40 */
proj_208a44:
.word 0x000e580b /* retained 0x208a44 */
proj_208a48:
.word 0x006e700b /* retained 0x208a48 */
proj_208a4c:
.word 0x002e0825 /* retained 0x208a4c */
proj_208a50:
.word 0x004b702a /* retained 0x208a50 */
proj_208a54:
.word 0x004e580b /* retained 0x208a54 */
proj_208a58:
.word 0x006e700b /* retained 0x208a58 */
proj_208a5c:
.word 0x002e0825 /* retained 0x208a5c */
proj_208a60:
.word 0xa4ab0044 /* retained 0x208a60 */
proj_208a64:
lhu $12,232($a0)
proj_208a68:
sll $v0,$11,1
proj_208a6c:
sltu $v0,$12,$v0
proj_208a70:
bnez $v0,proj_208a80
proj_208a74:
sll $13,$12,16
proj_208a78:
j np_gte_projection_zero+8
proj_208a7c:
nop
proj_208a80:
.word 0x01ab001b /* DIVU only when H < 2*SZ3; SZ3 is nonzero. */
proj_208a84:
mflo $11
proj_208a88:
nop
proj_208a8c:
.word 0x010b0018 /* retained 0x208a8c */
proj_208a90:
.word 0x712b0018 /* retained 0x208a90 */
proj_208a94:
.word 0x8c8c00e0 /* retained 0x208a94 */
proj_208a98:
.word 0x8c8d00e4 /* retained 0x208a98 */
proj_208a9c:
.word 0x70001030 /* retained 0x208a9c */
proj_208aa0:
.word 0x70421ba9 /* retained 0x208aa0 */
proj_208aa4:
.word 0x0182602d /* retained 0x208aa4 */
proj_208aa8:
.word 0x01a3682d /* retained 0x208aa8 */
proj_208aac:
j np_gte_screen_proj
proj_208ab0:
dsra $12,$12,16
proj_208ab4:
.word 0x3c0e8000 /* retained 0x208ab4 */
proj_208ab8:
.word 0x2402fc00 /* retained 0x208ab8 */
proj_208abc:
.word 0x240303ff /* retained 0x208abc */
proj_208ac0:
.word 0x35ce4000 /* retained 0x208ac0 */
proj_208ac4:
.word 0x0182782a /* retained 0x208ac4 */
proj_208ac8:
.word 0x004f600b /* retained 0x208ac8 */
proj_208acc:
.word 0x01cf780b /* retained 0x208acc */
proj_208ad0:
.word 0x002f0825 /* retained 0x208ad0 */
proj_208ad4:
.word 0x006c782a /* retained 0x208ad4 */
proj_208ad8:
.word 0x006f600b /* retained 0x208ad8 */
proj_208adc:
.word 0x01cf780b /* retained 0x208adc */
proj_208ae0:
.word 0x002f0825 /* retained 0x208ae0 */
proj_208ae4:
.word 0x25cee000 /* retained 0x208ae4 */
proj_208ae8:
.word 0x01a2782a /* retained 0x208ae8 */
proj_208aec:
.word 0x004f680b /* retained 0x208aec */
proj_208af0:
.word 0x01cf780b /* retained 0x208af0 */
proj_208af4:
.word 0x002f0825 /* retained 0x208af4 */
proj_208af8:
.word 0x006d782a /* retained 0x208af8 */
proj_208afc:
.word 0x006f680b /* retained 0x208afc */
proj_208b00:
.word 0x01cf780b /* retained 0x208b00 */
proj_208b04:
.word 0x002f0825 /* retained 0x208b04 */
proj_208b08:
.word 0xa4ac0030 /* retained 0x208b08 */
proj_208b0c:
.word 0xa4ad0032 /* retained 0x208b0c */
proj_208b10:
.word 0x2739ffff /* retained 0x208b10 */
proj_208b14:
.word 0x24a50004 /* retained 0x208b14 */
proj_208b18:
.word 0x17200000 | (((proj_208900 - . - 4) / 4) & 0xffff)
proj_208b1c:
.word 0x24c60008 /* retained 0x208b1c */
proj_208b20:
.word 0x848200ec /* retained 0x208b20 */
proj_208b24:
lw $v1,240($a0)
proj_208b28:
bnez $v0,np_gte_depth_nonzero
proj_208b2c:
nop
proj_208b30:
sw $v1,96($a0)
proj_208b34:
dsra $11,$v1,12
proj_208b38:
.word 0x24031000 /* retained 0x208b38 */
proj_208b3c:
.word 0x0160702a /* retained 0x208b3c */
proj_208b40:
.word 0x006b782a /* retained 0x208b40 */
proj_208b44:
.word 0x000e580b /* retained 0x208b44 */
proj_208b48:
.word 0x006f580b /* retained 0x208b48 */
proj_208b4c:
.word 0x01cf7025 /* retained 0x208b4c */
proj_208b50:
.word 0x000e7300 /* retained 0x208b50 */
proj_208b54:
.word 0x002e0825 /* retained 0x208b54 */
proj_208b58:
.word 0xac8b0020 /* retained 0x208b58 */
proj_208b5c:
.word 0x03e00008 /* retained 0x208b5c */
proj_208b60:
.word 0xac8100fc /* retained 0x208b60 */
proj_208b64:
.word 0x00000000 /* retained 0x208b64 */
proj_208b68:
.globl np_gte_rtpt_lm1
np_gte_rtpt_lm1:
li $25,3
proj_208b6c:
.word 0x9482004c /* retained 0x208b6c */
proj_208b70:
.word 0x0080282d /* retained 0x208b70 */
proj_208b74:
.word 0x10000000 | (((proj_2088f8 - . - 4) / 4) & 0xffff)
proj_208b78:
.word 0xa4820040 /* retained 0x208b78 */
np_gte_projection_zero:
    bnez $11,1f
    nop
    lui $11,1
    ori $11,$11,0xffff
    lui $v0,0x8002
    or $at,$at,$v0
    j proj_208a8c
    nop
1:  .word 0x01ab001b
    mflo $11
    j proj_208a8c
    nop

.globl np_gte_select_rtps
np_gte_select_rtps:
    srl $v0,$s0,19
    andi $v0,$v0,1
    bnez $v0,select_rtps_sf1
    andi $v1,$s0,0x400
    la $a0,np_gte_rtps_sf0_lm0
    la $v0,np_gte_rtps_sf0_lm1
    movn $a0,$v0,$v1
    j 0x206c5c
    nop
select_rtps_sf1:
    la $a0,np_gte_hot_rtps
    la $v0,np_gte_rtps_lm1
    movn $a0,$v0,$v1
    j 0x206c5c
    nop
.globl np_gte_select_rtpt
np_gte_select_rtpt:
    srl $v0,$s0,19
    andi $v0,$v0,1
    bnez $v0,select_rtpt_sf1
    andi $v1,$s0,0x400
    la $a0,np_gte_rtpt_sf0_lm0
    la $v0,np_gte_rtpt_sf0_lm1
    movn $a0,$v0,$v1
    j 0x206c5c
    nop
select_rtpt_sf1:
    la $a0,np_gte_hot_rtpt
    la $v0,np_gte_rtpt_lm1
    movn $a0,$v0,$v1
    j 0x206c5c
    nop
.globl np_gte_select_intpl
np_gte_select_intpl:
    srl $v0,$s0,19
    andi $v0,$v0,1
    bnez $v0,select_intpl_sf1
    andi $v1,$s0,0x400
    la $a0,np_gte_intpl_sf0_lm0
    la $v0,np_gte_intpl_sf0_lm1
    movn $a0,$v0,$v1
    j 0x206c5c
    nop
select_intpl_sf1:
    la $a0,np_gte_intpl_lm0
    la $v0,np_gte_intpl_lm1
    movn $a0,$v0,$v1
    j 0x206c5c
    nop
.globl np_gte_intpl_sf0_lm0
np_gte_intpl_sf0_lm0:
int_sf0_0_209770:
.word 0x240b1000 /* retained 0x209770 */
int_sf0_0_209774:
.word 0x240c1000 /* retained 0x209774 */
int_sf0_0_209778:
.word 0x240d1000 /* retained 0x209778 */
int_sf0_0_20977c:
.word 0x8c880024 /* retained 0x20977c */
int_sf0_0_209780:
.word 0x8c890028 /* retained 0x209780 */
int_sf0_0_209784:
.word 0x8c8a002c /* retained 0x209784 */
int_sf0_0_209788:
.word 0x010b4018 /* retained 0x209788 */
int_sf0_0_20978c:
.word 0x012c4818 /* retained 0x20978c */
int_sf0_0_209790:
.word 0x714d5018 /* retained 0x209790 */
int_sf0_0_209794:
pxor $at,$at,$at
int_sf0_0_209798:
.word 0x8c820054 /* retained 0x209798 */
int_sf0_0_20979c:
.word 0x8c830058 /* retained 0x20979c */
int_sf0_0_2097a0:
.word 0xac820050 /* retained 0x2097a0 */
int_sf0_0_2097a4:
.word 0xac830054 /* retained 0x2097a4 */
int_sf0_0_2097a8:
.word 0x8c8b00d4 /* retained 0x2097a8 */
int_sf0_0_2097ac:
.word 0x8c8c00d8 /* retained 0x2097ac */
int_sf0_0_2097b0:
.word 0x8c8d00dc /* retained 0x2097b0 */
int_sf0_0_2097b4:
.word 0x000b5b38 /* retained 0x2097b4 */
int_sf0_0_2097b8:
.word 0x000c6338 /* retained 0x2097b8 */
int_sf0_0_2097bc:
.word 0x000d6b38 /* retained 0x2097bc */
int_sf0_0_2097c0:
.word 0x0168582f /* retained 0x2097c0 */
int_sf0_0_2097c4:
.word 0x0189602f /* retained 0x2097c4 */
int_sf0_0_2097c8:
.word 0x01aa682f /* retained 0x2097c8 */
int_sf0_0_2097cc:
sll $11,$11,0
int_sf0_0_2097d0:
sll $12,$12,0
int_sf0_0_2097d4:
sll $13,$13,0
int_sf0_0_2097d8:
.word 0x24028000 /* retained 0x2097d8 */
int_sf0_0_2097dc:
.word 0x24037fff /* retained 0x2097dc */
int_sf0_0_2097e0:
.word 0x0162702a /* retained 0x2097e0 */
int_sf0_0_2097e4:
.word 0x006b782a /* retained 0x2097e4 */
int_sf0_0_2097e8:
.word 0x004e580b /* retained 0x2097e8 */
int_sf0_0_2097ec:
.word 0x006f580b /* retained 0x2097ec */
int_sf0_0_2097f0:
.word 0x01cf7025 /* retained 0x2097f0 */
int_sf0_0_2097f4:
.word 0x000e7600 /* retained 0x2097f4 */
int_sf0_0_2097f8:
.word 0x002e0825 /* retained 0x2097f8 */
int_sf0_0_2097fc:
.word 0x0182702a /* retained 0x2097fc */
int_sf0_0_209800:
.word 0x006c782a /* retained 0x209800 */
int_sf0_0_209804:
.word 0x004e600b /* retained 0x209804 */
int_sf0_0_209808:
.word 0x006f600b /* retained 0x209808 */
int_sf0_0_20980c:
.word 0x01cf7025 /* retained 0x20980c */
int_sf0_0_209810:
.word 0x000e75c0 /* retained 0x209810 */
int_sf0_0_209814:
.word 0x002e0825 /* retained 0x209814 */
int_sf0_0_209818:
.word 0x01a2702a /* retained 0x209818 */
int_sf0_0_20981c:
.word 0x006d782a /* retained 0x20981c */
int_sf0_0_209820:
.word 0x004e680b /* retained 0x209820 */
int_sf0_0_209824:
.word 0x006f680b /* retained 0x209824 */
int_sf0_0_209828:
.word 0x01cf7025 /* retained 0x209828 */
int_sf0_0_20982c:
.word 0x000e7580 /* retained 0x20982c */
int_sf0_0_209830:
.word 0x002e0825 /* retained 0x209830 */
int_sf0_0_209834:
.word 0x8c8e0020 /* retained 0x209834 */
int_sf0_0_209838:
.word 0x016e5818 /* retained 0x209838 */
int_sf0_0_20983c:
.word 0x018e6018 /* retained 0x20983c */
int_sf0_0_209840:
.word 0x71ae6818 /* retained 0x209840 */
int_sf0_0_209844:
.word 0x010b402d /* retained 0x209844 */
int_sf0_0_209848:
.word 0x012c482d /* retained 0x209848 */
int_sf0_0_20984c:
.word 0x014d502d /* retained 0x20984c */
int_sf0_0_209850:
sll $8,$8,0
int_sf0_0_209854:
sll $9,$9,0
int_sf0_0_209858:
sll $10,$10,0
pextlw $8,$9,$8
pcpyld $8,$10,$8
lui $v0,0x7000
lq $14,784($v0)
lq $15,800($v0)
lq $24,768($v0)
pub_sf0_0_208bf4:
.word 0x0008103f /* retained 0x208bf4 */
pub_sf0_0_208bf8:
.word 0xac880064 /* retained 0x208bf8 */
pub_sf0_0_208bfc:
.word 0x71081ba9 /* retained 0x208bfc */
pub_sf0_0_208c00:
.word 0xac820068 /* retained 0x208c00 */
pub_sf0_0_208c04:
.word 0xac83006c /* retained 0x208c04 */
pub_sf0_0_208c08:
.word 0x71c81088 /* retained 0x208c08 */
pub_sf0_0_208c0c:
.word 0x71c818c8 /* retained 0x208c0c */
pub_sf0_0_208c10:
.word 0x70585c89 /* retained 0x208c10 */
pub_sf0_0_208c14:
.word 0x706f1088 /* retained 0x208c14 */
pub_sf0_0_208c18:
.word 0x702b0ca9 /* retained 0x208c18 */
pub_sf0_0_208c1c:
.word 0x70585c89 /* retained 0x208c1c */
pub_sf0_0_208c20:
.word 0x706f48e8 /* retained 0x208c20 */
pub_sf0_0_208c24:
.word 0x702b0ca9 /* retained 0x208c24 */
pub_sf0_0_208c28:
.word 0x0009103f /* retained 0x208c28 */
pub_sf0_0_208c2c:
.word 0xac890024 /* retained 0x208c2c */
pub_sf0_0_208c30:
.word 0x71291ba9 /* retained 0x208c30 */
pub_sf0_0_208c34:
.word 0xac820028 /* retained 0x208c34 */
pub_sf0_0_208c38:
.word 0xac83002c /* retained 0x208c38 */
pub_sf0_0_208c3c:
.word 0x7008413f /* retained 0x208c3c */
pub_sf0_0_208c40:
.word 0x3c027000 /* retained 0x208c40 */
pub_sf0_0_208c44:
.word 0x78430330 /* retained 0x208c44 */
pub_sf0_0_208c48:
.word 0x78420340 /* retained 0x208c48 */
pub_sf0_0_208c4c:
.word 0x70085888 /* retained 0x208c4c */
pub_sf0_0_208c50:
.word 0x700860c8 /* retained 0x208c50 */
pub_sf0_0_208c54:
.word 0x71636c89 /* retained 0x208c54 */
pub_sf0_0_208c58:
.word 0x71825888 /* retained 0x208c58 */
pub_sf0_0_208c5c:
.word 0x702d0ca9 /* retained 0x208c5c */
pub_sf0_0_208c60:
.word 0x71636c89 /* retained 0x208c60 */
pub_sf0_0_208c64:
.word 0x718248e8 /* retained 0x208c64 */
pub_sf0_0_208c68:
.word 0x702d0ca9 /* retained 0x208c68 */
pub_sf0_0_208c6c:
.word 0x808b001b /* retained 0x208c6c */
pub_sf0_0_208c70:
.word 0x0009103f /* retained 0x208c70 */
pub_sf0_0_208c74:
.word 0xa0890058 /* retained 0x208c74 */
pub_sf0_0_208c78:
.word 0x71291ba9 /* retained 0x208c78 */
pub_sf0_0_208c7c:
.word 0xa0820059 /* retained 0x208c7c */
pub_sf0_0_208c80:
.word 0xa083005a /* retained 0x208c80 */
pub_sf0_0_208c84:
.word 0xa08b005b /* retained 0x208c84 */
pub_sf0_0_208c88:
.word 0x0001103e /* retained 0x208c88 */
pub_sf0_0_208c8c:
.word 0x70211ca8 /* retained 0x208c8c */
pub_sf0_0_208c90:
.word 0x00220825 /* retained 0x208c90 */
pub_sf0_0_208c94:
.word 0x00230825 /* retained 0x208c94 */
pub_sf0_0_208c98:
.word 0x3c027f87 /* retained 0x208c98 */
pub_sf0_0_208c9c:
.word 0x3442e000 /* retained 0x208c9c */
pub_sf0_0_208ca0:
.word 0x3c038000 /* retained 0x208ca0 */
pub_sf0_0_208ca4:
.word 0x00411024 /* retained 0x208ca4 */
pub_sf0_0_208ca8:
.word 0x0002180a /* retained 0x208ca8 */
pub_sf0_0_208cac:
.word 0x00230825 /* retained 0x208cac */
pub_sf0_0_208cb0:
.word 0x03e00008 /* retained 0x208cb0 */
pub_sf0_0_208cb4:
.word 0xac8100fc /* retained 0x208cb4 */
.globl np_gte_intpl_sf0_lm1
np_gte_intpl_sf0_lm1:
int_sf0_1_209770:
.word 0x240b1000 /* retained 0x209770 */
int_sf0_1_209774:
.word 0x240c1000 /* retained 0x209774 */
int_sf0_1_209778:
.word 0x240d1000 /* retained 0x209778 */
int_sf0_1_20977c:
.word 0x8c880024 /* retained 0x20977c */
int_sf0_1_209780:
.word 0x8c890028 /* retained 0x209780 */
int_sf0_1_209784:
.word 0x8c8a002c /* retained 0x209784 */
int_sf0_1_209788:
.word 0x010b4018 /* retained 0x209788 */
int_sf0_1_20978c:
.word 0x012c4818 /* retained 0x20978c */
int_sf0_1_209790:
.word 0x714d5018 /* retained 0x209790 */
int_sf0_1_209794:
pxor $at,$at,$at
int_sf0_1_209798:
.word 0x8c820054 /* retained 0x209798 */
int_sf0_1_20979c:
.word 0x8c830058 /* retained 0x20979c */
int_sf0_1_2097a0:
.word 0xac820050 /* retained 0x2097a0 */
int_sf0_1_2097a4:
.word 0xac830054 /* retained 0x2097a4 */
int_sf0_1_2097a8:
.word 0x8c8b00d4 /* retained 0x2097a8 */
int_sf0_1_2097ac:
.word 0x8c8c00d8 /* retained 0x2097ac */
int_sf0_1_2097b0:
.word 0x8c8d00dc /* retained 0x2097b0 */
int_sf0_1_2097b4:
.word 0x000b5b38 /* retained 0x2097b4 */
int_sf0_1_2097b8:
.word 0x000c6338 /* retained 0x2097b8 */
int_sf0_1_2097bc:
.word 0x000d6b38 /* retained 0x2097bc */
int_sf0_1_2097c0:
.word 0x0168582f /* retained 0x2097c0 */
int_sf0_1_2097c4:
.word 0x0189602f /* retained 0x2097c4 */
int_sf0_1_2097c8:
.word 0x01aa682f /* retained 0x2097c8 */
int_sf0_1_2097cc:
sll $11,$11,0
int_sf0_1_2097d0:
sll $12,$12,0
int_sf0_1_2097d4:
sll $13,$13,0
int_sf0_1_2097d8:
.word 0x24028000 /* retained 0x2097d8 */
int_sf0_1_2097dc:
.word 0x24037fff /* retained 0x2097dc */
int_sf0_1_2097e0:
.word 0x0162702a /* retained 0x2097e0 */
int_sf0_1_2097e4:
.word 0x006b782a /* retained 0x2097e4 */
int_sf0_1_2097e8:
.word 0x004e580b /* retained 0x2097e8 */
int_sf0_1_2097ec:
.word 0x006f580b /* retained 0x2097ec */
int_sf0_1_2097f0:
.word 0x01cf7025 /* retained 0x2097f0 */
int_sf0_1_2097f4:
.word 0x000e7600 /* retained 0x2097f4 */
int_sf0_1_2097f8:
.word 0x002e0825 /* retained 0x2097f8 */
int_sf0_1_2097fc:
.word 0x0182702a /* retained 0x2097fc */
int_sf0_1_209800:
.word 0x006c782a /* retained 0x209800 */
int_sf0_1_209804:
.word 0x004e600b /* retained 0x209804 */
int_sf0_1_209808:
.word 0x006f600b /* retained 0x209808 */
int_sf0_1_20980c:
.word 0x01cf7025 /* retained 0x20980c */
int_sf0_1_209810:
.word 0x000e75c0 /* retained 0x209810 */
int_sf0_1_209814:
.word 0x002e0825 /* retained 0x209814 */
int_sf0_1_209818:
.word 0x01a2702a /* retained 0x209818 */
int_sf0_1_20981c:
.word 0x006d782a /* retained 0x20981c */
int_sf0_1_209820:
.word 0x004e680b /* retained 0x209820 */
int_sf0_1_209824:
.word 0x006f680b /* retained 0x209824 */
int_sf0_1_209828:
.word 0x01cf7025 /* retained 0x209828 */
int_sf0_1_20982c:
.word 0x000e7580 /* retained 0x20982c */
int_sf0_1_209830:
.word 0x002e0825 /* retained 0x209830 */
int_sf0_1_209834:
.word 0x8c8e0020 /* retained 0x209834 */
int_sf0_1_209838:
.word 0x016e5818 /* retained 0x209838 */
int_sf0_1_20983c:
.word 0x018e6018 /* retained 0x20983c */
int_sf0_1_209840:
.word 0x71ae6818 /* retained 0x209840 */
int_sf0_1_209844:
.word 0x010b402d /* retained 0x209844 */
int_sf0_1_209848:
.word 0x012c482d /* retained 0x209848 */
int_sf0_1_20984c:
.word 0x014d502d /* retained 0x20984c */
int_sf0_1_209850:
sll $8,$8,0
int_sf0_1_209854:
sll $9,$9,0
int_sf0_1_209858:
sll $10,$10,0
pextlw $8,$9,$8
pcpyld $8,$10,$8
lui $v0,0x7000
pxor $14,$14,$14
lq $15,800($v0)
lq $24,768($v0)
pub_sf0_1_208bf4:
.word 0x0008103f /* retained 0x208bf4 */
pub_sf0_1_208bf8:
.word 0xac880064 /* retained 0x208bf8 */
pub_sf0_1_208bfc:
.word 0x71081ba9 /* retained 0x208bfc */
pub_sf0_1_208c00:
.word 0xac820068 /* retained 0x208c00 */
pub_sf0_1_208c04:
.word 0xac83006c /* retained 0x208c04 */
pub_sf0_1_208c08:
.word 0x71c81088 /* retained 0x208c08 */
pub_sf0_1_208c0c:
.word 0x71c818c8 /* retained 0x208c0c */
pub_sf0_1_208c10:
.word 0x70585c89 /* retained 0x208c10 */
pub_sf0_1_208c14:
.word 0x706f1088 /* retained 0x208c14 */
pub_sf0_1_208c18:
.word 0x702b0ca9 /* retained 0x208c18 */
pub_sf0_1_208c1c:
.word 0x70585c89 /* retained 0x208c1c */
pub_sf0_1_208c20:
.word 0x706f48e8 /* retained 0x208c20 */
pub_sf0_1_208c24:
.word 0x702b0ca9 /* retained 0x208c24 */
pub_sf0_1_208c28:
.word 0x0009103f /* retained 0x208c28 */
pub_sf0_1_208c2c:
.word 0xac890024 /* retained 0x208c2c */
pub_sf0_1_208c30:
.word 0x71291ba9 /* retained 0x208c30 */
pub_sf0_1_208c34:
.word 0xac820028 /* retained 0x208c34 */
pub_sf0_1_208c38:
.word 0xac83002c /* retained 0x208c38 */
pub_sf0_1_208c3c:
.word 0x7008413f /* retained 0x208c3c */
pub_sf0_1_208c40:
.word 0x3c027000 /* retained 0x208c40 */
pub_sf0_1_208c44:
.word 0x78430330 /* retained 0x208c44 */
pub_sf0_1_208c48:
.word 0x78420340 /* retained 0x208c48 */
pub_sf0_1_208c4c:
.word 0x70085888 /* retained 0x208c4c */
pub_sf0_1_208c50:
.word 0x700860c8 /* retained 0x208c50 */
pub_sf0_1_208c54:
.word 0x71636c89 /* retained 0x208c54 */
pub_sf0_1_208c58:
.word 0x71825888 /* retained 0x208c58 */
pub_sf0_1_208c5c:
.word 0x702d0ca9 /* retained 0x208c5c */
pub_sf0_1_208c60:
.word 0x71636c89 /* retained 0x208c60 */
pub_sf0_1_208c64:
.word 0x718248e8 /* retained 0x208c64 */
pub_sf0_1_208c68:
.word 0x702d0ca9 /* retained 0x208c68 */
pub_sf0_1_208c6c:
.word 0x808b001b /* retained 0x208c6c */
pub_sf0_1_208c70:
.word 0x0009103f /* retained 0x208c70 */
pub_sf0_1_208c74:
.word 0xa0890058 /* retained 0x208c74 */
pub_sf0_1_208c78:
.word 0x71291ba9 /* retained 0x208c78 */
pub_sf0_1_208c7c:
.word 0xa0820059 /* retained 0x208c7c */
pub_sf0_1_208c80:
.word 0xa083005a /* retained 0x208c80 */
pub_sf0_1_208c84:
.word 0xa08b005b /* retained 0x208c84 */
pub_sf0_1_208c88:
.word 0x0001103e /* retained 0x208c88 */
pub_sf0_1_208c8c:
.word 0x70211ca8 /* retained 0x208c8c */
pub_sf0_1_208c90:
.word 0x00220825 /* retained 0x208c90 */
pub_sf0_1_208c94:
.word 0x00230825 /* retained 0x208c94 */
pub_sf0_1_208c98:
.word 0x3c027f87 /* retained 0x208c98 */
pub_sf0_1_208c9c:
.word 0x3442e000 /* retained 0x208c9c */
pub_sf0_1_208ca0:
.word 0x3c038000 /* retained 0x208ca0 */
pub_sf0_1_208ca4:
.word 0x00411024 /* retained 0x208ca4 */
pub_sf0_1_208ca8:
.word 0x0002180a /* retained 0x208ca8 */
pub_sf0_1_208cac:
.word 0x00230825 /* retained 0x208cac */
pub_sf0_1_208cb0:
.word 0x03e00008 /* retained 0x208cb0 */
pub_sf0_1_208cb4:
.word 0xac8100fc /* retained 0x208cb4 */
.globl np_gte_rtps_sf0_lm0
np_gte_rtps_sf0_lm0:
proj_sf0_0_2088c8:
.word 0x8c820034 /* retained 0x2088c8 */
proj_sf0_0_2088cc:
.word 0x8c830038 /* retained 0x2088cc */
proj_sf0_0_2088d0:
.word 0x94880044 /* retained 0x2088d0 */
proj_sf0_0_2088d4:
.word 0x94890048 /* retained 0x2088d4 */
proj_sf0_0_2088d8:
.word 0x948a004c /* retained 0x2088d8 */
proj_sf0_0_2088dc:
.word 0xac820030 /* retained 0x2088dc */
proj_sf0_0_2088e0:
.word 0xac830034 /* retained 0x2088e0 */
proj_sf0_0_2088e4:
.word 0xa4880040 /* retained 0x2088e4 */
proj_sf0_0_2088e8:
.word 0xa4890044 /* retained 0x2088e8 */
proj_sf0_0_2088ec:
.word 0xa48a0048 /* retained 0x2088ec */
proj_sf0_0_2088f0:
.word 0x24190001 /* retained 0x2088f0 */
proj_sf0_0_2088f4:
.word 0x24850008 /* retained 0x2088f4 */
proj_sf0_0_2088f8:
.word 0x0000082d /* retained 0x2088f8 */
proj_sf0_0_2088fc:
.word 0x0080302d /* retained 0x2088fc */
proj_sf0_0_208900:
.word 0x84ce0000 /* retained 0x208900 */
proj_sf0_0_208904:
.word 0x84cf0002 /* retained 0x208904 */
proj_sf0_0_208908:
.word 0x84d80004 /* retained 0x208908 */
proj_sf0_0_20890c:
.word 0x848b0080 /* retained 0x20890c */
proj_sf0_0_208910:
.word 0x848c0082 /* retained 0x208910 */
proj_sf0_0_208914:
.word 0x016e5818 /* retained 0x208914 */
proj_sf0_0_208918:
.word 0x718f6018 /* retained 0x208918 */
proj_sf0_0_20891c:
.word 0x8c880094 /* retained 0x20891c */
proj_sf0_0_208920:
.word 0x848d0084 /* retained 0x208920 */
proj_sf0_0_208924:
.word 0x00084338 /* retained 0x208924 */
proj_sf0_0_208928:
.word 0x01b86818 /* retained 0x208928 */
proj_sf0_0_20892c:
.word 0x010b402d /* retained 0x20892c */
proj_sf0_0_208930:
.word 0x010c402d /* retained 0x208930 */
proj_sf0_0_208934:
.word 0x010d402d /* retained 0x208934 */
proj_sf0_0_208938:
.word 0x848b0086 /* retained 0x208938 */
proj_sf0_0_20893c:
.word 0x848c0088 /* retained 0x20893c */
proj_sf0_0_208940:
.word 0x016e5818 /* retained 0x208940 */
proj_sf0_0_208944:
.word 0x718f6018 /* retained 0x208944 */
proj_sf0_0_208948:
.word 0x8c890098 /* retained 0x208948 */
proj_sf0_0_20894c:
.word 0x848d008a /* retained 0x20894c */
proj_sf0_0_208950:
.word 0x00094b38 /* retained 0x208950 */
proj_sf0_0_208954:
.word 0x01b86818 /* retained 0x208954 */
proj_sf0_0_208958:
.word 0x012b482d /* retained 0x208958 */
proj_sf0_0_20895c:
.word 0x012c482d /* retained 0x20895c */
proj_sf0_0_208960:
.word 0x012d482d /* retained 0x208960 */
proj_sf0_0_208964:
.word 0x848b008c /* retained 0x208964 */
proj_sf0_0_208968:
.word 0x848c008e /* retained 0x208968 */
proj_sf0_0_20896c:
.word 0x016e5818 /* retained 0x20896c */
proj_sf0_0_208970:
.word 0x718f1818 /* retained 0x208970 */
proj_sf0_0_208974:
.word 0x8c8a009c /* retained 0x208974 */
proj_sf0_0_208978:
.word 0x848d0090 /* retained 0x208978 */
proj_sf0_0_20897c:
.word 0x000a5338 /* retained 0x20897c */
proj_sf0_0_208980:
.word 0x01b86818 /* retained 0x208980 */
proj_sf0_0_208984:
.word 0x014b502d /* retained 0x208984 */
proj_sf0_0_208988:
.word 0x0143502d /* retained 0x208988 */
proj_sf0_0_20898c:
.word 0x014d502d /* retained 0x20898c */
proj_sf0_0_208990:
sll $8,$8,0
proj_sf0_0_208994:
sll $9,$9,0
proj_sf0_0_208998:
dsll $11,$10,20
proj_sf0_0_20899c:
dsra32 $11,$11,0
proj_sf0_0_2089a0:
sll $10,$10,0
proj_sf0_0_2089a4:
nop
proj_sf0_0_2089a8:
.word 0xac880064 /* retained 0x2089a8 */
proj_sf0_0_2089ac:
.word 0xac890068 /* retained 0x2089ac */
proj_sf0_0_2089b0:
.word 0xac8a006c /* retained 0x2089b0 */
proj_sf0_0_2089b4:
nop
proj_sf0_0_2089b8:
.word 0x24028000 /* retained 0x2089b8 */
proj_sf0_0_2089bc:
.word 0x24037fff /* retained 0x2089bc */
proj_sf0_0_2089c0:
.word 0x3c0e8100 /* retained 0x2089c0 */
proj_sf0_0_2089c4:
.word 0x0102782a /* retained 0x2089c4 */
proj_sf0_0_2089c8:
.word 0x004f400b /* retained 0x2089c8 */
proj_sf0_0_2089cc:
.word 0x01cf780b /* retained 0x2089cc */
proj_sf0_0_2089d0:
.word 0x002f0825 /* retained 0x2089d0 */
proj_sf0_0_2089d4:
.word 0x0068782a /* retained 0x2089d4 */
proj_sf0_0_2089d8:
.word 0x006f400b /* retained 0x2089d8 */
proj_sf0_0_2089dc:
.word 0x01cf780b /* retained 0x2089dc */
proj_sf0_0_2089e0:
.word 0x002f0825 /* retained 0x2089e0 */
proj_sf0_0_2089e4:
.word 0x3c0e8080 /* retained 0x2089e4 */
proj_sf0_0_2089e8:
.word 0x0122782a /* retained 0x2089e8 */
proj_sf0_0_2089ec:
.word 0x004f480b /* retained 0x2089ec */
proj_sf0_0_2089f0:
.word 0x01cf780b /* retained 0x2089f0 */
proj_sf0_0_2089f4:
.word 0x002f0825 /* retained 0x2089f4 */
proj_sf0_0_2089f8:
.word 0x0069782a /* retained 0x2089f8 */
proj_sf0_0_2089fc:
.word 0x006f480b /* retained 0x2089fc */
proj_sf0_0_208a00:
.word 0x01cf780b /* retained 0x208a00 */
proj_sf0_0_208a04:
.word 0x002f0825 /* retained 0x208a04 */
proj_sf0_0_208a08:
li $v0,-32768
proj_sf0_0_208a0c:
slt $15,$11,$v0
proj_sf0_0_208a10:
slt $14,$v1,$11
proj_sf0_0_208a14:
or $15,$15,$14
proj_sf0_0_208a18:
sll $15,$15,22
proj_sf0_0_208a1c:
or $at,$at,$15
proj_sf0_0_208a20:
pmaxw $10,$10,$v0
proj_sf0_0_208a24:
pminw $10,$10,$v1
proj_sf0_0_208a28:
nop
proj_sf0_0_208a2c:
.word 0xac880024 /* retained 0x208a2c */
proj_sf0_0_208a30:
.word 0xac890028 /* retained 0x208a30 */
proj_sf0_0_208a34:
.word 0xac8a002c /* retained 0x208a34 */
proj_sf0_0_208a38:
.word 0x3402ffff /* retained 0x208a38 */
proj_sf0_0_208a3c:
.word 0x3c038006 /* retained 0x208a3c */
proj_sf0_0_208a40:
.word 0x0160702a /* retained 0x208a40 */
proj_sf0_0_208a44:
.word 0x000e580b /* retained 0x208a44 */
proj_sf0_0_208a48:
.word 0x006e700b /* retained 0x208a48 */
proj_sf0_0_208a4c:
.word 0x002e0825 /* retained 0x208a4c */
proj_sf0_0_208a50:
.word 0x004b702a /* retained 0x208a50 */
proj_sf0_0_208a54:
.word 0x004e580b /* retained 0x208a54 */
proj_sf0_0_208a58:
.word 0x006e700b /* retained 0x208a58 */
proj_sf0_0_208a5c:
.word 0x002e0825 /* retained 0x208a5c */
proj_sf0_0_208a60:
.word 0xa4ab0044 /* retained 0x208a60 */
proj_sf0_0_208a64:
lhu $12,232($a0)
proj_sf0_0_208a68:
sll $v0,$11,1
proj_sf0_0_208a6c:
sltu $v0,$12,$v0
proj_sf0_0_208a70:
bnez $v0,proj_sf0_0_208a80
proj_sf0_0_208a74:
sll $13,$12,16
proj_sf0_0_208a78:
j np_gte_sf0_zero_0+8
proj_sf0_0_208a7c:
nop
proj_sf0_0_208a80:
.word 0x01ab001b /* DIVU only when H < 2*SZ3; SZ3 is nonzero. */
proj_sf0_0_208a84:
mflo $11
proj_sf0_0_208a88:
nop
proj_sf0_0_208a8c:
.word 0x010b0018 /* retained 0x208a8c */
proj_sf0_0_208a90:
.word 0x712b0018 /* retained 0x208a90 */
proj_sf0_0_208a94:
.word 0x8c8c00e0 /* retained 0x208a94 */
proj_sf0_0_208a98:
.word 0x8c8d00e4 /* retained 0x208a98 */
proj_sf0_0_208a9c:
.word 0x70001030 /* retained 0x208a9c */
proj_sf0_0_208aa0:
.word 0x70421ba9 /* retained 0x208aa0 */
proj_sf0_0_208aa4:
.word 0x0182602d /* retained 0x208aa4 */
proj_sf0_0_208aa8:
.word 0x01a3682d /* retained 0x208aa8 */
proj_sf0_0_208aac:
j np_gte_screen_proj_sf0_0
proj_sf0_0_208ab0:
dsra $12,$12,16
proj_sf0_0_208ab4:
.word 0x3c0e8000 /* retained 0x208ab4 */
proj_sf0_0_208ab8:
.word 0x2402fc00 /* retained 0x208ab8 */
proj_sf0_0_208abc:
.word 0x240303ff /* retained 0x208abc */
proj_sf0_0_208ac0:
.word 0x35ce4000 /* retained 0x208ac0 */
proj_sf0_0_208ac4:
.word 0x0182782a /* retained 0x208ac4 */
proj_sf0_0_208ac8:
.word 0x004f600b /* retained 0x208ac8 */
proj_sf0_0_208acc:
.word 0x01cf780b /* retained 0x208acc */
proj_sf0_0_208ad0:
.word 0x002f0825 /* retained 0x208ad0 */
proj_sf0_0_208ad4:
.word 0x006c782a /* retained 0x208ad4 */
proj_sf0_0_208ad8:
.word 0x006f600b /* retained 0x208ad8 */
proj_sf0_0_208adc:
.word 0x01cf780b /* retained 0x208adc */
proj_sf0_0_208ae0:
.word 0x002f0825 /* retained 0x208ae0 */
proj_sf0_0_208ae4:
.word 0x25cee000 /* retained 0x208ae4 */
proj_sf0_0_208ae8:
.word 0x01a2782a /* retained 0x208ae8 */
proj_sf0_0_208aec:
.word 0x004f680b /* retained 0x208aec */
proj_sf0_0_208af0:
.word 0x01cf780b /* retained 0x208af0 */
proj_sf0_0_208af4:
.word 0x002f0825 /* retained 0x208af4 */
proj_sf0_0_208af8:
.word 0x006d782a /* retained 0x208af8 */
proj_sf0_0_208afc:
.word 0x006f680b /* retained 0x208afc */
proj_sf0_0_208b00:
.word 0x01cf780b /* retained 0x208b00 */
proj_sf0_0_208b04:
.word 0x002f0825 /* retained 0x208b04 */
proj_sf0_0_208b08:
.word 0xa4ac0030 /* retained 0x208b08 */
proj_sf0_0_208b0c:
.word 0xa4ad0032 /* retained 0x208b0c */
proj_sf0_0_208b10:
.word 0x2739ffff /* retained 0x208b10 */
proj_sf0_0_208b14:
.word 0x24a50004 /* retained 0x208b14 */
proj_sf0_0_208b18:
.word 0x17200000 | (((proj_sf0_0_208900 - . - 4) / 4) & 0xffff)
proj_sf0_0_208b1c:
.word 0x24c60008 /* retained 0x208b1c */
proj_sf0_0_208b20:
.word 0x848200ec /* retained 0x208b20 */
proj_sf0_0_208b24:
lw $v1,240($a0)
proj_sf0_0_208b28:
bnez $v0,np_gte_depth_nonzero
proj_sf0_0_208b2c:
nop
proj_sf0_0_208b30:
sw $v1,96($a0)
proj_sf0_0_208b34:
dsra $11,$v1,12
proj_sf0_0_208b38:
.word 0x24031000 /* retained 0x208b38 */
proj_sf0_0_208b3c:
.word 0x0160702a /* retained 0x208b3c */
proj_sf0_0_208b40:
.word 0x006b782a /* retained 0x208b40 */
proj_sf0_0_208b44:
.word 0x000e580b /* retained 0x208b44 */
proj_sf0_0_208b48:
.word 0x006f580b /* retained 0x208b48 */
proj_sf0_0_208b4c:
.word 0x01cf7025 /* retained 0x208b4c */
proj_sf0_0_208b50:
.word 0x000e7300 /* retained 0x208b50 */
proj_sf0_0_208b54:
.word 0x002e0825 /* retained 0x208b54 */
proj_sf0_0_208b58:
.word 0xac8b0020 /* retained 0x208b58 */
proj_sf0_0_208b5c:
.word 0x03e00008 /* retained 0x208b5c */
proj_sf0_0_208b60:
.word 0xac8100fc /* retained 0x208b60 */
proj_sf0_0_208b64:
.word 0x00000000 /* retained 0x208b64 */
proj_sf0_0_208b68:
.globl np_gte_rtpt_sf0_lm0
np_gte_rtpt_sf0_lm0:
li $25,3
proj_sf0_0_208b6c:
.word 0x9482004c /* retained 0x208b6c */
proj_sf0_0_208b70:
.word 0x0080282d /* retained 0x208b70 */
proj_sf0_0_208b74:
.word 0x10000000 | (((proj_sf0_0_2088f8 - . - 4) / 4) & 0xffff)
proj_sf0_0_208b78:
.word 0xa4820040 /* retained 0x208b78 */
np_gte_sf0_zero_0:
    bnez $11,zero_nonzero_0
    nop
    lui $11,1
    ori $11,$11,0xffff
    lui $v0,0x8002
    or $at,$at,$v0
    j proj_sf0_0_208a8c
    nop
zero_nonzero_0:
    .word 0x01ab001b
    mflo $11
    j proj_sf0_0_208a8c
    nop
.globl np_gte_rtps_sf0_lm1
np_gte_rtps_sf0_lm1:
proj_sf0_1_2088c8:
.word 0x8c820034 /* retained 0x2088c8 */
proj_sf0_1_2088cc:
.word 0x8c830038 /* retained 0x2088cc */
proj_sf0_1_2088d0:
.word 0x94880044 /* retained 0x2088d0 */
proj_sf0_1_2088d4:
.word 0x94890048 /* retained 0x2088d4 */
proj_sf0_1_2088d8:
.word 0x948a004c /* retained 0x2088d8 */
proj_sf0_1_2088dc:
.word 0xac820030 /* retained 0x2088dc */
proj_sf0_1_2088e0:
.word 0xac830034 /* retained 0x2088e0 */
proj_sf0_1_2088e4:
.word 0xa4880040 /* retained 0x2088e4 */
proj_sf0_1_2088e8:
.word 0xa4890044 /* retained 0x2088e8 */
proj_sf0_1_2088ec:
.word 0xa48a0048 /* retained 0x2088ec */
proj_sf0_1_2088f0:
.word 0x24190001 /* retained 0x2088f0 */
proj_sf0_1_2088f4:
.word 0x24850008 /* retained 0x2088f4 */
proj_sf0_1_2088f8:
.word 0x0000082d /* retained 0x2088f8 */
proj_sf0_1_2088fc:
.word 0x0080302d /* retained 0x2088fc */
proj_sf0_1_208900:
.word 0x84ce0000 /* retained 0x208900 */
proj_sf0_1_208904:
.word 0x84cf0002 /* retained 0x208904 */
proj_sf0_1_208908:
.word 0x84d80004 /* retained 0x208908 */
proj_sf0_1_20890c:
.word 0x848b0080 /* retained 0x20890c */
proj_sf0_1_208910:
.word 0x848c0082 /* retained 0x208910 */
proj_sf0_1_208914:
.word 0x016e5818 /* retained 0x208914 */
proj_sf0_1_208918:
.word 0x718f6018 /* retained 0x208918 */
proj_sf0_1_20891c:
.word 0x8c880094 /* retained 0x20891c */
proj_sf0_1_208920:
.word 0x848d0084 /* retained 0x208920 */
proj_sf0_1_208924:
.word 0x00084338 /* retained 0x208924 */
proj_sf0_1_208928:
.word 0x01b86818 /* retained 0x208928 */
proj_sf0_1_20892c:
.word 0x010b402d /* retained 0x20892c */
proj_sf0_1_208930:
.word 0x010c402d /* retained 0x208930 */
proj_sf0_1_208934:
.word 0x010d402d /* retained 0x208934 */
proj_sf0_1_208938:
.word 0x848b0086 /* retained 0x208938 */
proj_sf0_1_20893c:
.word 0x848c0088 /* retained 0x20893c */
proj_sf0_1_208940:
.word 0x016e5818 /* retained 0x208940 */
proj_sf0_1_208944:
.word 0x718f6018 /* retained 0x208944 */
proj_sf0_1_208948:
.word 0x8c890098 /* retained 0x208948 */
proj_sf0_1_20894c:
.word 0x848d008a /* retained 0x20894c */
proj_sf0_1_208950:
.word 0x00094b38 /* retained 0x208950 */
proj_sf0_1_208954:
.word 0x01b86818 /* retained 0x208954 */
proj_sf0_1_208958:
.word 0x012b482d /* retained 0x208958 */
proj_sf0_1_20895c:
.word 0x012c482d /* retained 0x20895c */
proj_sf0_1_208960:
.word 0x012d482d /* retained 0x208960 */
proj_sf0_1_208964:
.word 0x848b008c /* retained 0x208964 */
proj_sf0_1_208968:
.word 0x848c008e /* retained 0x208968 */
proj_sf0_1_20896c:
.word 0x016e5818 /* retained 0x20896c */
proj_sf0_1_208970:
.word 0x718f1818 /* retained 0x208970 */
proj_sf0_1_208974:
.word 0x8c8a009c /* retained 0x208974 */
proj_sf0_1_208978:
.word 0x848d0090 /* retained 0x208978 */
proj_sf0_1_20897c:
.word 0x000a5338 /* retained 0x20897c */
proj_sf0_1_208980:
.word 0x01b86818 /* retained 0x208980 */
proj_sf0_1_208984:
.word 0x014b502d /* retained 0x208984 */
proj_sf0_1_208988:
.word 0x0143502d /* retained 0x208988 */
proj_sf0_1_20898c:
.word 0x014d502d /* retained 0x20898c */
proj_sf0_1_208990:
sll $8,$8,0
proj_sf0_1_208994:
sll $9,$9,0
proj_sf0_1_208998:
dsll $11,$10,20
proj_sf0_1_20899c:
dsra32 $11,$11,0
proj_sf0_1_2089a0:
sll $10,$10,0
proj_sf0_1_2089a4:
nop
proj_sf0_1_2089a8:
.word 0xac880064 /* retained 0x2089a8 */
proj_sf0_1_2089ac:
.word 0xac890068 /* retained 0x2089ac */
proj_sf0_1_2089b0:
.word 0xac8a006c /* retained 0x2089b0 */
proj_sf0_1_2089b4:
nop
proj_sf0_1_2089b8:
move $v0,$zero
proj_sf0_1_2089bc:
.word 0x24037fff /* retained 0x2089bc */
proj_sf0_1_2089c0:
.word 0x3c0e8100 /* retained 0x2089c0 */
proj_sf0_1_2089c4:
.word 0x0102782a /* retained 0x2089c4 */
proj_sf0_1_2089c8:
.word 0x004f400b /* retained 0x2089c8 */
proj_sf0_1_2089cc:
.word 0x01cf780b /* retained 0x2089cc */
proj_sf0_1_2089d0:
.word 0x002f0825 /* retained 0x2089d0 */
proj_sf0_1_2089d4:
.word 0x0068782a /* retained 0x2089d4 */
proj_sf0_1_2089d8:
.word 0x006f400b /* retained 0x2089d8 */
proj_sf0_1_2089dc:
.word 0x01cf780b /* retained 0x2089dc */
proj_sf0_1_2089e0:
.word 0x002f0825 /* retained 0x2089e0 */
proj_sf0_1_2089e4:
.word 0x3c0e8080 /* retained 0x2089e4 */
proj_sf0_1_2089e8:
.word 0x0122782a /* retained 0x2089e8 */
proj_sf0_1_2089ec:
.word 0x004f480b /* retained 0x2089ec */
proj_sf0_1_2089f0:
.word 0x01cf780b /* retained 0x2089f0 */
proj_sf0_1_2089f4:
.word 0x002f0825 /* retained 0x2089f4 */
proj_sf0_1_2089f8:
.word 0x0069782a /* retained 0x2089f8 */
proj_sf0_1_2089fc:
.word 0x006f480b /* retained 0x2089fc */
proj_sf0_1_208a00:
.word 0x01cf780b /* retained 0x208a00 */
proj_sf0_1_208a04:
.word 0x002f0825 /* retained 0x208a04 */
proj_sf0_1_208a08:
li $v0,-32768
proj_sf0_1_208a0c:
slt $15,$11,$v0
proj_sf0_1_208a10:
slt $14,$v1,$11
proj_sf0_1_208a14:
or $15,$15,$14
proj_sf0_1_208a18:
sll $15,$15,22
proj_sf0_1_208a1c:
or $at,$at,$15
proj_sf0_1_208a20:
pmaxw $10,$10,$zero
proj_sf0_1_208a24:
pminw $10,$10,$v1
proj_sf0_1_208a28:
nop
proj_sf0_1_208a2c:
.word 0xac880024 /* retained 0x208a2c */
proj_sf0_1_208a30:
.word 0xac890028 /* retained 0x208a30 */
proj_sf0_1_208a34:
.word 0xac8a002c /* retained 0x208a34 */
proj_sf0_1_208a38:
.word 0x3402ffff /* retained 0x208a38 */
proj_sf0_1_208a3c:
.word 0x3c038006 /* retained 0x208a3c */
proj_sf0_1_208a40:
.word 0x0160702a /* retained 0x208a40 */
proj_sf0_1_208a44:
.word 0x000e580b /* retained 0x208a44 */
proj_sf0_1_208a48:
.word 0x006e700b /* retained 0x208a48 */
proj_sf0_1_208a4c:
.word 0x002e0825 /* retained 0x208a4c */
proj_sf0_1_208a50:
.word 0x004b702a /* retained 0x208a50 */
proj_sf0_1_208a54:
.word 0x004e580b /* retained 0x208a54 */
proj_sf0_1_208a58:
.word 0x006e700b /* retained 0x208a58 */
proj_sf0_1_208a5c:
.word 0x002e0825 /* retained 0x208a5c */
proj_sf0_1_208a60:
.word 0xa4ab0044 /* retained 0x208a60 */
proj_sf0_1_208a64:
lhu $12,232($a0)
proj_sf0_1_208a68:
sll $v0,$11,1
proj_sf0_1_208a6c:
sltu $v0,$12,$v0
proj_sf0_1_208a70:
bnez $v0,proj_sf0_1_208a80
proj_sf0_1_208a74:
sll $13,$12,16
proj_sf0_1_208a78:
j np_gte_sf0_zero_1+8
proj_sf0_1_208a7c:
nop
proj_sf0_1_208a80:
.word 0x01ab001b /* DIVU only when H < 2*SZ3; SZ3 is nonzero. */
proj_sf0_1_208a84:
mflo $11
proj_sf0_1_208a88:
nop
proj_sf0_1_208a8c:
.word 0x010b0018 /* retained 0x208a8c */
proj_sf0_1_208a90:
.word 0x712b0018 /* retained 0x208a90 */
proj_sf0_1_208a94:
.word 0x8c8c00e0 /* retained 0x208a94 */
proj_sf0_1_208a98:
.word 0x8c8d00e4 /* retained 0x208a98 */
proj_sf0_1_208a9c:
.word 0x70001030 /* retained 0x208a9c */
proj_sf0_1_208aa0:
.word 0x70421ba9 /* retained 0x208aa0 */
proj_sf0_1_208aa4:
.word 0x0182602d /* retained 0x208aa4 */
proj_sf0_1_208aa8:
.word 0x01a3682d /* retained 0x208aa8 */
proj_sf0_1_208aac:
j np_gte_screen_proj_sf0_1
proj_sf0_1_208ab0:
dsra $12,$12,16
proj_sf0_1_208ab4:
.word 0x3c0e8000 /* retained 0x208ab4 */
proj_sf0_1_208ab8:
.word 0x2402fc00 /* retained 0x208ab8 */
proj_sf0_1_208abc:
.word 0x240303ff /* retained 0x208abc */
proj_sf0_1_208ac0:
.word 0x35ce4000 /* retained 0x208ac0 */
proj_sf0_1_208ac4:
.word 0x0182782a /* retained 0x208ac4 */
proj_sf0_1_208ac8:
.word 0x004f600b /* retained 0x208ac8 */
proj_sf0_1_208acc:
.word 0x01cf780b /* retained 0x208acc */
proj_sf0_1_208ad0:
.word 0x002f0825 /* retained 0x208ad0 */
proj_sf0_1_208ad4:
.word 0x006c782a /* retained 0x208ad4 */
proj_sf0_1_208ad8:
.word 0x006f600b /* retained 0x208ad8 */
proj_sf0_1_208adc:
.word 0x01cf780b /* retained 0x208adc */
proj_sf0_1_208ae0:
.word 0x002f0825 /* retained 0x208ae0 */
proj_sf0_1_208ae4:
.word 0x25cee000 /* retained 0x208ae4 */
proj_sf0_1_208ae8:
.word 0x01a2782a /* retained 0x208ae8 */
proj_sf0_1_208aec:
.word 0x004f680b /* retained 0x208aec */
proj_sf0_1_208af0:
.word 0x01cf780b /* retained 0x208af0 */
proj_sf0_1_208af4:
.word 0x002f0825 /* retained 0x208af4 */
proj_sf0_1_208af8:
.word 0x006d782a /* retained 0x208af8 */
proj_sf0_1_208afc:
.word 0x006f680b /* retained 0x208afc */
proj_sf0_1_208b00:
.word 0x01cf780b /* retained 0x208b00 */
proj_sf0_1_208b04:
.word 0x002f0825 /* retained 0x208b04 */
proj_sf0_1_208b08:
.word 0xa4ac0030 /* retained 0x208b08 */
proj_sf0_1_208b0c:
.word 0xa4ad0032 /* retained 0x208b0c */
proj_sf0_1_208b10:
.word 0x2739ffff /* retained 0x208b10 */
proj_sf0_1_208b14:
.word 0x24a50004 /* retained 0x208b14 */
proj_sf0_1_208b18:
.word 0x17200000 | (((proj_sf0_1_208900 - . - 4) / 4) & 0xffff)
proj_sf0_1_208b1c:
.word 0x24c60008 /* retained 0x208b1c */
proj_sf0_1_208b20:
.word 0x848200ec /* retained 0x208b20 */
proj_sf0_1_208b24:
lw $v1,240($a0)
proj_sf0_1_208b28:
bnez $v0,np_gte_depth_nonzero
proj_sf0_1_208b2c:
nop
proj_sf0_1_208b30:
sw $v1,96($a0)
proj_sf0_1_208b34:
dsra $11,$v1,12
proj_sf0_1_208b38:
.word 0x24031000 /* retained 0x208b38 */
proj_sf0_1_208b3c:
.word 0x0160702a /* retained 0x208b3c */
proj_sf0_1_208b40:
.word 0x006b782a /* retained 0x208b40 */
proj_sf0_1_208b44:
.word 0x000e580b /* retained 0x208b44 */
proj_sf0_1_208b48:
.word 0x006f580b /* retained 0x208b48 */
proj_sf0_1_208b4c:
.word 0x01cf7025 /* retained 0x208b4c */
proj_sf0_1_208b50:
.word 0x000e7300 /* retained 0x208b50 */
proj_sf0_1_208b54:
.word 0x002e0825 /* retained 0x208b54 */
proj_sf0_1_208b58:
.word 0xac8b0020 /* retained 0x208b58 */
proj_sf0_1_208b5c:
.word 0x03e00008 /* retained 0x208b5c */
proj_sf0_1_208b60:
.word 0xac8100fc /* retained 0x208b60 */
proj_sf0_1_208b64:
.word 0x00000000 /* retained 0x208b64 */
proj_sf0_1_208b68:
.globl np_gte_rtpt_sf0_lm1
np_gte_rtpt_sf0_lm1:
li $25,3
proj_sf0_1_208b6c:
.word 0x9482004c /* retained 0x208b6c */
proj_sf0_1_208b70:
.word 0x0080282d /* retained 0x208b70 */
proj_sf0_1_208b74:
.word 0x10000000 | (((proj_sf0_1_2088f8 - . - 4) / 4) & 0xffff)
proj_sf0_1_208b78:
.word 0xa4820040 /* retained 0x208b78 */
np_gte_sf0_zero_1:
    bnez $11,zero_nonzero_1
    nop
    lui $11,1
    ori $11,$11,0xffff
    lui $v0,0x8002
    or $at,$at,$v0
    j proj_sf0_1_208a8c
    nop
zero_nonzero_1:
    .word 0x01ab001b
    mflo $11
    j proj_sf0_1_208a8c
    nop
.globl np_gte_near_template,np_gte_near_template_end
np_gte_near_template:
    lhu $12,232($a0)
    sll $v0,$11,1
    sltu $v0,$12,$v0
    .word 0x14400003 /* BNEZ v0,+3; template is copied verbatim. */
    sll $13,$12,16
    j np_gte_zero_depth+8
    nop
    .word 0x01ab001b
    mflo $11
    nop
np_gte_near_template_end:

.balign 64
.globl np_gte_hot_rtps
np_gte_hot_rtps:
hot_2088c8:
.word 0x8c820034
hot_2088cc:
.word 0x8c830038
hot_2088d0:
.word 0x94880044
hot_2088d4:
.word 0x94890048
hot_2088d8:
.word 0x948a004c
hot_2088dc:
.word 0xac820030
hot_2088e0:
.word 0xac830034
hot_2088e4:
.word 0xa4880040
hot_2088e8:
.word 0xa4890044
hot_2088ec:
.word 0xa48a0048
hot_2088f0:
.word 0x24190001
hot_2088f4:
.word 0x24850008
hot_2088f8:
.word 0x0000082d
hot_2088fc:
.word 0x0080302d
hot_208900:
.word 0x84ce0000
hot_208904:
.word 0x84cf0002
hot_208908:
.word 0x84d80004
hot_20890c:
.word 0x848b0080
hot_208910:
.word 0x848c0082
hot_208914:
.word 0x016e5818
hot_208918:
.word 0x718f6018
hot_20891c:
.word 0x8c880094
hot_208920:
.word 0x848d0084
hot_208924:
.word 0x00084338
hot_208928:
.word 0x01b86818
hot_20892c:
.word 0x010b402d
hot_208930:
.word 0x010c402d
hot_208934:
.word 0x010d402d
hot_208938:
.word 0x848b0086
hot_20893c:
.word 0x848c0088
hot_208940:
.word 0x016e5818
hot_208944:
.word 0x718f6018
hot_208948:
.word 0x8c890098
hot_20894c:
.word 0x848d008a
hot_208950:
.word 0x00094b38
hot_208954:
.word 0x01b86818
hot_208958:
.word 0x012b482d
hot_20895c:
.word 0x012c482d
hot_208960:
.word 0x012d482d
hot_208964:
.word 0x848b008c
hot_208968:
.word 0x848c008e
hot_20896c:
.word 0x016e5818
hot_208970:
.word 0x718f1818
hot_208974:
.word 0x8c8a009c
hot_208978:
.word 0x848d0090
hot_20897c:
.word 0x000a5338
hot_208980:
.word 0x01b86818
hot_208984:
.word 0x014b502d
hot_208988:
.word 0x0143502d
hot_20898c:
.word 0x014d502d
hot_208990:
.word 0x00084538
hot_208994:
.word 0x00094d38
hot_208998:
.word 0x000a5538
hot_20899c:
.word 0x0008403f
hot_2089a0:
.word 0x0009483f
hot_2089a4:
.word 0x000a503f
hot_2089a8:
.word 0xac880064
hot_2089ac:
.word 0xac890068
hot_2089b0:
.word 0xac8a006c
hot_2089b4:
.word 0x0140582d
hot_2089b8:
.word 0x24028000
hot_2089bc:
.word 0x24037fff
hot_2089c0:
.word 0x3c0e8100
hot_2089c4:
.word 0x0102782a
hot_2089c8:
.word 0x004f400b
hot_2089cc:
.word 0x01cf780b
hot_2089d0:
.word 0x002f0825
hot_2089d4:
.word 0x0068782a
hot_2089d8:
.word 0x006f400b
hot_2089dc:
.word 0x01cf780b
hot_2089e0:
.word 0x002f0825
hot_2089e4:
.word 0x3c0e8080
hot_2089e8:
.word 0x0122782a
hot_2089ec:
.word 0x004f480b
hot_2089f0:
.word 0x01cf780b
hot_2089f4:
.word 0x002f0825
hot_2089f8:
.word 0x0069782a
hot_2089fc:
.word 0x006f480b
hot_208a00:
.word 0x01cf780b
hot_208a04:
.word 0x002f0825
hot_208a08:
.word 0x3c0e0040
hot_208a0c:
.word 0x0142782a
hot_208a10:
.word 0x004f500b
hot_208a14:
.word 0x01cf780b
hot_208a18:
.word 0x002f0825
hot_208a1c:
.word 0x006a782a
hot_208a20:
.word 0x006f500b
hot_208a24:
.word 0x01cf780b
hot_208a28:
.word 0x002f0825
hot_208a2c:
.word 0xac880024
hot_208a30:
.word 0xac890028
hot_208a34:
.word 0xac8a002c
hot_208a38:
.word 0x3402ffff
hot_208a3c:
.word 0x3c038006
hot_208a40:
.word 0x0160702a
hot_208a44:
.word 0x000e580b
hot_208a48:
.word 0x006e700b
hot_208a4c:
.word 0x002e0825
hot_208a50:
.word 0x004b702a
hot_208a54:
.word 0x004e580b
hot_208a58:
.word 0x006e700b
hot_208a5c:
.word 0x002e0825
hot_208a60:
.word 0xa4ab0044
hot_208a64:
lhu $12,232($a0)
hot_208a68:
sll $v0,$11,1
hot_208a6c:
sltu $v0,$12,$v0
hot_208a70:
beqz $v0,np_gte_hot_overflow
hot_208a74:
sll $13,$12,16
hot_208a78:
hot_208a7c:
hot_208a80:
.word 0x01ab001b
hot_208a84:
/* OFX is independent of the quotient. Issue its existing load while the
 * native division is running; leave the second offset load shared below. */
lw $12,224($a0)
hot_208a88:
mflo $11
hot_208a8c:
.word 0x010b0018
hot_208a90:
.word 0x712b0018
hot_208a94: /* OFX was already loaded. */
hot_208a98:
.word 0x8c8d00e4
hot_208a9c:
.word 0x70001030
hot_208aa0:
.word 0x70421ba9
hot_208aa4:
.word 0x0182602d
hot_208aa8:
.word 0x01a3682d
hot_208aac:
j np_gte_screen_hot
hot_208ab0:
dsra $12,$12,16
hot_208ab4:
.word 0x3c0e8000
hot_208ab8:
.word 0x2402fc00
hot_208abc:
.word 0x240303ff
hot_208ac0:
.word 0x35ce4000
hot_208ac4:
.word 0x0182782a
hot_208ac8:
.word 0x004f600b
hot_208acc:
.word 0x01cf780b
hot_208ad0:
.word 0x002f0825
hot_208ad4:
.word 0x006c782a
hot_208ad8:
.word 0x006f600b
hot_208adc:
.word 0x01cf780b
hot_208ae0:
.word 0x002f0825
hot_208ae4:
.word 0x25cee000
hot_208ae8:
.word 0x01a2782a
hot_208aec:
.word 0x004f680b
hot_208af0:
.word 0x01cf780b
hot_208af4:
.word 0x002f0825
hot_208af8:
.word 0x006d782a
hot_208afc:
.word 0x006f680b
hot_208b00:
.word 0x01cf780b
hot_208b04:
.word 0x002f0825
hot_208b08:
.word 0xa4ac0030
hot_208b0c:
.word 0xa4ad0032
hot_208b10:
.word 0x2739ffff
hot_208b14:
.word 0x24a50004
hot_208b18:
.word 0x17200000 | (((hot_208900 - . - 4) / 4) & 0xffff)
hot_208b1c:
.word 0x24c60008
hot_208b20:
.word 0x848200ec
hot_208b24:
lw $v1,240($a0)
hot_208b28:
bnez $v0,np_gte_depth_nonzero
hot_208b2c:
nop
hot_208b30:
sw $v1,96($a0)
hot_208b34:
dsra $11,$v1,12
hot_208b38:
.word 0x24031000
hot_208b3c:
.word 0x0160702a
hot_208b40:
.word 0x006b782a
hot_208b44:
.word 0x000e580b
hot_208b48:
.word 0x006f580b
hot_208b4c:
.word 0x01cf7025
hot_208b50:
.word 0x000e7300
hot_208b54:
.word 0x002e0825
hot_208b58:
.word 0xac8b0020
hot_208b5c:
.word 0x03e00008
hot_208b60:
.word 0xac8100fc
hot_208b64:
.word 0x00000000
.globl np_gte_hot_rtpt
np_gte_hot_rtpt:
hot_208b68:
.word 0x24190003
hot_208b6c:
.word 0x9482004c
hot_208b70:
.word 0x0080282d
hot_208b74:
.word 0x10000000 | (((hot_2088f8 - . - 4) / 4) & 0xffff)
hot_208b78:
.word 0xa4820040
np_gte_hot_overflow:
lui $11,1
ori $11,$11,0xffff
lui $v0,0x8002
or $at,$at,$v0
j hot_208a8c
/* Saturation bypasses division and supplies OFX in its old empty delay slot. */
lw $12,224($a0)

.balign 16
.globl np_gte_compile_nclip
np_gte_compile_nclip:
la $a0,np_gte_nclip
j 0x206c5c
nop
.globl np_gte_nclip
np_gte_nclip:
.word 0x84880030
.word 0x84890032
.word 0x848a0034
.word 0x848b0036
.word 0x848c0038
.word 0x848d003a
.word 0x010b0018
.word 0x710d0018
.word 0x714d0000
.word 0x71490020
.word 0x71897000
.word 0x718b7820
pmfhl.lw $14
pcpyud $15,$14,$14
dsubu $24,$14,$15
sw $24,96($a0)
dsra $v0,$24,31
dsra32 $v1,$24,31
xor $v0,$v0,$v1
andi $v1,$v1,0x8000
lui $at,0x8001
subu $at,$at,$v1
movz $at,$zero,$v0
jr $ra
sw $at,252($a0)
.globl np_gte_screen_native
np_gte_screen_native:
dsra $13,$13,16
li $v0,-1024
li $v1,1023
slt $15,$12,$v0
bnez $15,np_gte_screen_native_xneg
slt $14,$v1,$12
bnez $14,np_gte_screen_native_xpos
slt $15,$13,$v0
np_gte_screen_native_y:
bnez $15,np_gte_screen_native_yneg
slt $14,$v1,$13
bnez $14,np_gte_screen_native_ypos
sh $12,48($a1)
np_gte_screen_native_store_y:
j 0x208b10
sh $13,50($a1)
np_gte_screen_native_xneg:
slti $15,$12,-32768
sll $15,$15,15
lui $14,0x8000
ori $14,$14,16384
or $15,$15,$14
or $at,$at,$15
move $12,$v0
b np_gte_screen_native_y
slt $15,$13,$v0
np_gte_screen_native_xpos:
ori $15,$zero,32767
slt $15,$15,$12
sll $15,$15,16
lui $14,0x8000
ori $14,$14,16384
or $15,$15,$14
or $at,$at,$15
move $12,$v1
b np_gte_screen_native_y
slt $15,$13,$v0
np_gte_screen_native_yneg:
slti $15,$13,-32768
sll $15,$15,15
lui $14,0x8000
ori $14,$14,8192
or $15,$15,$14
or $at,$at,$15
move $13,$v0
b np_gte_screen_native_store_y
sh $12,48($a1)
np_gte_screen_native_ypos:
ori $15,$zero,32767
slt $15,$15,$13
sll $15,$15,16
lui $14,0x8000
ori $14,$14,8192
or $15,$15,$14
or $at,$at,$15
move $13,$v1
b np_gte_screen_native_store_y
nop
.globl np_gte_screen_proj
np_gte_screen_proj:
dsra $13,$13,16
li $v0,-1024
li $v1,1023
slt $15,$12,$v0
bnez $15,np_gte_screen_proj_xneg
slt $14,$v1,$12
bnez $14,np_gte_screen_proj_xpos
slt $15,$13,$v0
np_gte_screen_proj_y:
bnez $15,np_gte_screen_proj_yneg
slt $14,$v1,$13
bnez $14,np_gte_screen_proj_ypos
sh $12,48($a1)
np_gte_screen_proj_store_y:
j proj_208b10
sh $13,50($a1)
np_gte_screen_proj_xneg:
slti $15,$12,-32768
sll $15,$15,15
lui $14,0x8000
ori $14,$14,16384
or $15,$15,$14
or $at,$at,$15
move $12,$v0
b np_gte_screen_proj_y
slt $15,$13,$v0
np_gte_screen_proj_xpos:
ori $15,$zero,32767
slt $15,$15,$12
sll $15,$15,16
lui $14,0x8000
ori $14,$14,16384
or $15,$15,$14
or $at,$at,$15
move $12,$v1
b np_gte_screen_proj_y
slt $15,$13,$v0
np_gte_screen_proj_yneg:
slti $15,$13,-32768
sll $15,$15,15
lui $14,0x8000
ori $14,$14,8192
or $15,$15,$14
or $at,$at,$15
move $13,$v0
b np_gte_screen_proj_store_y
sh $12,48($a1)
np_gte_screen_proj_ypos:
ori $15,$zero,32767
slt $15,$15,$13
sll $15,$15,16
lui $14,0x8000
ori $14,$14,8192
or $15,$15,$14
or $at,$at,$15
move $13,$v1
b np_gte_screen_proj_store_y
nop
.globl np_gte_screen_proj_sf0_0
np_gte_screen_proj_sf0_0:
dsra $13,$13,16
li $v0,-1024
li $v1,1023
slt $15,$12,$v0
bnez $15,np_gte_screen_proj_sf0_0_xneg
slt $14,$v1,$12
bnez $14,np_gte_screen_proj_sf0_0_xpos
slt $15,$13,$v0
np_gte_screen_proj_sf0_0_y:
bnez $15,np_gte_screen_proj_sf0_0_yneg
slt $14,$v1,$13
bnez $14,np_gte_screen_proj_sf0_0_ypos
sh $12,48($a1)
np_gte_screen_proj_sf0_0_store_y:
j proj_sf0_0_208b10
sh $13,50($a1)
np_gte_screen_proj_sf0_0_xneg:
slti $15,$12,-32768
sll $15,$15,15
lui $14,0x8000
ori $14,$14,16384
or $15,$15,$14
or $at,$at,$15
move $12,$v0
b np_gte_screen_proj_sf0_0_y
slt $15,$13,$v0
np_gte_screen_proj_sf0_0_xpos:
ori $15,$zero,32767
slt $15,$15,$12
sll $15,$15,16
lui $14,0x8000
ori $14,$14,16384
or $15,$15,$14
or $at,$at,$15
move $12,$v1
b np_gte_screen_proj_sf0_0_y
slt $15,$13,$v0
np_gte_screen_proj_sf0_0_yneg:
slti $15,$13,-32768
sll $15,$15,15
lui $14,0x8000
ori $14,$14,8192
or $15,$15,$14
or $at,$at,$15
move $13,$v0
b np_gte_screen_proj_sf0_0_store_y
sh $12,48($a1)
np_gte_screen_proj_sf0_0_ypos:
ori $15,$zero,32767
slt $15,$15,$13
sll $15,$15,16
lui $14,0x8000
ori $14,$14,8192
or $15,$15,$14
or $at,$at,$15
move $13,$v1
b np_gte_screen_proj_sf0_0_store_y
nop
.globl np_gte_screen_proj_sf0_1
np_gte_screen_proj_sf0_1:
dsra $13,$13,16
li $v0,-1024
li $v1,1023
slt $15,$12,$v0
bnez $15,np_gte_screen_proj_sf0_1_xneg
slt $14,$v1,$12
bnez $14,np_gte_screen_proj_sf0_1_xpos
slt $15,$13,$v0
np_gte_screen_proj_sf0_1_y:
bnez $15,np_gte_screen_proj_sf0_1_yneg
slt $14,$v1,$13
bnez $14,np_gte_screen_proj_sf0_1_ypos
sh $12,48($a1)
np_gte_screen_proj_sf0_1_store_y:
j proj_sf0_1_208b10
sh $13,50($a1)
np_gte_screen_proj_sf0_1_xneg:
slti $15,$12,-32768
sll $15,$15,15
lui $14,0x8000
ori $14,$14,16384
or $15,$15,$14
or $at,$at,$15
move $12,$v0
b np_gte_screen_proj_sf0_1_y
slt $15,$13,$v0
np_gte_screen_proj_sf0_1_xpos:
ori $15,$zero,32767
slt $15,$15,$12
sll $15,$15,16
lui $14,0x8000
ori $14,$14,16384
or $15,$15,$14
or $at,$at,$15
move $12,$v1
b np_gte_screen_proj_sf0_1_y
slt $15,$13,$v0
np_gte_screen_proj_sf0_1_yneg:
slti $15,$13,-32768
sll $15,$15,15
lui $14,0x8000
ori $14,$14,8192
or $15,$15,$14
or $at,$at,$15
move $13,$v0
b np_gte_screen_proj_sf0_1_store_y
sh $12,48($a1)
np_gte_screen_proj_sf0_1_ypos:
ori $15,$zero,32767
slt $15,$15,$13
sll $15,$15,16
lui $14,0x8000
ori $14,$14,8192
or $15,$15,$14
or $at,$at,$15
move $13,$v1
b np_gte_screen_proj_sf0_1_store_y
nop
.globl np_gte_screen_hot
np_gte_screen_hot:
dsra $13,$13,16
li $v0,-1024
li $v1,1023
slt $15,$12,$v0
bnez $15,np_gte_screen_hot_xneg
slt $14,$v1,$12
bnez $14,np_gte_screen_hot_xpos
slt $15,$13,$v0
np_gte_screen_hot_y:
bnez $15,np_gte_screen_hot_yneg
slt $14,$v1,$13
bnez $14,np_gte_screen_hot_ypos
sh $12,48($a1)
np_gte_screen_hot_store_y:
j hot_208b10
sh $13,50($a1)
np_gte_screen_hot_xneg:
slti $15,$12,-32768
sll $15,$15,15
lui $14,0x8000
ori $14,$14,16384
or $15,$15,$14
or $at,$at,$15
move $12,$v0
b np_gte_screen_hot_y
slt $15,$13,$v0
np_gte_screen_hot_xpos:
ori $15,$zero,32767
slt $15,$15,$12
sll $15,$15,16
lui $14,0x8000
ori $14,$14,16384
or $15,$15,$14
or $at,$at,$15
move $12,$v1
b np_gte_screen_hot_y
slt $15,$13,$v0
np_gte_screen_hot_yneg:
slti $15,$13,-32768
sll $15,$15,15
lui $14,0x8000
ori $14,$14,8192
or $15,$15,$14
or $at,$at,$15
move $13,$v0
b np_gte_screen_hot_store_y
sh $12,48($a1)
np_gte_screen_hot_ypos:
ori $15,$zero,32767
slt $15,$15,$13
sll $15,$15,16
lui $14,0x8000
ori $14,$14,8192
or $15,$15,$14
or $at,$at,$15
move $13,$v1
b np_gte_screen_hot_store_y
nop
.globl np_gte_depth_native
np_gte_depth_native:
lw $v1,240($a0)
bnez $v0,np_gte_depth_nonzero
nop
sw $v1,96($a0)
dsra $11,$v1,12
b np_gte_depth_clamp
nop
.globl np_gte_depth_nonzero
np_gte_depth_nonzero:
mult $v0,$11
pmfhl.lw $11
daddu $11,$11,$v1
sw $11,96($a0)
dsra $v0,$11,31
dsra32 $v1,$11,31
xor $v0,$v0,$v1
andi $v1,$v1,0x8000
lui $14,0x8001
subu $14,$14,$v1
movz $14,$zero,$v0
or $at,$at,$14
dsra $11,$11,12
np_gte_depth_clamp:
.word 0x24031000
.word 0x0160702a
.word 0x006b782a
.word 0x000e580b
.word 0x006f580b
.word 0x01cf7025
.word 0x000e7300
.word 0x002e0825
.word 0xac8b0020
.word 0x03e00008
.word 0xac8100fc

#if NP_GTE_MATRIX_MODES

.balign 16
.globl np_gte_matrix_compile
np_gte_matrix_compile:
li $v0,3
beq $8,$v0,matrix_choose_special
nop
li $v0,2
beq $10,$v0,matrix_choose_far
nop
li $a0,0x209c70
b matrix_choose_done
nop
matrix_choose_special:
li $v0,2
beq $10,$v0,matrix_choose_both
nop
la $a0,np_gte_matrix_entries
b matrix_choose_done
nop
matrix_choose_far:
la $a0,np_gte_far_entries
b matrix_choose_done
nop
matrix_choose_both:
la $a0,np_gte_matrix_far_entries
matrix_choose_done:
sll $v0,$11,5
sll $v1,$a2,4
or $v0,$v0,$v1
addu $a0,$a0,$v0
j 0x206c5c
addiu $s1,$s1,4
.globl np_gte_matrix_entries
np_gte_matrix_entries:
li $at,-32768
li $25,0
j matrix_body
nop
li $at,0
li $25,0
j matrix_body
nop
li $at,-32768
li $25,12
j matrix_body
nop
li $at,0
li $25,12
j matrix_body
nop
matrix_body:
lbu $9,24($a0)
sll $9,$9,4
subu $8,$zero,$9
lh $10,32($a0)
lh $11,132($a0)
move $12,$11
move $13,$11
lh $14,136($a0)
move $15,$14
matrix_209cc8:
.word 0x14c00000 | (((matrix_209ce0 - . - 4) / 4) & 0xffff)
matrix_209ccc:
move $24,$14
matrix_209cd0:
.word 0x84820024
matrix_209cd4:
.word 0x84830028
matrix_209cd8:
.word 0x10000000 | (((matrix_209cec - . - 4) / 4) & 0xffff)
matrix_209cdc:
.word 0x8485002c
matrix_209ce0:
.word 0x84c20000
matrix_209ce4:
.word 0x84c30002
matrix_209ce8:
.word 0x84c50004
matrix_209cec:
.word 0x01020018
matrix_209cf0:
.word 0x71620018
matrix_209cf4:
.word 0x71230000
matrix_209cf8:
.word 0x71830020
matrix_209cfc:
.word 0x71450000
matrix_209d00:
.word 0x71a50020
matrix_209d04:
.word 0x70004030
matrix_209d08:
.word 0x71084ba9
matrix_209d0c:
.word 0x01c20018
matrix_209d10:
.word 0x71e30000
matrix_209d14:
.word 0x73050000
matrix_209d18:
.word 0x0000582d
matrix_209d1c:
.word 0x0000602d
matrix_209d20:
.word 0x70005030
matrix_209d24:
.word 0x00000000
matrix_209d28:
.word 0x10e00000 | (((matrix_209d3c - . - 4) / 4) & 0xffff)
matrix_209d2c:
.word 0x0000682d
matrix_209d30:
.word 0x8ceb0000
matrix_209d34:
.word 0x8cec0004
matrix_209d38:
.word 0x8ced0008
matrix_209d3c:
.word 0x000b5b38
matrix_209d40:
.word 0x000c6338
matrix_209d44:
.word 0x000d6b38
matrix_209d48:
.word 0x010b402c
matrix_209d4c:
.word 0x012c482c
matrix_209d50:
.word 0x13200000 | (((matrix_209d64 - . - 4) / 4) & 0xffff)
matrix_209d54:
.word 0x014d502c
matrix_209d58:
.word 0x0008433b
matrix_209d5c:
.word 0x00094b3b
matrix_209d60:
.word 0x000a533b
matrix_209d64:
sll $8,$8,0
sll $9,$9,0
sll $10,$10,0
sw $8,100($a0)
matrix_209d68:
.word 0xac890068
matrix_209d6c:
.word 0xac8a006c
matrix_209d70:
.word 0x0020102d
matrix_209d74:
.word 0x24037fff
matrix_209d78:
.word 0x0000082d
matrix_209d7c:
.word 0x0102282a
matrix_209d80:
.word 0x0068302a
matrix_209d84:
.word 0x0045400b
matrix_209d88:
.word 0x0066400b
matrix_209d8c:
.word 0x00a62825
matrix_209d90:
.word 0x00052e00
matrix_209d94:
.word 0x00250825
matrix_209d98:
.word 0x0122282a
matrix_209d9c:
.word 0x0069302a
matrix_209da0:
.word 0x0045480b
matrix_209da4:
.word 0x0066480b
matrix_209da8:
.word 0x00a62825
matrix_209dac:
.word 0x00052dc0
matrix_209db0:
.word 0x00250825
matrix_209db4:
.word 0x0142282a
matrix_209db8:
.word 0x006a302a
matrix_209dbc:
.word 0x0045500b
matrix_209dc0:
.word 0x0066500b
matrix_209dc4:
.word 0x00a62825
matrix_209dc8:
.word 0x00052d80
matrix_209dcc:
.word 0x00250825
matrix_209dd0:
.word 0xac880024
matrix_209dd4:
.word 0xac890028
matrix_209dd8:
.word 0xac8a002c
matrix_209ddc:
.word 0x3c027f87
matrix_209de0:
.word 0x3442e000
matrix_209de4:
.word 0x3c038000
matrix_209de8:
.word 0x00411024
matrix_209dec:
.word 0x0002180a
matrix_209df0:
.word 0x00230825
matrix_209df4:
.word 0x03e00008
matrix_209df8:
.word 0xac8100fc
.globl np_gte_far_entries
np_gte_far_entries:
li $at,-32768
li $25,0
j far_load
nop
li $at,0
li $25,0
j far_load
nop
li $at,-32768
li $25,12
j far_load
nop
li $at,0
li $25,12
j far_load
nop
.globl np_gte_matrix_far_entries
np_gte_matrix_far_entries:
li $at,-32768
li $25,0
j far_special_load
nop
li $at,0
li $25,0
j far_special_load
nop
li $at,-32768
li $25,12
j far_special_load
nop
li $at,0
li $25,12
j far_special_load
nop
far_load:
far_matrix_209ca8:
.word 0x84a80000
far_matrix_209cac:
.word 0x84a90002
far_matrix_209cb0:
.word 0x84aa0004
far_matrix_209cb4:
.word 0x84ab0006
far_matrix_209cb8:
.word 0x84ac0008
far_matrix_209cbc:
.word 0x84ad000a
far_matrix_209cc0:
.word 0x84ae000c
far_matrix_209cc4:
.word 0x84af000e
lh $24,16($a1)
b far_vector
nop
far_special_load:
lbu $9,24($a0)
sll $9,$9,4
subu $8,$zero,$9
lh $10,32($a0)
lh $11,132($a0)
move $12,$11
move $13,$11
lh $14,136($a0)
move $15,$14
move $24,$14
far_vector:
bnez $a2,far_packed_vector
nop
lh $v0,36($a0)
lh $v1,40($a0)
b far_products
lh $a1,44($a0)
far_packed_vector:
lh $v0,0($a2)
lh $v1,2($a2)
lh $a1,4($a2)
far_products:
mult $8,$v0
mult1 $11,$v0
pmfhl.lw $8
pcpyud $11,$8,$8
mult $14,$v0
pmfhl.lw $14
move $a3,$at
move $a2,$zero
lw $v0,212($a0)
dsll $v0,$v0,12
daddu $8,$8,$v0
dsra32 $at,$8,11
slt $v0,$zero,$at
slti $at,$at,-1
sll $v0,$v0,30
or $a2,$a2,$v0
sll $at,$at,27
or $a2,$a2,$at
dsrav $8,$8,$25
sll $8,$8,0
slti $v0,$8,-32768
li $at,32767
slt $at,$at,$8
or $v0,$v0,$at
sll $v0,$v0,24
or $a2,$a2,$v0
lw $v0,216($a0)
dsll $v0,$v0,12
daddu $11,$11,$v0
dsra32 $at,$11,11
slt $v0,$zero,$at
slti $at,$at,-1
sll $v0,$v0,29
or $a2,$a2,$v0
sll $at,$at,26
or $a2,$a2,$at
dsrav $11,$11,$25
sll $11,$11,0
slti $v0,$11,-32768
li $at,32767
slt $at,$at,$11
or $v0,$v0,$at
sll $v0,$v0,23
or $a2,$a2,$v0
lw $v0,220($a0)
dsll $v0,$v0,12
daddu $14,$14,$v0
dsra32 $at,$14,11
slt $v0,$zero,$at
slti $at,$at,-1
sll $v0,$v0,28
or $a2,$a2,$v0
sll $at,$at,25
or $a2,$a2,$at
dsrav $14,$14,$25
sll $14,$14,0
slti $v0,$14,-32768
li $at,32767
slt $at,$at,$14
or $v0,$v0,$at
sll $v0,$v0,22
or $a2,$a2,$v0
mult $9,$v1
mult1 $12,$v1
madd $10,$a1
madd1 $13,$a1
pmfhl.lw $8
pcpyud $9,$8,$8
mult $15,$v1
madd $24,$a1
pmfhl.lw $10
dsrav $8,$8,$25
dsrav $9,$9,$25
dsrav $10,$10,$25
sll $8,$8,0
sll $9,$9,0
sll $10,$10,0
move $at,$a3
far_publish_209d64:
.word 0xac880064
far_publish_209d68:
.word 0xac890068
far_publish_209d6c:
.word 0xac8a006c
far_publish_209d70:
.word 0x0020102d
far_publish_209d74:
.word 0x24037fff
far_publish_209d78:
move $at,$a2
far_publish_209d7c:
.word 0x0102282a
far_publish_209d80:
.word 0x0068302a
far_publish_209d84:
.word 0x0045400b
far_publish_209d88:
.word 0x0066400b
far_publish_209d8c:
.word 0x00a62825
far_publish_209d90:
.word 0x00052e00
far_publish_209d94:
.word 0x00250825
far_publish_209d98:
.word 0x0122282a
far_publish_209d9c:
.word 0x0069302a
far_publish_209da0:
.word 0x0045480b
far_publish_209da4:
.word 0x0066480b
far_publish_209da8:
.word 0x00a62825
far_publish_209dac:
.word 0x00052dc0
far_publish_209db0:
.word 0x00250825
far_publish_209db4:
.word 0x0142282a
far_publish_209db8:
.word 0x006a302a
far_publish_209dbc:
.word 0x0045500b
far_publish_209dc0:
.word 0x0066500b
far_publish_209dc4:
.word 0x00a62825
far_publish_209dc8:
.word 0x00052d80
far_publish_209dcc:
.word 0x00250825
far_publish_209dd0:
.word 0xac880024
far_publish_209dd4:
.word 0xac890028
far_publish_209dd8:
.word 0xac8a002c
far_publish_209ddc:
.word 0x3c027f87
far_publish_209de0:
.word 0x3442e000
far_publish_209de4:
.word 0x3c038000
far_publish_209de8:
.word 0x00411024
far_publish_209dec:
.word 0x0002180a
far_publish_209df0:
.word 0x00230825
far_publish_209df4:
.word 0x03e00008
far_publish_209df8:
.word 0xac8100fc
#endif

#if NP_MDEC_ZERO_SCALE && NP_MDEC_BLOCK_FASTPATH

/* Zero-scale cache uses the existing native AAN precision. Preserve all
 * nonzero-scale cache entries and both multiply banks. */
.balign 16
.globl np_mdec_zero_quant_init
np_mdec_zero_quant_init:
/* The native builder already retains its cache base in $10. The cold frame
 * caller supplies the same base in its call delay slot. */
la $9,np_mdec_zero_factors
lq $8,0($9)
sq $8,0($10)
sq $8,128($10)
lq $8,16($9)
sq $8,16($10)
sq $8,144($10)
lq $8,32($9)
sq $8,32($10)
sq $8,160($10)
lq $8,48($9)
sq $8,48($10)
sq $8,176($10)
lq $8,64($9)
sq $8,64($10)
sq $8,192($10)
lq $8,80($9)
sq $8,80($10)
sq $8,208($10)
lq $8,96($9)
sq $8,96($10)
sq $8,224($10)
lq $8,112($9)
sq $8,112($10)
jr $ra
sq $8,240($10)
/* Zero scale is independent of quant-table uploads, including first decode. */
.globl np_mdec_zero_frame
np_mdec_zero_frame:
/* The hook supplies the original LUI in its delay slot. Reuse that base;
 * perform the stack allocation in the conditional branch delay slot. */
lh $8,0x7d60($v1)
addiu $8,$8,-16
beqz $8,mdec_zero_frame_ready
addiu $sp,$sp,-96
move $24,$ra
jal np_mdec_zero_quant_init
addiu $10,$v1,0x7d60
move $ra,$24
mdec_zero_frame_ready:
/* Replay the guarded third prologue instruction in the return delay slot. */
j 0x20dbc4
sd $s1,24($sp)
/* The general parser already cleared the block and stored its DC/scale.
 * A zero scale uses linear positions instead of the zigzag table. */
.globl np_mdec_zero_rle
np_mdec_zero_rle:
sra $9,$9,22
sh $9,0($a1)
li $a3,-63
mdec_zero_token:
lhu $a0,0($v0)
addiu $v0,$v0,2
beq $a0,$8,mdec_zero_padding
srl $9,$a0,10
addu $a3,$a3,$9
bgez $a3,mdec_zero_padding
sll $12,$a3,1
sll $10,$a0,22
sra $10,$10,22
addu $12,$a1,$12
addiu $a3,$a3,1
bnez $a3,mdec_zero_token
/* Fold the negative index's block-end bias into the store displacement. */
sh $10,128($12)
mdec_zero_padding:
lhu $9,0($v0)
beq $9,$8,mdec_zero_padding
addiu $v0,$v0,2
mdec_zero_return:
jr $ra
addiu $v0,$v0,-2
.balign 16
.globl np_mdec_zero_factors
np_mdec_zero_factors:
.hword 16,22,20,18,16,12,8,4
.hword 22,30,28,26,22,17,12,6
.hword 20,28,27,24,20,16,11,5
.hword 18,26,24,22,18,14,10,5
.hword 16,22,20,18,16,12,8,4
.hword 12,17,16,14,12,9,6,3
.hword 8,12,11,10,8,6,4,2
.hword 4,6,5,5,4,3,2,1
#endif
.globl np_gte_native_three_end
np_gte_native_three_end:
