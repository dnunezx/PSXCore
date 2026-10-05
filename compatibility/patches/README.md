# Optional compatibility writes

Reviewed JSON manifests in this folder describe optional, tested POPS byte corrections selected through game compatibility profiles. See [the catalog guide](../README.md). The current profiles use built-in license/readback adapters and need no JSON byte-write entries.

Texture wrapping is now a [core runtime correction](../../RUNTIME-CORRECTIONS.md) implemented in PSXCore source. Its former option bit 4, symbol `NP_FIX_TEXTURE_WRAP`, and address interval `0x0021d6a4`–`0x0021d6a8` are reserved; the generator rejects conflicting optional entries. New optional corrections must use an unused power-of-two bit and be tested with the selected profile's complete fix set.
