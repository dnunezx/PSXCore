# POPS/PSXCore change inventory

Compatibility investigations, fixes and instrumentation target POPS/PSXCore only. Never troubleshoot, instrument, build or patch PCSX2. Use an unchanged, separately supplied test host and existing public interfaces to observe guest state.

| Change | Actual target | Current state |
| --- | --- | --- |
| Shared texture wrapping | POPS texture-window handler, installed by `src/runtime_corrections.c` | Required core correction on the verified runtime in every mode. Broader games and physical hardware remain unverified. |
| Readback descriptor restoration | POPS DMA-start path, redirected to `runtime/hooks.S` | Optional guarded PSXCore adapter. Its host-environment condition is selection metadata, not an emulator patch. |
| Startup/license/disc adaptations | Verified POPS staged code/data and PSXCore bootstrap | Optional guarded runtime adaptations; supplied input files remain unchanged. |
| Save-worker guards | POPS worker redirects and PSXCore adapter code | Required checked launch/save-routing protections. |
| Profile selection and transactional patching | PSXCore source | Exact revision/identity/environment selection and expected-byte, bounds and overlap checks. |

The packed RGB24 intro repair, memory-exception repair, EE profiler and GS tracer were PCSX2 changes. Their patches, copied source fixtures and custom-host tools have been removed rather than incorporated into PSXCore. No equivalent intro or CPU repair is claimed here. Previous modified-host results are historical evidence only; stock-host and physical-console validation remain pending.

The retained tools inspect POPS/PSXCore guest counters, runtime identity, ELF metadata and saved scenes. `state_patch.py` changes a checked POPS instruction in a new guest-state copy; it does not patch an emulator. Profiles and optional runtime corrections are maintained under `compatibility/`; shared corrections belong in the core runtime path.
