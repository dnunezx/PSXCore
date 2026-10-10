/* Original resident layout; included inside the SDK SECTIONS block. */
.diagnostic 0x000f0000: {
 KEEP(*(._psxcore_hooks))
 KEEP(*(._psxcore_memory))
 KEEP(*(._psxcore_rgb24))
 KEEP(*(._psxcore_gp0))
} :resident
ASSERT(. <= 0x000f7000, "runtime diagnostic hooks overflow")
.return_delay 0x000f7000: {
 KEEP(*(._psxcore_return_delay))
} :resident
ASSERT(. <= 0x000f7800, "runtime return-load helpers overflow")
.gte_color 0x000f7800: {
 KEEP(*(._psxcore_gte_color))
} :resident
ASSERT(. <= 0x000f7900, "runtime GTE color helper overflow")
.interlace_scanout 0x000f7900: {
 KEEP(*(._psxcore_interlace_scanout))
} :resident
ASSERT(. <= 0x000f8000, "runtime interlace scanout helpers overflow")
.resident 0x000f8000: {
 KEEP(*(._psxcore_transfer))
 *libkernel.a:*(.text .text.* .rodata .rodata.* .data .data.*)
} :resident
ASSERT(. <= 0x000ff000, "runtime transfer resident overflow")
.code_guard 0x000ff000: {
 np_code_guard_resident_start = .;
 KEEP(*(._psxcore_code_guard))
 KEEP(*(._psxcore_code_guard_data))
 np_code_guard_resident_end = .;
} :resident
ASSERT(. <= 0x00100000, "runtime code guard overlaps loader")
