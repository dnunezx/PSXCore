/* Original resident layout; included inside the SDK SECTIONS block. */
/* Dormant control experiments only. Outside the checked IGR loader's maximum
 * 0x84000..0xe7000 image and below the existing 0xf0000 helpers. Activation
 * still requires revision/segment/stack/overlap checks and live persistence.
 * These CD arenas overlap IGR's private return stack; cdresponse.c rejects
 * that feature combination. They are NOT live-safe resident allocations. */
.cd_control 0x000e8000: {
 KEEP(*(._psxcore_cd_control.text))
 KEEP(*(._psxcore_cd_response_receipt.text))
} :resident
ASSERT(. <= 0x000e9000, "runtime experimental CD control arena overflow")
.cd_session 0x000e9000: {
 KEEP(*(._psxcore_cd_session.text))
} :resident
ASSERT(. <= 0x000ea000, "runtime experimental CD session arena overflow")
.cd_park 0x000ea000: {
 KEEP(*(._psxcore_cd_park.text))
} :resident
ASSERT(. <= 0x000eb000, "runtime experimental CD parking arena overflow")
.cd_gate 0x000eb000: {
 KEEP(*(._psxcore_cd_gate.entry))
 KEEP(*(._psxcore_cd_gate.text))
 . = ALIGN(16);
 KEEP(*(._psxcore_cd_gate.code_end))
 KEEP(*(._psxcore_cd_gate.data))
 . = ALIGN(16);
 KEEP(*(._psxcore_cd_gate.end))
} :resident
ASSERT(. <= 0x000ec000, "runtime experimental CD gate arena overflow")
.diagnostic 0x000f0000: {
 KEEP(*(._psxcore_hooks))
 KEEP(*(._psxcore_memory))
 KEEP(*(._psxcore_rgb24))
 KEEP(*(._psxcore_gp0))
} :resident
ASSERT(. <= 0x000f7000, "runtime diagnostic hooks overflow")
.return_delay 0x000f7000: {
 KEEP(*(._psxcore_return_delay))
 /* Default-off helpers use only the existing bounded resident gaps. */
 KEEP(*(._psxcore_cd_response_pool.text))
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
 KEEP(*(._psxcore_cd_protocol.text))
 KEEP(*(._psxcore_cd_response.text))
} :resident
ASSERT(. <= 0x000ff000, "runtime transfer resident overflow")
.code_guard 0x000ff000: {
 np_code_guard_resident_start = .;
 KEEP(*(._psxcore_code_guard))
 KEEP(*(._psxcore_code_guard_data))
 np_code_guard_resident_end = .;
} :resident
ASSERT(. <= 0x00100000, "runtime code guard overlaps loader")
