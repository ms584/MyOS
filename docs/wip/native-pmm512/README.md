# Private PMM512 slice, not promoted

Derived from frozen shm-8m-4hJG2I with corrected server/socketpair fixtures,
VM near hints, runtime64/kernel32 descriptors and actual SHM8MiB direct mapping.
Only memory.c differs from that clean tree. Main still uses PMM256MiB.

Reason: actual Cobalt bljhqeq7 returns ENOMEM for2MiB backing,36KiB mmap,
32752-byte malloc, followed by NULL+0x18 fault. No visible fixture or input.

The new allocator keeps bootstrap256MiB bitmap storage unchanged to respect
BIOS kernel/BSS reservation. Once the kernel heap exists, new bitmap storage
and reference metadata are allocated there; actual upper E820 usable pages
are discovered, excluding reserved regions, with a bounded512MiB maximum.
No new paging policy, swap, demand paging or access-right relaxation.

Focus28058 terminal0: actual BIOS16/256/512 + UEFI64/256/512, full PMM exhaustion
and recovery; frame above256MiB mapped/written/translatable with exact retain,
release and page-table recovery. Counts recorded in pmm512-{bios,uefi}.log.
Full regression pending. Native visual95819 terminal0 observation04:59:33,
n16qdlc8: GET only; new signal6, no visible HTML/input. Cached random-file
read failure suspected from actual disassembly, errno not yet known.
Private diagnostic18382 running05:02:34; NOT accepted Cobalt.

memory-512m-20261008.c and test/runner are reproducible WIP sources, not proof
of complete Cobalt or Phase33 closure. PROJECT_STATE/HANDOFF describe live jobs.
