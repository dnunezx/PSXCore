# Automatic compatibility profiles

Profiles select tested POPS/PSXCore fixes at boot. Only the verified staged POPS image in PS2 RAM is changed. The initial profiles are experimental, tested on a local PCSX2 build containing historical host modifications; physical-console acceptance remains unverified. No PCSX2 modifications are part of this feature.

Selection matches runtime revision, execution environment, canonical serial, and full SHA-256 of the prepared native disc image. The digest distinguishes revisions/conversions sharing a serial. Filenames are never identity. The frontend must measure the selected image and verify its association with the prepared partition. The bootstrap verifies runtime SHA-256/ELF layout; its disc check covers the header, not a second full-image read. See [the integration contract](../INTEGRATION.md).

| Profile | Exact identity | Tested fixes | Scope |
| --- | --- | --- | --- |
| [Doom USA](profiles/doom-us.json) | `SLUS_000.77` plus manifest image hash | License, readback | PCSX2 only |
| [Spyro USA](profiles/spyro-us.json) | `SCUS_942.28` plus manifest image hash | License, readback | PCSX2 only |

Automatic uses matching profiles. Unknown revisions receive baseline behavior, including [shared POPS core corrections](../RUNTIME-CORRECTIONS.md). Default suppresses optional fixes while retaining core corrections and required native adaptations. Custom starts from Automatic and applies explicit masks, rejecting contradictory or unavailable fixes. Texture wrapping is core behavior and cannot be disabled by these options. Logs report profile ID, catalog revision, optional mask, game, and exact-image revision description, plus a separate core-correction summary. Console requests do not inherit PCSX2-only game profiles; the core correction applies to the supported runtime under both platform values, with physical-console validation still pending. No timing or overclock fix is enabled.

## Contributing

1. Test the POPS/PSXCore change at nominal CPU settings. Record fingerprints, serial, environment, expected bytes, results, and limitations. Exclude firmware, games, BIOS, captures, personal paths, and user names.
2. Add a reviewed manifest under `profiles/`, following the examples. List only fixes tested together on that exact image/environment, with evidence and limitations. Different hashes need separate profiles.
3. For bounded byte corrections, add a manifest under `patches/` with a unique power-of-two bit, `NP_FIX_*` symbol, virtual addresses, and equal-length expected/replacement bytes. License/readback refer to existing adapters; new hooks require original PSXCore code and validation.
4. Run `python tools/compatibility/generate_profiles.py` from PSXCore and include both generated headers. Run it with `--check`, the compatibility Python suite, CMake/CTest, and a PS2SDK runtime build. Test a cold boot; saved states can bypass selection.
5. Rebuild the library/bootstrap and frontend. Manifests compile into a reviewed catalog; copying JSON into an emulator does not activate it. Runtime folder scanning/script loading is not implemented.

The generator rejects duplicate identities/bits, unsupported runtimes, untested environments, unknown fixes, malformed hashes, oversized writes, and overlapping corrections. Application checks every expected byte and all bounds/overlaps before changing any byte. Failure aborts launch without partial application. Runtime-wide changes still require environment evidence and interaction testing.

Generated files: `include/psxcore_profile_catalog.h` and `src/compat_profiles.generated.h`. Catalog revision 3 is exposed through `NP_PROFILE_REVISION`; increment it when publishing changed selection policy. The former wrapping bit 4 is retired and reserved. The launch request stays 224 bytes. Rebuild consumers against the updated headers. The catalog contains no proprietary payloads.
