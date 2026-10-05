# PSXCore

PSXCore is an open-source PS1 launch and runtime-adaptation project for PlayStation 2. It began with a simple goal: make the launcher layer open for people to inspect, improve, fork, and integrate into their own projects.

This standalone repository contains the original implementation, reviewed compatibility manifests, generic tools, and synthetic regression tests. It has no frontend dependency and can be consumed as a Git submodule. Local development data, proprietary inputs, personal configuration, and private harnesses are excluded.

Sony's POPS emulator remains proprietary and **is not included**. Runtime execution requires separately supplied, checksum-verified `POPS.ELF` and `IOPRP252.IMG`. No Sony runtime, firmware, game, save, or emulator binary is distributed here.

**Compatibility fixes target POPS/PSXCore. PCSX2 is a test host, not the patch target.** PSXCore validates the separately supplied POPS revision and applies guarded changes to its staged code/data in PS2 RAM. Changes to the original launcher, patch policy and runtime adapters belong in PSXCore. Supplied Sony and game files remain unchanged.

Reusable POPS/PSXCore diagnostics are collected in [tools/compatibility](tools/compatibility/README.md). PCSX2 source patches, emulator regression fixtures and custom-host profiling tools have been removed. Never troubleshoot or modify PCSX2 for this project. Use an unmodified, separately supplied test host. Earlier modified-host results remain historical evidence and do not establish stock-host support. The [runtime change inventory](tools/compatibility/PATCHES.md) lists the remaining POPS/PSXCore adaptations.

## Status

[Automatic compatibility profiles](compatibility/README.md) match the serial, exact image hash, runtime revision, and tested environment. Doom and Spyro profiles select optional startup/readback adapters. The [shared texture-window correction](RUNTIME-CORRECTIONS.md) is integrated into PSXCore's core POPS boot path for every game/mode using the supported runtime; it no longer depends on Doom's profile. Developers add reviewed game profiles and optional byte corrections to `compatibility/`, regenerate the catalog, and rebuild. Default suppresses optional fixes while retaining core corrections; Custom overrides optional fixes only.

This is an experimental source release. The portable core validates VCD headers, fixed-size launch requests, and bounded ELF layouts and provides SHA-256 verification. It also resolves Automatic / Default / Custom compatibility options and validates complete patch sets before changing the staged runtime. The version-2 launch path selects an already prepared native disc partition and an explicit save partition; version 1 remains a separate dry-run API.

The separate runtime bootstrap contains version-specific startup, transfer, and in-memory adaptation code. Its experimental save path guards native worker IO and startup errors. Local PCSX2 diagnostics passed card creation/routing, invalid-card rejection, actual worker writes, a pre-replacement flush failure, and cold loading of the last successful diagnostic contents. Actual in-game save/reload, graceful shutdown, power-loss durability, native-disc provisioning, runtime handoff from a frontend, broader game compatibility, and physical-console acceptance remain to be established. See [INTEGRATION.md](INTEGRATION.md) for the launch contract and verification boundaries.

The portable prepared-disc catalog associates library images with existing native partitions by complete image SHA-256 and explicit stable save identity, then builds a checked v2 request from caller-measured image data. See [DISC-MAPPING.md](DISC-MAPPING.md). The consuming frontend owns image/storage verification, runtime transfer and provisioning; arbitrary-image conversion remains separate work.

## Build

Build the portable C11 library using CMake:

```sh
cmake -S . -B build/host
cmake --build build/host
ctest --test-dir build/host --output-on-failure
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

The opt-in generated-memory adapter and guest wait observers are documented in [MEMORY-ACCESS.md](MEMORY-ACCESS.md). They remain disabled by default; stock-host gameplay acceptance is incomplete.

## License

Copyright (c) 2026 Danny Nunez (dnunezx). Original PSXCore code uses the Academic Free License 3.0. See [LICENSE](LICENSE) and [THIRD_PARTY.md](THIRD_PARTY.md). External dependencies retain their own licenses.
