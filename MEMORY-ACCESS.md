# Experimental generated-memory adapter

`NP_MEMORY_ACCESS_ADAPTER=1` enables an experimental POPS adaptation only for
native v2 requests declaring the PCSX2 environment. Its default remains `0`.
The supplied POPS fingerprint, ELF layout, both emitter epilogues and their
delay instructions must pass before the transactional patch set is applied.
Runtime files and the separately supplied test host remain unchanged.

The scalar emitter at `0x205be8` and grouped emitter at `0x206198` reserve two
NOPs after supported memory operations. Their epilogues at `0x205d4c` and
`0x206358` now finalize those reservations into explicit resident helper calls.
Helpers dispatch MMIO through the existing native handlers and retain their
guest-cycle charges. RAM, scratchpad and BIOS reads retain direct accesses;
unknown addresses stop with a diagnostic. Merge operations and native
stack-based emission remain unchanged and are not covered by this adaptation.

The grouped path validates at most eight native descriptors and their
allocated registers, exact generated instructions, every NOP reservation and
resident helper targets before publishing any word. It preserves group order,
cursor and compiler callee state. EE register 30 is accepted because POPS pins
the PS1 return register there. A malformed final entry leaves the whole group
unchanged. Publication uses aligned word stores.

`NP_MEMORY_ACCESS_TRACE=1`, with the adapter enabled, adds six opt-in guest
WaitSema observers. Their call and delay instructions are checked before
installation. Each records entry/return counts, semaphore ID and original
caller, preserving arguments, wide results and the original syscall. The
default is `0`. Observations have overhead and are not atomic across threads.
They do not alter queue policy, signaling, CPU clocks or completion behavior.

Local C and linked-instruction checks cover scalar accesses, mixed groups,
native-only paths, register 30, atomic rejection, RAM/MMIO guards, epilogues and
observer preservation. Stock-host cold boots at nominal clocks have exercised
both emitters without logged TLB misses or adapter rejections, but subsequently
stalled at the POPS video completion wait with an empty graphics queue. This
does not establish successful gameplay, speed, audio, saves or console support.
Both build flags must remain opt-in pending further guest-runtime work.
