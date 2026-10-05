# Integration

The project has no frontend headers, UI dependencies, or frontend globals. Pin its Git commit in the consuming repository as a submodule, then link the CMake `PSXCore::core` target or the PS2SDK `build/libpsxcore.a` library.

## Portable C API

Headers under `include/` expose:

- `psxcore_vcd.h`: structural VCD header validation. A valid header does not establish complete disc integrity or playability.
- `psxcore_request.h`: builds and validates a versioned, pointer-free ATA/APA request. Version 1 remains a dry-run contract; it does not itself launch a game.
- `psxcore_elf.h`: bounded ELF validation for the original child memory layout.
- `psxcore_boot.h`: full runtime-layout validation and SHA-256 helpers.
- `psxcore_launch.h`: version-2 native-partition launch request validation and argument construction.
- `psxcore_compat.h`: independent compatibility overrides and effective profile/capability resolution.
- `psxcore_patch.h`: bounded, transactional patch-set construction and application.

The existing `np_`, `Np`, and `NP_` C identifiers and numeric request magic are preserved to avoid changing validated layouts. Public header names and produced artifacts use PSXCore. C++ callers should include the C headers inside an `extern "C"` block.

## Experimental runtime bootstrap

`runtime/main.c`, `runtime/hooks.S`, and `runtime/transfer.S` contain the startup, optional tracing/adaptation, and low-address transfer implementation. `make runtime` generates the required memory layout from the installed PS2SDK linker script and builds a separate ELF. It does not link the bootstrap into a consuming application's portable core library.

The bootstrap requires `host:/POPS.ELF`, `host:/IOPRP252.IMG`, and `host:/psxcore-runtime-mode.txt`. The mode file must contain a recognized mode followed by a single LF newline: `stock` uses the existing IOP services; `image` uses the verified IOP image. Missing or invalid configuration is rejected. HDD and game modes are experimental, depend on explicitly prepared native APA/PFS content, and have no bundled setup tools in this repository. The existing implementation contains version-specific offsets and must only execute its checksum-verified runtime profile.

The consuming application owns discovery, storage mounts, artwork, saves, settings, UI, and launch lifecycle. Connecting its dispatcher to PSXCore and supporting additional storage formats remain separate integration work. Submodule inclusion alone does not provide a finished launch path.

## Version-2 native launch

Write exactly `launch\n` to `host:/psxcore-runtime-mode.txt` and exactly one 224-byte `NpLaunchRequest` to `host:/psxcore-launch.bin`. The bootstrap rejects extra bytes, truncated requests, unknown modes, embedded NULs in mode files, and CRLF mode endings. It validates the request and checksum-pinned Sony runtime before resetting the IOP. This HostFS transport is an experimental bootstrap interface; a frontend still needs to supply the files and launch the separate ELF.

Initialize requests using `np_launch_init`, then fill:

| Field | Contract |
| --- | --- |
| `runtime` | `NP_RUNTIME_KNOWN`, initialized automatically; the bootstrap independently verifies the runtime files. |
| `environment` | `NP_ENV_PS2` or `NP_ENV_PCSX2`, supplied by a trusted frontend configuration. No automatic platform detection exists. |
| `disc_partition` | Existing APA partition such as `hdd0:PP.GAME`, containing the validated native `/disc/disc0` image. The partition name requires a dot followed by a nonempty suffix. |
| `save_key` | Stable, persisted identity: 1–20 ASCII letters, digits, underscores or hyphens. A filename change must retain this key. |
| `save_partition` | Existing, distinct partition named exactly `hdd0:PP.PSXC.<save_key>`, with the appropriate native `/ps1emu` layout. |
| `save_mode` | `NP_SAVE_ISOLATED` or `NP_SAVE_SHARED`. Shared mode records explicit intent; the frontend must enforce ownership and sharing. |
| `serial` | Empty for unidentified discs, otherwise the validated `AAAA_000.00` disc serial. |
| `disc_sha256` | Full SHA-256 of the native `/disc/disc0` image, including its header; 32 raw bytes. |
| `header_sha1` | SHA-1 of the image's first 256 KiB; 20 raw bytes. Retains the runtime's existing verification-worker signaling. |
| `options` | Compatibility mode, explicit enable/disable masks, and diagnostics 0 or 1. |

Integer fields use little-endian encoding on PS2. Reserved fields and padding must be zero. The struct has no pointers; its size is asserted at compile time. Non-PS2 consumers must serialize integer fields in little-endian order rather than assuming their native struct encoding.

`np_launch_check` validates structure and resolves options; it does **not** open partitions, verify disc hashes, confirm saves exist, or provision storage. `np_launch_argument` produces `hdd0:PP.GAME:/disc/disc0` after validation. Ordinary `.VCD` files inside a shared library are not automatically mapped to this native path.

The frontend must validate the actual selected native image, compute both hashes from that image, obtain its serial from validated disc content, and keep that image/partition association stable through launch. The bootstrap's native header job checks only the header. A supplied full digest selects a profile but is not runtime proof that the mounted disc matches it. Until that association is independently verified, automatic profiles are experimental metadata-driven selections.

A C11 frontend can assemble a request after completing those checks:

```c
#include "psxcore_launch.h"
#include <string.h>

/* Hash buffers and serial come from validation of the prepared native image.
 * The save identity and both partition names come from persisted ownership. */
const char *prepare_request(NpLaunchRequest *r, uint32_t environment,
    const unsigned char disc_sha256[32], const unsigned char header_sha1[20]) {
    np_launch_init(r);
    r->environment = environment;
    strcpy(r->disc_partition, "hdd0:PP.GAME");
    strcpy(r->save_key, "game_us_rev1");
    strcpy(r->save_partition, "hdd0:PP.PSXC.game_us_rev1");
    memcpy(r->disc_sha256, disc_sha256, 32);
    memcpy(r->header_sha1, header_sha1, 20);
    /* Empty serial intentionally selects no game-specific automatic profile. */
    NpCompatResult effective;
    return np_launch_check(r, sizeof(*r), &effective);
}
```

On little-endian hosts, write the validated request using binary I/O and check both the write and close results. Publish the completed request before starting the bootstrap; do not modify it during startup. Dynamic strings must be bounded before copying into the fixed fields. The example uses fixed strings that fit.

## Compatibility options

Resolution order is baseline, matching experimental profile, then Custom overrides. `NP_COMPAT_AUTO` enables matching profiles; `NP_COMPAT_DEFAULT` retains required native launch adaptations while suppressing optional profile fixes. `NP_COMPAT_CUSTOM` starts from Automatic and applies `enable` and `disable`. Reject overlapping masks, unknown fix bits, explicit overrides outside Custom, and unavailable enabled fixes. Use `np_compat_resolve` or the result from `np_launch_check` to display the actual effective mask, implemented capability mask, profile name, and revision.

| Fix | Implementation and boundary |
| --- | --- |
| `NP_FIX_LICENSE` | Guarded adaptation of an existing firmware license-string slot, requiring the existing native-sector fingerprint. Implemented experimentally for both platform values; not a general region fix or a hardware compatibility claim. |
| `NP_FIX_READBACK` | Restores a native readback DMA descriptor after FIFO writes. Available only for the PCSX2 environment; console requests reject attempts to enable it. |

Revision 1 contains one automatic profile: the exact previously tested converted US Spyro image with serial `SCUS_942.28` and its full native-image SHA-256, under PCSX2. Unknown identities and console requests receive baseline behavior. Custom can disable either fix or enable an implemented platform capability. These two independent fixes currently have no declared dependency on each other. There are no implemented disc-read, controller, overclock, frame-skip, or audio-speed controls.

`options.diagnostics = 0` suppresses optional resident interface trace output. Bootstrap configuration summaries, rejection reasons, and fatal initialization/save errors remain available. Startup logs report the original selected mode and actual prepared patch count, as well as the effective fixes for native requests.

## Save routing and ownership

The checksum-guarded first native mount call redirects `pfs1:` from the shared `__common` location to the requested resident save-partition string. Cards cannot initialize before that save mount succeeds. Native mount failures, failed disc initialization, or absent native card-ready flags halt the new path, without retrying against shared cards. The pinned card initializer returns `1` on several failure paths too, so a successful-looking return alone is insufficient: the hook also requires both slot-ready bits, set after each 128 KiB read and native header validation. Legacy modes retain their existing behavior, including shared paths.

The pinned native paths are `pfs1:/ps1emu/card0`, `card1`, and corresponding `card0.bak`/`card1.bak` files. Those paths follow the selected save mount. Required v2 guards check open, exact full-card write length, close, rename, synchronous flush, semaphore creation/wait, and thread creation/start results. A synchronous flush of the closed backup precedes replacement of the canonical card. The native worker clears its dirty flag only after the guarded final flush returns successfully. These guards are required in Automatic, Default and Custom modes; legacy modes retain their existing worker behavior.

Any guarded failure latches its stage/result, forces a diagnostic even when tracing is disabled, and stops EE scheduling before returning to the worker. Stage numbers are 1 open, 2 write, 3 close, 4 rename, 5 final flush, 6 semaphore creation, 7 thread creation, 8 thread start, 9 semaphore wait, and 10 backup flush before replacement. `save=flush-completed` reports a successful synchronous native flush return, not power-loss durability. A final-flush failure occurs after replacement and cannot undo it. This fatal guard supplies no recovery, frontend completion acknowledgement, or graceful shutdown; asynchronous game code may already have acknowledged the save.

Local PCSX2 tests created two isolated card pairs, cold loaded distinct diagnostic contents in A–B–A order, and rejected an invalid card despite native return `1`. A separate copied disk/profile then queued original tail markers through the actual native worker. Both full-card writes were read back from storage; unrelated cards were unchanged. An injected backup-flush failure stopped before replacement, kept dirty flags set, and preserved both canonical cards. A fresh process using the production bootstrap loaded the last successful markers. Linked instruction tests separately cover all guarded failure stages, partial writes, ABI preservation and legacy behavior. These tests use synthetic contents and forced host termination; game-driven saving, graceful shutdown, all live failure stages, power-loss durability, broader compatibility and physical-console support remain unverified.

No partition creation, save migration, retained backup history, capacity checks, ownership database, or frontend save-aware exit is implemented here. The native `.bak` file is temporary replacement staging, not a retained copy of the previous save. A frontend must persist and enforce identity ownership, explicitly approve shared identities for related discs, and provision/validate save storage before launch. Evaluate the APA allocation overhead before adopting one partition per game as the final design. General directory routing requires separate verified runtime work.

## Staged patches

Start with a zero-initialized `NpPatchSet`. Add bounded changes with `np_patch_add`, then call `np_patch_apply` once on the staged image after checking the runtime fingerprint and ELF layout. Application validates every expected byte, file/segment bound, and virtual/file overlap before changing any byte. A failing final patch leaves earlier patches unapplied. Runtime files on disk remain unchanged. The patch engine is not itself an ELF validator, runtime identity check, or registry of verified game compatibility.

## Inventory implementation

The optional inventory ELF uses the read-only block guard in `iop/readonly.c` and the scanner in `src/probe.c`. It does not execute POPS. Runtime modules are obtained from the installed SDK during the build.
