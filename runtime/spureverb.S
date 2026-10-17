/* ATTR.bit7 gates reverb RAM writes, not its reads or output. Keep native
 * filtering, read addresses, cursor wrapping and IRQ detection. These leaf
 * redirects guard only the eight native SAME/DIFF/APF halfword stores.
 * The pinned reverb leaf never uses at. Its checked call wrapper saves at
 * once; each guard still reads live ATTR. Both return paths restore at before
 * rejoining the caller or native IRQ dispatcher. No cached control state. */
#if NP_SPU_REVERB_READONLY != 1
#error "SPU reverb write guard accepts a boolean build flag only"
#endif
    .balign 16
    .globl np_spu_reverb_start,np_spu_reverb_end
np_spu_reverb_start:
    .macro NP_REVERB_STORE name, store, next, resume
    .globl \name
\name:
.Lreverb_start\@:
    lui $1,0x7000
    lhu $1,0x4fa($1)
    andi $1,$1,0x80
    beqz $1,1f
    nop
    .word \store
1:
    .word \next
    j \resume
    nop
    /* Preserve every existing store entry and resident extent. */
    .org .Lreverb_start\@+60,0
    .endm
    NP_REVERB_STORE np_spu_reverb_lsame,0xa5480000,0x0004bbc3,0x212650
    NP_REVERB_STORE np_spu_reverb_rsame,0xa52e0000,0x25290002,0x21273c
    NP_REVERB_STORE np_spu_reverb_ldiff,0xa6860000,0x26940002,0x212858
    NP_REVERB_STORE np_spu_reverb_rdiff,0xa5470000,0x254a0002,0x21291c
    NP_REVERB_STORE np_spu_reverb_lapf1,0xa5870000,0x24073def,0x212c00
    NP_REVERB_STORE np_spu_reverb_rapf1,0xa5ee0000,0x25ef0002,0x212c98
    NP_REVERB_STORE np_spu_reverb_lapf2,0xa5ee0000,0x25ef0002,0x212d40
    NP_REVERB_STORE np_spu_reverb_rapf2,0xa4cc0000,0x24c60002,0x212e38
    .balign 16
np_spu_reverb_end:

    .pushsection ._psxcore_spu_context,"ax",@progbits
    .balign 16
    .globl np_spu_reverb_call,np_spu_reverb_tail,np_spu_reverb_context_end
np_spu_reverb_call:
    addiu $sp,$sp,-32
    sq $1,0($sp)
    sq $ra,16($sp)
    jal 0x2123a0
    nop
    lq $1,0($sp)
    lq $ra,16($sp)
    jr $ra
    addiu $sp,$sp,32
np_spu_reverb_tail:
    /* Original reverb epilogue has already popped its 304-byte frame. */
    lq $1,0($sp)
    lq $ra,16($sp)
    addiu $sp,$sp,32
    j 0x210b38
    nop
    .balign 16
np_spu_reverb_context_end:
    .popsection
