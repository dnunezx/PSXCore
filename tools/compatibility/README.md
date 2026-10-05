# Compatibility tools

Generic, local diagnostics for developers and agents investigating game compatibility. Python 3.10+ is required. No game, firmware, BIOS, runtime, disk image, savestate or personal configuration is included. Diagnostic outputs and privately supplied inputs belong in ignored `output/` and `private/` directories. This folder follows the repository license.

See [TIMING.md](TIMING.md) for guest clock observations and [GRAPHICS.md](GRAPHICS.md) for texture evidence. The [automatic compatibility catalog](../../compatibility/README.md) is maintained through `generate_profiles.py`; contributors edit reviewed profiles and optional POPS corrections, then regenerate and rebuild. Shared behavior such as texture wrapping belongs in [core runtime corrections](../../RUNTIME-CORRECTIONS.md), independent of game profiles. Run the synthetic Python suite with `python -m unittest discover -s tools/compatibility/tests -v` from the repository root.

**Policy: investigate and fix POPS/PSXCore only. Never troubleshoot, instrument, build or patch PCSX2.** Emulator patches, host regression fixtures and custom-host profiling tools have been removed. Existing public interfaces may inspect guest state. If a host limitation blocks a test, record it and continue within POPS/PSXCore. Historical modified-host results do not establish stock-host support.

## Memory counters

Enable PCSX2 PINE explicitly in an isolated test profile. Supply its port and a JSON field description. `observe.py` only reads aligned 32-bit guest addresses through loopback. It distinguishes monotonic counters from changing state values, reports counter discontinuities and bounds recording to 60 seconds. Validate address mappings against the exact runtime/game revision before attributing meaning to fields. Samples are not atomic and polling adds overhead.

```json
{"fields": [{"name": "validated_counter", "address": "0x00000000", "kind": "counter"}, {"name": "validated_state", "address": "0x00000004", "kind": "state"}]}
```

These addresses are placeholders, not a compatibility profile.

```text
python observe.py --port PORT --fields private/fields.json --output output/counters.json
```

## Preserve and inspect a scene

`state.py capture` writes one unused emulator savestate slot. It refuses an existing slot, waits for completion and verifies the archive with an explicitly supplied local 7-Zip installation. Capturing briefly pauses the emulator and must not be part of a baseline speed measurement. ZIP metadata alone does not prove a compressed state is intact. Do not commit captures; they contain proprietary and user data.

```text
python state.py --sevenzip PATH_TO_7Z capture --port PORT --directory STATE_DIRECTORY --slot UNUSED_SLOT
python state.py --sevenzip PATH_TO_7Z inspect --state STATE_FILE --output output/state-metadata.json
```

The initial register metadata decoder is version-specific. Verify PCSX2's `cpuRegisters` layout and savestate tags before using another version. A saved PC is an emulator boundary observation, not an unbiased instruction profiler.

## Investigation rules

Record exact source/build hashes and settings. Start at nominal guest clocks, retain a reproducible scene and change one variable at a time. Separate host speed, game frames and emulated interrupt delivery. Keep measured facts separate from interpretations. Validate proposed POPS/PSXCore fixes against expected runtime bytes and appropriate original regression fixtures. If a host-emulator limitation is found, document it and investigate a POPS/PSXCore adaptation within this policy. Overclocking is not a compatibility fix. Physical-console acceptance requires separate evidence.

See [PATCHES.md](PATCHES.md) for the remaining POPS/PSXCore changes and evidence limits. Conversion/provisioning harnesses remain private until generalized. Reviewed selection manifests belong in `compatibility/profiles/`; reusable diagnostics belong here. User-specific emulator profiles and private fixtures stay excluded.

`elf_metadata.py` accepts a privately supplied MIPS ELF32 file, its required SHA-256, an aligned address range of at most 64 KiB and optional field offsets. It reports static references and opcode counts without exporting instruction bytes. Indirect calls and generated code are not resolved.

```text
python elf_metadata.py --elf private/runtime.elf --sha256 EXPECTED_HASH --start START --end END --output output/interface.json
```
