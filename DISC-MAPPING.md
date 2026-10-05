# Prepared-disc mapping and launch preparation

A library file and a prepared native disc are associated by the SHA-256 of their **complete, byte-identical images**, including the native header. Filenames and display names are not mapping keys. No serial-only matching, partition-name guessing, conversion, copying or partition creation occurs during selection.

The catalog has this format:

```text
PSXCORE-DISCMAP 1
1111111111111111111111111111111111111111111111111111111111111111 hdd0:PP.GAME_A game_a isolated SCUS_000.00
2222222222222222222222222222222222222222222222222222222222222222 hdd0:PP.GAME_B game_b isolated -
```

The example hashes are placeholders. Replace them with hashes measured from the actual complete images. Fields are SHA-256, prepared native partition, stable save key, `isolated` or `shared`, and declared serial (or `-` when unidentified). Keep the save key when renaming or moving a library file. The save partition is derived as `hdd0:PP.PSXC.<save_key>`; both cards must already be provisioned there. Serial metadata is schema-checked, not independently decoded from the disc by the resolver. Use `-` until it has been established from validated disc content.

Up to 32 entries and 4096 bytes are accepted. Use one ASCII space between fields and a final newline. LF and CRLF work. Empty lines, comments, embedded NULs, unsupported versions, malformed fields, duplicate image hashes and duplicate disc partitions are rejected. Disc partitions using the reserved `PP.PSXC.` save prefix are rejected. Reusing a save key is accepted only when **all** its catalog entries explicitly say `shared`; an isolated/shared collision is rejected before storage is accessed. Sharing permission is limited to this supplied catalog, not a global ownership database.

## Portable API

Include `psxcore_discmap.h` and `psxcore_hash.h` and link the portable PSXCore library:

1. Parse the complete bounded catalog with `np_disc_map_parse`.
2. Structurally check and completely hash the selected library image using the streaming SHA-256 interface. Compute SHA-1 over exactly its first 256 KiB for the existing native header job.
3. Find its unique entry with `np_disc_map_find`.
4. Read and hash the prepared partition's `/disc/disc0`, then compare both freshly measured `NpDiscIdentity` values with `np_disc_map_match`.
5. Validate the assigned save storage, runtime files and frontend ownership policy.
6. Build the 224-byte v2 request with `np_disc_map_launch`, supplying the platform and compatibility options. A NULL options argument selects the initialized Automatic settings. `np_launch_argument` then produces the prepared native disc argument.

These APIs do no IO, provision no storage and start no runtime. Parsing, lookup and request building clear outputs on failure. Keep input and output storage separate. A matching supplied digest alone is insufficient; the frontend must measure both actual files and keep the association stable through handoff. Hashing verifies association, not complete TOC/track integrity or playability.

## Frontend responsibilities

The consuming frontend owns catalog loading, source discovery, storage mounts, IO and runtime handoff. The native target must already contain the byte-identical `/disc/disc0` image. Before publishing a launch request, verify both complete images, the assigned existing card pair and the supported runtime fingerprints. Keep save identities stable and enforce sharing consistently across the frontend's ownership records.

Use only mounts owned by the verification operation, preserve other backends' mounts, and reject preparation when an owned temporary mount cannot be released. Source, disc and card verification should use read-only access. This portable package supplies no frontend-specific configuration, UI or storage adapter.

Only publish the checked request after all frontend readiness checks succeed. Request-file transport, IOP reset and transfer to the separate bootstrap remain the frontend's responsibility; constructing the request does not start the runtime.

Read-only readiness does not prove future write permission, free space, crash recovery, save persistence or power-loss durability. The runtime's mount and save-worker guards remain required. Revalidate after any source/catalog/storage change; do not cache readiness across future launches.
