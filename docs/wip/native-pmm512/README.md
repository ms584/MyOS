# Private PMM512 slice, not promoted

## Current continuation (2026-10-08 06:02 +07)

This section supersedes the earlier live/fatal hypotheses below.
The separate safe-pointer fix requires real PMM-owned frames, retaining U/S,
RW, canonical and whole-range checks. BIOS/UEFI280MiB every-page random/log
copy, RO/NONE/supervisor/preflight and two-launch exact cleanup PASS.
Full22674 failed absent ABI cache; full7014 failed absent pinned Cobalt core.
Normal restored, failure retained; retry2759 pending with both caches verified.
Combined SHM8/direct full47797 already PASS; no PMM/source promotion yet.

Native HTML/DOM/timers/key/click actually execute (hfcgru2i/n693w3zt), but long
refresh final Timer96/Clicks2 trails156timers/3clicks. No responsive60s claim.
Normal boot CPU probes remain alive indefinitely. Private retirement preserves
PROCESS_TESTS=1 stress, normal boots reap only probes after checks. BIOS/UEFI
counter-stop/timer/shell/resource focus PASS; c3sthg1e first background186s->95s,
DOM126.49s->74.62s. Final Timer114/Keyc still trails131; no click callbacks
this run. CPU-clock subtest absent in old diagnostic clone, NOT a CPU PASS.

Actual TLS IP URL rejected ERR_CERT_NON_UNIQUE_NAME,0HTTP; deliberately stopped
after concrete navigation failure. New DNS SAN attempt uses documented DNS-only
mapping cobalt.example.com->10.0.2.2 and enforcement=true. Disposable public CA
is loaded via actual hashed cert assets; no verifier bypass. Negative certs and
concurrent native assets pending. NOT external/public-CA HTTPS/full gate proof.
Never commit generated private keys/whole TLS directories. Close/relaunch,
RAMFS cache/resource accounting, scrolling and responsive rendering remain open.

## Historical PMM diagnosis

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
Negative-read18382 terminal0 observation05:04:35, same abort/no negative trace.
EOF97279 terminal0:l24t5y7t and identity75223 terminal0:fcp0dbb7; same fatal.
Successful random reads from other pthreads do not prove failing caller's path.
No acceptance. Full22674 now RUNNING after old combined full47797 terminal0.

memory-512m-20261008.c and test/runner are reproducible WIP sources, not proof
of complete Cobalt or Phase33 closure. PROJECT_STATE/HANDOFF describe live jobs.
