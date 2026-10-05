# Shared POPS runtime corrections

Core corrections repair behavior in the supported POPS runtime independently of game compatibility profiles. `src/runtime_corrections.c` queues them during every bootstrap mode after the supplied runtime's SHA-256 and ELF layout have been verified. `np_patch_apply` validates all expected bytes and bounds/overlaps before any staged byte changes. Files on disk remain unchanged; no PCSX2 modification is involved.

POPS is proprietary and its source is unavailable in this repository. This is an integrated correction to its loaded machine code, technically an in-memory binary patch, not a rebuilt POPS source release. Moving it into the core removes the per-game selection requirement; it does not imply universal hardware validation.

## Texture-window wrapping (core revision 1)

The supported runtime's texture-window handler initializes an unmasked axis to GS clamp. The correction changes that shared default to repeat; its existing masked-axis region-repeat branch is preserved. The change is at EE virtual address `0x0021d6a4`, expected word `0x240f0001`, corrected word `0x240f0000`. Only the checksum-pinned runtime is accepted.

This is a texture-handling correction, not a Doom sky override. The [PS1 texture-window specification](https://psx-spx.consoledev.net/ps1/gpu/rendering-attributes/#gp0e2h-texture-window-setting) describes coordinate masking/repetition, and [rectangle rendering](https://psx-spx.consoledev.net/ps1/gpu/render-rectangle-commands/) repeats texture data when drawing beyond the 256-texel window. Comparing this behavior with the recorded clamp/sky output supports the shared correction. The saved-scene evidence is in [GRAPHICS.md](tools/compatibility/GRAPHICS.md).

The correction applies to every game and bootstrap mode using the supported runtime, under both platform values. Automatic, Default and Custom options govern optional game compatibility adapters, not core behavior. No serial/image match is required for core corrections. The former `NP_FIX_TEXTURE_WRAP` option/bit 4 is retired and rejected; optional manifests cannot reuse its bit, symbol, or overwrite its four-byte address range. Catalog revision is now 3. Startup logs separately report `core-corrections=1 texture-window=repeat scope=runtime` and the optional compatibility mask.

Local validation includes 29 Python regressions, profile/core C tests, PS2SDK compilation with warnings treated as errors, existing ELF/identity/save-hook/worker/consumer checks, and isolated cold boots with Default mode and an unidentified-game Custom request. Read-only PINE checks the corrected instruction without loading a state or selecting Doom's profile. The Default test checks preparation/correction only; missing optional license/readback adapters do not establish successful game launch. Testing platform values is API coverage, not physical-console acceptance.

The correction remains experimental across other games and physical PS2 hardware. Existing local host modifications are documented separately; no stock-PCSX2 acceptance is claimed. No FPS, timing, overclock or speedhack change is made.
