/* Original resident layout; included inside the SDK SECTIONS block. */
/* Native swap arenas. The planner checks firmware, segment overlap and
 * main-thread ownership. The IGR loader/stack end at 0xe8000. */
.cd_control 0x000e8000: {
 np_cd_swap_begin = .;
 KEEP(*(._psxcore_cd_swap.text))
 KEEP(*(._psxcore_cd_swap.data))
 np_cd_swap_end = .;
} :resident
ASSERT(. <= 0x000e9000, "runtime CD control arena overflow")
.cd_session 0x000e9000: {
 KEEP(*(._psxcore_cd_reopen.text))
 KEEP(*(._psxcore_cd_reopen.data))
} :resident
ASSERT(. <= 0x000ea000, "runtime CD transport arena overflow")
.cd_park 0x000ea000: {
 KEEP(*(._psxcore_cd_swap.tocs))
} :resident
ASSERT(. <= 0x000eb000, "runtime CD TOC arena overflow")
.cd_gate 0x000eb000: {
 KEEP(*(._psxcore_cd_swap_runtime.entry))
 KEEP(*(._psxcore_cd_swap_runtime.text))
 . = ALIGN(16);
 KEEP(*(._psxcore_cd_swap_runtime.code_end))
 KEEP(*(._psxcore_cd_swap_runtime.data))
 . = ALIGN(16);
 KEEP(*(._psxcore_cd_swap_runtime.end))
} :resident
ASSERT(. <= 0x000ec000, "runtime CD scheduler arena overflow")
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
 KEEP(*(._psxcore_igr_probe_stack))
 KEEP(*(._psxcore_spu_context))
} :resident
ASSERT(. <= 0x000f7d40, "runtime interlace scanout helpers overflow")
.clut_refresh 0x000f7d40: {
 KEEP(*(._psxcore_clut_refresh))
} :resident
ASSERT(. <= 0x000f7fc0, "runtime CLUT refresh helpers overflow")
.cd_read_status 0x000f7fc0: {
 KEEP(*(._psxcore_cd_read_status))
} :resident
ASSERT(. <= 0x000f8000, "runtime CD read status helper overflow")
.resident 0x000f8000: {
 KEEP(*(._psxcore_transfer))
 *libkernel.a:*(.text .text.* .rodata .rodata.* .data .data.*)
 KEEP(*(._psxcore_igr_observe))
 KEEP(*(._psxcore_igr_wait))
 KEEP(*(._psxcore_igr_quit))
 KEEP(*(._psxcore_cd_observe))
 KEEP(*(._psxcore_cd_rendezvous))
 . = ALIGN(16);
 np_reverb_exact_start = .;
 KEEP(*(._psxcore_reverb_exact))
 KEEP(*(._psxcore_reverb_bridge))
 np_reverb_exact_end = .;
} :resident
ASSERT(. <= 0x000ff000, "runtime transfer resident overflow")
.code_guard 0x000ff000: {
 np_code_guard_resident_start = .;
 KEEP(*(._psxcore_code_guard))
 KEEP(*(._psxcore_code_guard_data))
 np_code_guard_resident_end = .;
} :resident
ASSERT(. <= 0x00100000, "runtime code guard overlaps loader")
