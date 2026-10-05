# PSXCore

PSXCore is an open-source PS1 launch and runtime-adaptation project for PlayStation 2. It began with a simple goal: make the launcher layer open for people to inspect, improve, fork, and integrate into their own projects.

This standalone repository contains the original implementation and its build files. It has no frontend dependency and can be consumed as a Git submodule. Development notes, plans, local data, test fixtures, test harnesses, and developer tools are excluded.

Sony's POPS emulator remains proprietary and **is not included**. Runtime execution requires separately supplied, checksum-verified `POPS.ELF` and `IOPRP252.IMG`. No Sony runtime, firmware, game, save, or emulator binary is distributed here.

## Status

This is an experimental source release. The portable core validates VCD headers, fixed-size launch requests, and bounded ELF layouts and provides SHA-256 verification. The separate runtime bootstrap contains version-specific startup, transfer, and in-memory adaptation code. It is not a finished end-user application; broad game compatibility, integration into a frontend, and physical-console acceptance remain to be established.

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

## License

Copyright (c) 2026 Danny Nunez (dnunezx). Original PSXCore code uses the Academic Free License 3.0. See [LICENSE](LICENSE) and [THIRD_PARTY.md](THIRD_PARTY.md). External dependencies retain their own licenses.
