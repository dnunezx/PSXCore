# Dependencies and attribution

Original PSXCore source and adaptations: Danny Nunez (dnunezx), 2026.

- **PS2SDK** — https://github.com/ps2dev/ps2sdk. External SDK and modules used by PS2 builds; their component licenses and attribution remain applicable. No SDK source or prebuilt module is committed here. The runtime build derives its linker layout from the installed SDK startup script and preserves that script's upstream notices in the generated file.
- **POPS2CUE/CUE2POPS** — https://github.com/bucanero/pops2cue. Public interoperability reference for VCD fields; its implementation was not copied into the core.
- **Sony POPS** — separately supplied proprietary runtime, referenced for interoperability. Not included or relicensed by PSXCore.

No closed-source launcher executable or unreleased launcher source is a build dependency. POPS/PSXCore diagnostics and runtime adaptations are cataloged in [tools/compatibility/PATCHES.md](tools/compatibility/PATCHES.md). PCSX2-derived patches and fixtures have been removed. No emulator modification is a source or build dependency. Never troubleshoot or patch PCSX2 for this project. Private fixtures, emulator binaries and project-specific development harnesses remain outside this source package.
