# PSXCore

**An open-source PS1 launch and runtime-adaptation project for PlayStation 2.**

PSXCore provides a reusable core for PS2 homebrew projects, with launch interfaces, guarded runtime corrections, **working in-game return**, and **native multi-disc swapping**. Integrate it into your own frontend and build on source you can inspect, modify, and share.

Sony's proprietary POPS runtime is **not included**. Supply the supported, checksum-verified `POPS.ELF` and `IOPRP252.IMG` separately, along with your games in VCD format. PSXCore applies validated changes to the runtime in PS2 memory; supplied firmware and game files remain unchanged.

## Core features

- **In-game return:** leave gameplay through the quit menu and return to the configured frontend, including LUNA integrations.
- **Multi-disc games:** switch between two to four registered VCD images during gameplay while retaining the same virtual memory-card pair.
- **Launch and integration:** VCD validation, bounded ELF loading, checked launch requests, hashing, and reusable interfaces for frontend developers.
- **Runtime safeguards:** revision checks, expected-byte validation, bounds checks, and overlap checks before runtime changes are applied.
- **Graphics:** GPU and GTE corrections, texture-window handling, indexed-texture palette refresh, RGB24 movie output, and interlaced display handling.
- **Audio and movies:** SPU and CDDA corrections, reverb processing improvements, and movie-decoding fast paths.
- **CPU and memory:** guarded memory access, load-delay and scratchpad corrections, cycle accounting, and optional decoded-code validation.
- **Storage:** FAT32/exFAT USB and internal ATA integration, with storage and launch configuration supplied by the consuming frontend.

## How PSXCore works

PSXCore connects your game menu to Sony's POPS emulator and adapts the runtime that executes on the PS2. Here is what happens when you launch a game:

1. **Your frontend chooses the session.** A menu such as LUNA supplies the VCD location, storage settings, virtual memory-card paths, and launch options. For a multi-disc game, it also registers the ordered disc set.

2. **PSXCore checks the launch.** It validates the launch request, VCD information, and supported POPS revision. Before applying a runtime correction, it checks the original code bytes, memory bounds, and patch locations for conflicts.

3. **PSXCore prepares POPS in memory.** It loads the runtime into PS2 RAM and applies the selected corrections there. These changes address CPU and memory behavior, graphics, audio, and movie decoding. The supplied runtime and game files remain intact on storage.

4. **POPS runs the game with those changes in place.** During play, PSXCore's installed runtime support handles registered disc swaps and the confirmed return to the configured frontend. Disc swaps retain the session's selected memory-card pair.

**A practical example:** an indexed-texture palette correction changes how the loaded POPS runtime refreshes color data before drawing a textured object. The game continues to supply its original texture and palette data; the correction adjusts how POPS handles that data. Other runtime corrections apply the same approach to their respective graphics, audio, CPU, or decoding routines.

### In-game return

In-game return works through POPS' native confirmed-quit route. The return menu offers **Return** and **Keep Playing**; confirming Return exits gameplay and loads the configured frontend ELF. The LUNA integration returns to its menu, so another game can be selected without restarting the console. The return target is configured by the integrating build.

### Native multi-disc swapping

A frontend registers an ordered set of **two to four VCDs** before launch. When the game requests another disc, use controller 1 to hold **L2 + R2 + Select** with the direction for that disc:

| Direction | Disc |
| --- | --- |
| Up | Disc 1 |
| Right | Disc 2 |
| Down | Disc 3 |
| Left | Disc 4 |

Release all buttons to complete insertion. PSXCore uses POPS' native disc services to reopen the selected image and validates its registered header and disc geometry. The selected memory-card pair stays bound to the entire set, allowing the game to continue using the same saves across discs. Single-disc launches do not install the swap hooks.

Disc-set registration belongs to the frontend integration; placing several VCD files in a folder alone does not register a set.

## Build from source

### Portable library

Build the reusable C11 library with CMake:

```sh
cmake -S . -B build/host
cmake --build build/host
```

### PS2 library and runtime

Use an installed PS2SDK toolchain. The runtime also requires matching PS2SDK source and the external storage-driver modules verified by the Makefile.

```sh
make library
make runtime NP_BDMA_MODULE_DIR=/path/to/pinned-drivers NP_PS2SDK_SOURCE=/path/to/ps2sdk-source
```

The PS2 library is written to `build/libpsxcore.a`. Generated artifacts remain under ignored build directories. SDK and storage-driver inputs are external dependencies.

## Integrate PSXCore

The core has no frontend dependency. Add it to your project as a Git submodule:

```sh
git submodule add https://github.com/dnunezx/PSXCore.git external/psxcore
git submodule update --init --recursive
```

For CMake integration:

```cmake
add_subdirectory(external/psxcore)
target_link_libraries(your_target PRIVATE PSXCore::core)
```

Your frontend supplies the game library, user interface, storage selection, and launch configuration. Use the current implementation headers for API and ABI details. See [INTEGRATION.md](https://github.com/dnunezx/PSXCore/blob/main/INTEGRATION.md) for additional integration guidance.

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

Thanks also to **ShaolinAssassin and AstroSnail** for helping preserve POPStarter documentation. See [THIRD_PARTY.md](https://github.com/dnunezx/PSXCore/blob/main/THIRD_PARTY.md) for dependency attribution and license information.

## License

Copyright (c) 2026 Danny Nunez (dnunezx). Original PSXCore code uses the **Academic Free License 3.0**. See [LICENSE](https://github.com/dnunezx/PSXCore/blob/main/LICENSE) and [AFL-3.0](https://github.com/dnunezx/PSXCore/blob/main/LICENSES/AFL-3.0.txt). External dependencies retain their own licenses and notices.

Sony POPS, firmware, games, and saves are supplied separately and are not licensed or distributed by this project.
