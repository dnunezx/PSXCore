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
/* CD admission arena; packet construction has a separate resident gap. */
/* Shared GTE compiler adapter; all guarded redirects use this fixed entry. */
.gte_transfer 0x000ec000: {
 KEEP(*(._psxcore_gte_transfer))
} :resident
ASSERT(np_gte_flag_write == 0x000ec000, "runtime FLAG write adapter moved")
ASSERT(. <= 0x000ec800, "runtime GTE transfer adapter overflow")
.cd_order 0x000ec800: {
 np_cd_order_begin = .;
 KEEP(*(._psxcore_cd_order.text))
 KEEP(*(._psxcore_cd_order.data))
 np_cd_order_end = .;
} :resident
ASSERT(. <= 0x000f0000, "runtime CD order arena overflow")
/* The native return loader/private stack ends below 0xef000. This helper
 * occupies the remaining bounded gap before the established 0xf0000 arena. */
ASSERT(. <= 0x000ef000, "runtime CD order overlaps loader retirement")
.loader_retire 0x000ef000: {
 np_loader_retire_begin = .;
 KEEP(*(._psxcore_loader_retire.text))
 KEEP(*(._psxcore_loader_retire.data))
 np_loader_retire_end = .;
} :resident
ASSERT(. <= 0x000f0000, "runtime loader retirement arena overflow")
.diagnostic 0x000f0000: {
 KEEP(*(._psxcore_hooks))
 KEEP(*(._psxcore_memory))
 KEEP(*(._psxcore_rgb24))
 KEEP(*(._psxcore_gp0))
} :resident
ASSERT(. <= 0x000f7000, "runtime diagnostic hooks overflow")
.return_delay 0x000f7000: {
 KEEP(*(._psxcore_return_delay))
 /* Dormant output hooks; explicit launch option installs their callers. */
 KEEP(*(._psxcore_hdtv))
} :resident
ASSERT(. <= 0x000f7800, "runtime return-load helpers overflow")
.gte_color 0x000f7800: {
 KEEP(*(._psxcore_gte_color))
} :resident
ASSERT(. <= 0x000f7900, "runtime GTE color helper overflow")
/* Shared runtime corrections contain guarded absolute compiler redirects. */
ASSERT(np_gte_mfc_sxyp == 0x000f7854, "runtime SXYP MFC adapter moved")
ASSERT(np_gte_swc_sxyp == 0x000f7868, "runtime SXYP SWC adapter moved")
ASSERT(np_gte_sxyp_end <= 0x000f7880, "runtime SXYP adapters overlap color input")
.interlace_scanout 0x000f7900: {
 KEEP(*(._psxcore_interlace_scanout))
 KEEP(*(._psxcore_igr_probe_stack))
 KEEP(*(._psxcore_spu_context))
} :resident
ASSERT(. <= 0x000f7d40, "runtime interlace scanout helpers overflow")
.clut_refresh 0x000f7d40: {
 KEEP(*(._psxcore_clut_refresh))
} :resident
ASSERT(. <= 0x000f7e10, "runtime CLUT refresh helpers overflow")
.gte_intpl 0x000f7e10: {
 KEEP(*(._psxcore_gte_intpl))
} :resident
ASSERT(. <= 0x000f7fc0, "runtime GTE interpolation helpers overflow")
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
/* The default resident ends below this reserved control helper. Optional
 * combinations that exhaust the gap must reject instead of overlapping it. */
.gp1_idle 0x000fec00: {
 KEEP(*(._psxcore_gp1_idle))
} :resident
ASSERT(SIZEOF(.gp1_idle)==0 || ADDR(.resident)+SIZEOF(.resident)<=0x000fec00,
       "runtime resident overlaps GP1 ownership helper")
ASSERT(. <= 0x000ff000, "runtime GP1 ownership helper overflow")
.code_guard 0x000ff000: {
 np_code_guard_resident_start = .;
 KEEP(*(._psxcore_code_guard))
 KEEP(*(._psxcore_code_guard_data))
 np_code_guard_resident_end = .;
} :resident
ASSERT(. <= 0x00100000, "runtime code guard overlaps loader")
/* Keep the optional packet builder clear of IGR's private return stack and
 * all established helpers. Oversized diagnostic combinations must reject. */
.packet_fast 0x000ffd00: {
 KEEP(*(._packet_fast.text))
} :resident
ASSERT(SIZEOF(.packet_fast)==0 ||
       ADDR(.code_guard)+SIZEOF(.code_guard)<=0x000ffd00,
       "runtime code guard overlaps polygon packet helper")
ASSERT(. <= 0x00100000, "runtime polygon packet helper overlaps loader")
