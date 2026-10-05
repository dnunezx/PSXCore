# Compatibility scope

Fix and investigate POPS/PSXCore only. PCSX2 is an unchanged test host: never
troubleshoot, instrument, build or patch it. Do not add emulator source patches,
emulator regression fixtures or tools that depend on custom emulator builds.
Existing interfaces may inspect guest state for POPS/PSXCore investigations.

Apply runtime changes only after revision, expected-byte, bounds and overlap
validation. Preserve separately supplied firmware/game files and user saves.
Results obtained previously with modified hosts remain historical evidence;
stock-host and physical-console acceptance require separate validation.
