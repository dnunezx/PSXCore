# Pinned POPS video progress: first trace

The guest ELF is SHA-256 `59df3389c4df88a572daa720b05507c52c34eddfa0031a6fbeec55e0c2d0fcb1`.
This map is for that revision only. Addresses below are guest EE addresses.

## Control flow

- `0x20b418` is the only direct call to `0x21aeb0`; its caller address on entry is `0x20b420`. The surrounding `0x20b410` function is used as a callback.
- `0x21aec8` waits on the semaphore stored at `0x70003c0c` (11 in the captured scene). POPS ignores the native wait result for its branch decision.
- After a return, `0x21aed0` exits if `0x7000373c != 0`; `0x21aedc` exits if `0x70003304 == 0`.
- Otherwise `0x21af00` loops to the wait while `0x70003358 == (0x7000334c == (0x700032b8 & 1))`. Inequality releases the loop, then the common exit calls `0x21a568`. The branch and its delay slot are retained by the trace.
- `0x21cd68`, registered during video setup, samples the GS CSR field into `0x7000334c` when `0x7000373c == 0`, then signals semaphore 11 through `0x22e880` and `0x22e850`. The graphics worker is not the sole possible signal producer. The guest command path at `0x221aa8` can also signal semaphore 11 when a display-mode bit changes.

## Tested state writers

| Scratchpad address | Pinned guest writers | Observed role |
| --- | --- | --- |
| `0x7000373c` | `0x2220b4`, `0x2220e4`, `0x22262c` | Video path state flag used for early exit and CSR sampling |
| `0x70003304` | `0x21b168`, `0x21c238`, `0x221ad4`, `0x221ae0`, `0x221ae8` | Display state flag used for early exit |
| `0x700032b8` | `0x21b160`, `0x21c248`, `0x221914` | Mode value whose low bit participates in the comparison |
| `0x7000334c` | `0x21cda0`, `0x221c10` | CSR-derived or toggled phase value |
| `0x70003358` | `0x221b70`, `0x221c38` | Latched comparison value; `0x221c38` derives it from the mode and phase values |
| `0x70003348` | `0x221b7c` | Increments in the command-dispatch path; not a validated frame or gameplay metric |

Initialization also clears or copies these fields in the pinned video setup. The writer list is a direct-store map; indirect writes and DMA need separate verification before claiming complete coverage.

## Bounded, default-off trace

With `NP_MEMORY_ACCESS_ADAPTER=1` and `NP_MEMORY_ACCESS_TRACE=1`, the runtime now counts routine entries, loop backs and exits and stores the latest 64 decision/exit records. Each record has a begin/end sequence pair; the observer discards incomplete or overwritten reads. It captures the exact branch operands, a sampled field set, caller, wait-return count and low 64 bits of the native wait result. The wait wrapper preserves the complete native return register. The trace uses one writer in the video routine and no printing or blocking operation in the hot path. Existing defaults remain zero.

The first run is `tekken-test/memory-test/grouped-20261005-183634/`, bootstrap SHA-256 `e175ed0f5b083030f46bbfbb774842949d0eeb3a95115bc815df02e948f72e20`, scene slot 127. At 19.965 seconds, the prior counter is 675; routine entries 676, exits 675, loop backs 278, video waits 954 and returns 953. The latest 64 complete records are all loop backs within entry 676. In those records, `0x7000373c=0`, `0x70003304=1`, `0x700032b8=0`, `0x7000334c=0`, and `0x70003358=1`. Thus the comparison is `1 == 1` on every recorded return. Native wait results are 11. This is an internal predicate loop, not repeated outer calls or a permanently blocked semaphore wait.

The command counter plateaus with the loop. Its producer at `0x221b7c` establishes only command-path activity. The worker wait count also plateaus. The single log `TLB Miss` occurred at BIOS PC `0xbfc00000` before PSXCore launched, while the external observer had begun polling; it is not evidence of an adapted guest access failure. All checked game, firmware, host, disk and settings hashes matched before and after; the owned process exited. Visual gameplay acceptance is still open.

## Next diagnosis

Trace or otherwise verify the producer of the phase and latched values at `0x21cd68` and `0x221b58`/`0x221c38`, including the guest-visible GS CSR bit and command traffic at the transition. A repair needs evidence for why the expected phase inequality never appears. Do not force a signal or skip the comparison.
