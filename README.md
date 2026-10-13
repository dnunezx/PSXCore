# PSXCore

PSXCore is an open-source PS1 launch and runtime-adaptation project for PlayStation 2. It began with a simple goal: make the launcher layer open for people to inspect, improve, fork, and integrate into their own projects.

This standalone repository contains the original implementation and its build files. It has no frontend dependency and can be consumed as a Git submodule. Development notes, plans, local data, test fixtures, test harnesses, and developer tools are excluded.

Sony's POPS emulator remains proprietary and **is not included**. Runtime execution requires separately supplied, checksum-verified `POPS.ELF` and `IOPRP252.IMG`. No Sony runtime, firmware, game, save, or emulator binary is distributed here.


## Improvements made

- **CPU and memory correctness:** guarded generated-memory access, a targeted return/load-delay correction, scratchpad read timing, internal branch-target cycle accounting, and validation of original decoded instructions before reusing compiled blocks. These address demonstrated startup and progression failures without editing game code.
- **Graphics:** shared texture-window wrapping, valid linked GPU DMA from RAM address zero, GTE23 register writes, RGB24 movie conversion and update routing, display-transition handling, and buffered interlace presentation.
- **Audio:** SPU capture IRQ delivery, audio-timer handling, CDDA playback, reverb processing, and preservation of voice/register state when enabling the SPU. Gap de-clicking reduces audible pops; it cannot correct sustained audio starvation.
- **Performance:** RAM fast paths, GP0 packet coalescing and scalar writes, GPU DMA/raster accounting, and MDEC movie-decoding fast paths. Improvements depend on the scene; there is no universal full-speed claim.
- **Launch and storage:** checked launch requests, Automatic / Default / Custom options, transactional runtime changes, FAT32/exFAT VCD access, native POPS storage integration, startup error reporting, and a physical power-button handler loaded after the POPS IOP reset.

## Known issues

- **USB storage:** the reported R5 failure with multiple USB storage devices remains unresolved.
- **ATA/exFAT storage:** the R6/R7 bootstrap corrects the native ATA driver's device-name handling after reset. The affected physical PS2 launch path still needs hardware confirmation.
- **Other storage devices:** MX4SIO integration is not ready to enable; MMCE support remains unfinished.
- **IGR:** returning from a game to the frontend is unavailable. The isolated return-loader investigation currently stops while reading the destination ELF after the IOP reset. Input/policy helpers and diagnostic hooks do not constitute working IGR.
- **Multiple discs:** in-game disc switching is not implemented. Frontend-managed disc groups and native protocol work remain in progress.

## Build

Build the portable C11 library using CMake:

```sh
cmake -S . -B build/host
cmake --build build/host
```

For PS2 builds, use an installed PS2SDK toolchain:

```sh
make library
make runtime
```

These produce `build/libpsxcore.a` and `build/runtime/psxcore-runtime-bootstrap.elf`. The optional `make inventory` target builds the read-only ATA/APA inventory implementation. Generated files stay under ignored `build/` directories. The SDK and its modules are external dependencies.

Using the SDK container from PowerShell in this repository:

```powershell
docker run --rm -v "${PWD}:/src" -w /src ps2max/dev:v20260228 sh -c 'make library && make runtime'
```

## Use as a submodule

Run in the consuming repository:

```sh
git submodule add https://github.com/dnunezx/PSXCore.git external/psxcore
git add .gitmodules external/psxcore
git commit -m "Add PSXCore submodule"
```

Commit the submodule pointer on the consuming project's desired branch. Other contributors initialize it with `git submodule update --init --recursive`.

For CMake integration:

```cmake
add_subdirectory(external/psxcore)
target_link_libraries(your_target PRIVATE PSXCore::core)
```

See [INTEGRATION.md](INTEGRATION.md) for API and runtime requirements. Forks, fixes, and integrations are welcome under the applicable license.

## Thanks and upstream credits

PSXCore benefits from work shared by the PS2 homebrew community. Thank you to:

- **[israpps (El_isra)](https://github.com/israpps/BDMAssault)** for BDMAssault's USB exFAT drivers and the original POPSLoader project.
- **saildot4k (R3Z3N)** for the BDMA-ATA driver used for internal hard drive support.
- **[Ripto / NathanNeurotic](https://github.com/NathanNeurotic/POPSLoader)** for maintaining and publishing POPSLoader and its driver variants.
- **krHACKen** for POPStarter, whose documented behavior informed PSXCore's launch and power button work.
- **[PS2DEV and PS2SDK contributors](https://github.com/ps2dev/ps2sdk)** for the PS2 development tools, libraries, and `poweroff.irx`.
- **[ChaN](https://elm-chan.org/fsw/ff/)** for FatFs, used by the storage drivers.
- **[bucanero](https://github.com/bucanero/pops2cue)** for a public VCD format reference.

Thanks also to **ShaolinAssassin and AstroSnail** for helping preserve POPStarter documentation. Attribution and license notices for bundled driver binaries are included with the runtime package; other dependencies are described in [THIRD_PARTY.md](THIRD_PARTY.md).

## License

Copyright (c) 2026 Danny Nunez (dnunezx). Original PSXCore code uses the Academic Free License 3.0. See [LICENSE](LICENSE) and [THIRD_PARTY.md](THIRD_PARTY.md). External dependencies retain their own licenses.
