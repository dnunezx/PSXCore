# PSXCore

**An open-source PS1 launch and runtime-adaptation project for PlayStation 2, with a standalone tester frontend.**

PSXCore provides an inspectable launcher layer, guarded runtime corrections, and reusable interfaces for other PS2 homebrew projects. The core can be integrated into your own frontend; **PSXCore Tester** provides a small game browser to try PSXCore.

Sony's proprietary POPS runtime is **not included**. You must separately supply the supported, checksum-verified `POPS.ELF` and `IOPRP252.IMG`, along with your games in VCD format. PSXCore applies validated changes to the runtime in PS2 memory; supplied firmware and game files remain unchanged.

> **Experimental draft:** the current tester package is based on source revision [`b968ee0`](https://github.com/dnunezx/PSXCore/commit/b968ee06c74bbf5a2cebad77ed2503aca34e1058). Compilation and local frontend, card-safety, launch-policy, and executable-layout checks passed. Physical-PS2 gameplay, save/reload, and storage acceptance testing remain pending. These checks do not establish compatibility for a particular console or game.

## Recent changes

### PSXCore Tester

- Standalone PS2 frontend with a seven-title paged library, wrapped game names, launch checks, and a dark interface with green highlights.
- USB and internal ATA selection for FAT32/exFAT storage.
- Shared or per-game virtual memory cards, with shared cards selected at startup.
- Automatic creation and verification of missing cards before launch, while preserving existing cards.
- Card management for creating an absent pair or copying a pair into absent destinations after confirmation.
- An R2 toggle for **Code Validation**, with matching ON and OFF launch arguments supported by the bootstrap.
- Bootstrap memory-layout checks before handoff. The tester loads above 16 MiB, separate from the bootstrap's lower memory region.
- Decorative animation updates without rescanning storage or checking cards on every animation tick.

### Graphics and GTE

- Corrected the GPU information-query selector so it uses the requested query rather than the command opcode.
- Added an indexed-texture palette refresh before native textured draws.
- Added zero-depth GTE projection handling without trapping.
- Retained shared texture-window wrapping, valid linked GPU DMA from RAM address zero, GTE register-23 writes, and GTE color/register corrections.
- Retained RGB24 movie output, display-transition handling, and buffered interlace presentation.

### Audio and movie decoding

- Reduced native reverb processing and clipping work while preserving the guarded routine's arithmetic, guest-access ordering, and relevant register state.
- Reduced RGB24 movie color-conversion work and added guarded RLE and constant-chroma decoding fast paths.
- Reduced audio-callback and empty graphics-flush overhead.
- Retained SPU capture-IRQ delivery, audio-timer and CDDA corrections, SPU state preservation, and gap de-clicking.
- **Adaptive Audio remains OFF by default.** It is available to callers through an explicit launch option, but the current tester does not enable it or provide a toggle.

### CPU, storage, and runtime safeguards

- Reduced decoded-code validation and RAM-access overhead; Code Validation is now an optional launch setting.
- Retained guarded memory access, return/load-delay handling, scratchpad timing, and CPU loop-cycle accounting.
- Added an exact polygon-cost division fast path and a bounded sector-tail cache to avoid repeated disc seeks.
- Corrected the seeking-status bit after a sector passes the native sync-header check.
- Retained native FAT32/exFAT storage integration, post-reset ATA device-name handling, and onscreen startup diagnostics.
- Runtime changes require a supported revision, matching expected bytes, valid bounds, and a non-overlapping patch plan before application.
- IGR and CD-switching work remains experimental and disabled in the tester package.

Performance improvements depend on the game and scene. They do not establish universal full-speed playback, eliminate every audio gap, or guarantee that a game can be completed.

## Storage support

The current tester scans **`mass0:/POPS`**. It lists up to **128 VCDs** directly in that folder; it does not scan subfolders or combine libraries from multiple volumes.

| Storage layout | Current tester behavior |
| --- | --- |
| USB drive with FAT32 or exFAT | Implemented. Select **Square** at startup. |
| Internal ATA HDD with FAT32 or exFAT | Implemented. Select **Cross** at startup. The volume must appear as `mass0:`. |
| Traditional APA/PFS PS2 HDD | Not supported by this tester. The core has a native-partition launch foundation, but an end-to-end APA/PFS browser and launch flow is not packaged here. |
| MX4SIO or MMCE | Not available in this tester. |

**An exFAT internal HDD and a traditional APA/PFS HDD require different storage paths.** Selecting ATA does not make this tester browse existing PFS partitions. An existing APA/PFS drive can remain intact while the tester runs from a separate USB drive.

The FAT32/exFAT paths are implemented; physical-console acceptance remains pending. A previously reported failure with multiple USB storage devices remains unresolved.

## Install PSXCore Tester

You need a PS2 that can already launch homebrew ELF files, the matching tester package, a suitable storage volume, the supported POPS files, and VCD games.

For a first USB setup:

1. Extract the tester package to the root of a FAT32 USB drive. FAT32 also avoids requiring exFAT support in the homebrew launcher used to start the tester.
2. Keep the included bootstrap in the `POPS` folder.
3. Add your separately supplied `POPS.ELF`, `IOPRP252.IMG`, and VCD games to that same folder.
4. Connect the drive to the PS2 and start `PSXCORE-TESTER.ELF` through your homebrew launcher, such as [wLaunchELF](https://github.com/ps2homebrew/wLaunchELF).
5. Press **Square** for USB, select a title, and press **Cross** to launch. Use **Triangle** to inspect checks if launch is blocked.

```text
Drive root/
├── PSXCORE-TESTER.ELF
└── POPS/
    ├── psxcore-runtime-bootstrap.elf
    ├── POPS.ELF                 ← supply separately
    ├── IOPRP252.IMG             ← supply separately
    └── Your Game.VCD           ← supply separately
```

For an internal FAT32/exFAT ATA drive, use the same layout on that drive and select **Cross** at startup. Keep the tester and its matching bootstrap together when updating. Missing, unsupported, or invalid inputs can block launch.

The package includes setup instructions, dependency notices, a source-revision manifest, and SHA-256 checksums. Firmware, games, and saves are supplied separately.

## Controls

| Screen | Button | Action |
| --- | --- | --- |
| Storage selection | Cross | Select internal ATA storage. |
| Storage selection | Square | Select USB storage. |
| Game library | Up / Down | Select a title. |
| Game library | L1 / R1 | Move between pages. |
| Game library | Cross | Launch the selected title when ready. |
| Game library / launch checks | Triangle | Toggle launch checks. |
| Game library / launch checks | Square | Switch shared and per-game cards. |
| Game library / launch checks | R2 | Toggle Code Validation for the next launch. |
| Game library / launch checks | Select | Rescan the library. |
| Game library | Circle | Open card management. |
| Launch checks | Circle | Return to the library. |
| Card management | Cross | Request creation of a blank pair. Confirmation is required. |
| Card management | Square | Request copying from the other card mode. Confirmation is required. |

## Default settings

| Setting | Current tester default |
| --- | --- |
| Memory cards | Shared pair. |
| Code Validation | **OFF**. R2 changes the next launch; the setting resets to OFF when the tester restarts. |
| Adaptive Audio | **OFF**. No tester toggle is provided. |
| Shared functional corrections and normal fast paths | Enabled by the standard core build where applicable. |
| IGR / return to frontend | Disabled. |
| In-game disc switching | Disabled. |
| Optional diagnostic and CD/IGR experiments | Disabled. |

Code Validation checks decoded PS1 instructions before reusing compiled blocks. Enabling it can help with some nonboot or black-screen failures, at an added processing cost. POPS retains its native code invalidation behavior when this optional check is OFF.

Adaptive Audio is a separate, optional output-rate feedback mechanism. It can affect tempo and pitch; it is not a CPU-speed fix. Native output-rate behavior remains selected in this tester.

## Virtual memory cards

Games use a pair of **128 KiB virtual PS1 memory cards**. Shared mode starts with:

```text
/POPS/ps1emu/card0
/POPS/ps1emu/card1
```

Per-game mode uses:

```text
/POPS/CARDS/<game-path-hash>-0.vmc
/POPS/CARDS/<game-path-hash>-1.vmc
```

Before launch, the tester creates and verifies missing cards in the selected mode. If only one slot is missing, it creates only that slot. Existing cards are never replaced. Invalid cards, inaccessible destinations, or recovery files associated with a missing slot block automatic creation and launch.

Card management can create an absent pair or copy a pair from the other mode after confirmation. Both destination files must be absent. Copying preserves the source; it does not merge saves or overwrite a destination. A failed write may leave an incomplete new file that needs inspection before retrying.

Per-game identities derive from the case-insensitive VCD path. Renaming or moving a VCD can select a different pair. Restore its original path to select the previous pair again. The tester does not provide a save editor; games write saves through PSXCore while running.

## Known limitations

- Physical-console gameplay, storage behavior, and actual game-save/cold-reload acceptance remain pending for this package.
- Traditional APA/PFS HDD browsing and launch are not available in the tester.
- Multiple USB storage devices can encounter the unresolved reported storage failure.
- MX4SIO and MMCE are unfinished and unavailable here.
- In-game return and disc swapping are unavailable. Internal helpers and experimental code do not constitute working user features.
- Audio gaps, movie stutter, and scene-dependent slowdown can remain. More buffering and adaptive output do not prove correct emulation timing.
- Save-aware shutdown and power-loss durability are unverified. The physical power-button handler requests immediate shutdown without waiting for an active save, and still needs physical-console validation.
- No full-playthrough or universal game-compatibility claim is made for this draft.

## Build from source

### Portable library

The reusable C11 library can be built with CMake:

```sh
cmake -S . -B build/host
cmake --build build/host
```

### PS2 runtime and tester

Use an installed PS2SDK toolchain. The runtime also requires the matching SDK source for its private iomanX build and four external storage-driver modules with the exact names and SHA-256 hashes checked by the root Makefile.

```sh
make library
make runtime NP_BDMA_MODULE_DIR=/path/to/pinned-drivers NP_PS2SDK_SOURCE=/path/to/ps2sdk-source
make tester
```

| Output | Purpose |
| --- | --- |
| `build/libpsxcore.a` | Reusable PS2 core library. |
| `build/runtime/psxcore.elf` | Runtime bootstrap. The tester package installs it as `POPS/psxcore-runtime-bootstrap.elf`. |
| `build/psxcore-tester.elf` | Standalone tester. The package names it `PSXCORE-TESTER.ELF`. |

The optional `make inventory` target builds a read-only ATA/APA inventory utility. That developer target does not add APA/PFS game support to the tester. Generated artifacts remain under ignored build directories; the SDK and driver inputs are external dependencies.

## Integrate PSXCore

The core has no frontend dependency. Add it to another project as a Git submodule:

```sh
git submodule add https://github.com/dnunezx/PSXCore.git external/psxcore
git submodule update --init --recursive
```

For a CMake consumer:

```cmake
add_subdirectory(external/psxcore)
target_link_libraries(your_target PRIVATE PSXCore::core)
```

Reusable interfaces cover VCD checking, bounded ELF loading, launch requests, compatibility-option resolution, hashing, and guarded patch planning. Automatic / Default / Custom options belong to the core API; the tester exposes only the controls documented above. The public catalog currently contains no automatic game profiles.

See [INTEGRATION.md](https://github.com/dnunezx/PSXCore/blob/main/INTEGRATION.md) and the implementation headers for integration details. Some older documentation describes earlier launch-contract revisions; use the current headers and implementation as the authority for ABI details.

## Thanks and upstream credits

PSXCore benefits from work shared by the PS2 homebrew community. Thank you to:

- **[israpps (El_isra)](https://github.com/israpps/BDMAssault)** for BDMAssault's USB exFAT drivers and the original POPSLoader project.
- **saildot4k (R3Z3N)** for the BDMA-ATA driver used for internal HDD support.
- **[Ripto / NathanNeurotic](https://github.com/NathanNeurotic/POPSLoader)** for maintaining and publishing POPSLoader and its driver variants.
- **Hugopocked** for POPStarter-related driver and compatibility work.
- **krHACKen** for POPStarter, whose documented behavior informed PSXCore's launch and power-button work.
- **[PS2DEV and PS2SDK contributors](https://github.com/ps2dev/ps2sdk)** for the development tools, libraries, and IOP modules.
- **[ChaN](https://elm-chan.org/fsw/ff/)** for FatFs, used by the storage drivers.
- **[bucanero](https://github.com/bucanero/pops2cue)** for a public VCD format reference.

Thanks also to **ShaolinAssassin and AstroSnail** for helping preserve POPStarter documentation. Runtime packages retain dependency notices and applicable licenses. See [THIRD_PARTY.md](https://github.com/dnunezx/PSXCore/blob/main/THIRD_PARTY.md) and the package's `THIRD_PARTY.txt` and `LICENSES` directory.

## License

Copyright (c) 2026 Danny Nunez (dnunezx). Original PSXCore code and the tester use the **Academic Free License 3.0**. See [LICENSE](https://github.com/dnunezx/PSXCore/blob/main/LICENSE) and [AFL-3.0](https://github.com/dnunezx/PSXCore/blob/main/LICENSES/AFL-3.0.txt). External dependencies retain their own licenses and notices.

Sony POPS, firmware, games, and saves are supplied separately and are not licensed or distributed by this project.
