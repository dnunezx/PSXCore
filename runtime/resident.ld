/* Original resident layout; included inside the SDK SECTIONS block. */
.diagnostic 0x000f0000: {
 KEEP(*(._psxcore_hooks))
 KEEP(*(._psxcore_memory))
} :resident
ASSERT(. <= 0x000f7000, "runtime diagnostic hooks overflow")
.resident 0x000f8000: {
 KEEP(*(._psxcore_transfer))
 *libkernel.a:*(.text .text.* .rodata .rodata.* .data .data.*)
} :resident
ASSERT(. <= 0x000ff000, "runtime transfer resident overflow")
