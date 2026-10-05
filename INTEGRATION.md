# Integration

The project has no frontend headers, UI dependencies, or frontend globals. Pin its Git commit in the consuming repository as a submodule, then link the CMake `PSXCore::core` target or the PS2SDK `build/libpsxcore.a` library.

## Portable C API

Headers under `include/` expose:

- `psxcore_vcd.h`: structural VCD header validation. A valid header does not establish complete disc integrity or playability.
- `psxcore_request.h`: builds and validates a versioned, pointer-free ATA/APA request. Version 1 remains a dry-run contract; it does not itself launch a game.
- `psxcore_elf.h`: bounded ELF validation for the original child memory layout.
- `psxcore_boot.h`: full runtime-layout validation and SHA-256 helpers.

The existing `np_`, `Np`, and `NP_` C identifiers and numeric request magic are preserved to avoid changing validated layouts. Public header names and produced artifacts use PSXCore. C++ callers should include the C headers inside an `extern "C"` block.

## Experimental runtime bootstrap

`runtime/main.c`, `runtime/hooks.S`, and `runtime/transfer.S` contain the startup, optional tracing/adaptation, and low-address transfer implementation. `make runtime` generates the required memory layout from the installed PS2SDK linker script and builds a separate ELF. It does not link the bootstrap into a consuming application's portable core library.

The bootstrap requires `host:/POPS.ELF`, `host:/IOPRP252.IMG`, and `host:/psxcore-runtime-mode.txt`. The mode file must contain a recognized mode followed by a single LF newline: `stock` uses the existing IOP services; `image` uses the verified IOP image. Missing or invalid configuration is rejected. HDD and game modes are experimental, depend on explicitly prepared native APA/PFS content, and have no bundled setup tools in this repository. The existing implementation contains version-specific offsets and must only execute its checksum-verified runtime profile.

The consuming application owns discovery, storage mounts, artwork, saves, settings, UI, and launch lifecycle. Connecting its dispatcher to PSXCore and supporting additional storage formats remain separate integration work. Submodule inclusion alone does not provide a finished launch path.

## Inventory implementation

The optional inventory ELF uses the read-only block guard in `iop/readonly.c` and the scanner in `src/probe.c`. It does not execute POPS. Runtime modules are obtained from the installed SDK during the build.
