# Tekken 3: local PSXCore correction review

This review concerns our local PSXCore launch/readback adaptations. No online Tekken patch, POPStarter mode selector, timing routine, overclock or gameplay cheat is applied.

## What our correction changes

The graphics readback correction redirects one guarded instruction in POPS at `0x0021bfe8` to `np_graphics_readback_start` in `runtime/hooks.S`. POPS has programmed a VIF1 DMA descriptor before sending two FIFO quadwords. The local host's FIFO interpreter also advances DMA address/count for these manual writes, leaving the readback shifted by 32 bytes and shortened. The adapter reloads the already prepared native address/count and performs the original DMA start. It preserves registers and does not rewrite pixels, texture data, GPU packets, game code or clocks.

This is shared readback behavior, not a Tekken-specific game problem. It was originally reproduced with Spyro: uploads reached GS VRAM intact, while a 32,768-byte font came back shifted. After the adapter, the entire CPU buffer and cached texture matched with no byte disagreements. Earlier Tekken captures also verified the boot-font readback. These results establish a host-interoperability correction implemented inside POPS through PSXCore; they do not prove unmodified POPS is wrong on physical PS2 hardware.

Earlier investigation attributed the descriptor consumption to host FIFO handling. This is historical evidence, not an instruction to investigate the emulator. PCSX2 patches and emulator-specific fixtures/runners have been removed; future work targets POPS/PSXCore only. Stock-host and physical-console behavior remain unverified.

| Local item | Classification |
| --- | --- |
| Readback descriptor restoration | Shared POPS/host interoperability adaptation, not a Tekken game-code fix. Remains an optional PCSX2-environment capability pending physical evidence. |
| License adapter | Guarded runtime startup/license-format adaptation used by other titles too. Not a demonstrated Tekken emulation defect. |
| Tekken header digest in legacy `hddlicensetekken` | Identity of the prepared disc, not a bug fix. The v2 request supplies this as measured image metadata. |
| Texture-window correction | Shared POPS core behavior, present in Default, Automatic and Custom. No title match or option enables it. |
| Save-worker guards | Required v2 launch protections and isolated save routing, independent of optional profile fixes. |

The older title mode merely combines the common license/readback adapters with Tekken's header digest. It contains no new Tekken CPU, GTE, animation or gameplay modification. Remaining character-selection/portrait bands were not proved fixed by that adapter. The historical packed RGB24 movie repair was in PCSX2. That implementation has been removed from the project; PSXCore does not yet contain an equivalent intro repair. It must not be relabeled as a PSXCore or physical-console POPS correction.

## Current test

The three-track US image is `SLUS_004.02`, SHA-256 `84f3a003de3afe367013e75917ecc04ed066c4f44d18e1292903ca77def0b570`; its header SHA-1 is `5fa2a71ec1a0082daf4d5acf943d1311beed465b`. A fresh disk clone, complete mounted-disc readback, and isolated `tekken3` save partition preserve the earlier fixture.

The new bootstrap uses core revision 1/catalog revision 3 with nominal EE rate/skip and the existing software-rendered host. Strict Default selected baseline with optional mask 0, installed the shared wrapping correction and native guards, mounted the disc/save partitions, and validated both cards. The visible window remained black; preparation is not successful gameplay acceptance. Default leaves the existing license/readback adapters off. Any subsequent run enabling them must be labeled Custom rather than Default. Broad graphics, audio, in-game save and physical-console acceptance remain unverified.

The former modified-host private test launchers have been removed. The accepted historical intro result does not transfer automatically to an unmodified host.

After removal, a fresh launch with the unchanged local baseline executable and its matching stock application files used the current PSXCore bootstrap, nominal clocks and EE interpreter. Custom selected baseline/catalog revision 3 with optional mask 3; core revision 1, license guards, native disc mounting and both cards passed. Visible snapshots confirmed the PlayStation boot screen and Tekken's opening movie. The movie again showed corrupt graphics; one observed interval showed roughly 39% host speed and 7.69 game FPS. This is a boot observation, not gameplay, audio, save or performance acceptance. No emulator repair was reintroduced.
