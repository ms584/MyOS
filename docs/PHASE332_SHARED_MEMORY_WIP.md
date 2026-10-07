# Phase 33.2 shared memory WIP

Updated12:34 +07,2026-10-07. Diagnostic/private code; not main promotion.
Actual Cobalt requires /dev/shm temporary files and MAP_SHARED|RW; see
PHASE332_VOLUME_AUDIO_WIP.md. Increasing exports alone cannot satisfy it.

## Implementation
- PMM owns one reference per allocation; page_retain adds reference, page_free
  releases one and makes frame available only when last reference disappears.
  uint16 overflow rejects retain; metadata is heap allocated after bootstrap,
  not low-memory BSS. Existing exclusive boot allocations initialized to1.
- Dedicated /dev/shm page-backed files up to1MiB, zero-filled, with atomic
  growth allocation rollback. Normal RAMFS files remain64KiB heap-backed.
- FD I/O and shared aliases use same physical frames, not copied mmap data.
  Aliases retain frames independently of node/name/fd; process VM ledger owns
  each mapping reference and releases it through ordinary unmap/stop cleanup.
- Readonly descriptor imposes write ceiling; private copies independent.
  Execution unsupported. Whole pages beyond EOF rejected until SIGBUS exists.
- Mapped shrink/O_TRUNC explicitly EBUSY, no invalidation/SIGBUS yet.
  Dedicated subtree omitted from v1/v2 persistent snapshot export; format
  unchanged. No demand paging/COW/general writable-file-map claim.

## Evidence
- shm-unit-D8Tyjd ASAN/UBSAN terminal0 12:26:11: real backing code; mock
  privileged physical allocator boundary only.256KiB/zero-fill/cross-page
  coherence/retained aliases/growth OOM rollback/exact frame releases.
- cd-QJYMwG pmm-refs.status0 at12:28:02: actual native16/64/256MiB bootstrap,
  retain/invalid retain, alias lifetime, paging/exhaustion recovery.
- Native shared-memory-f9pps4rn BIOS16/64MiB PASS: four launches/profile,
 256KiB, fd/alias coherence, readonly ceilings/private copies, unlink/close/
  process-stop lifetime, four workers and exact page/heap restoration.
- UEFI16MiB prekernel failure preserved; no serial output/fixture execution.
  Retry unchanged binary64/256MiB terminal0 12:34:26: PASS,0pqiocf9.
- RAMFS shared integration ASAN/UBSAN k4Wzg1 terminal0 12:37:15: v1/v2
  excludes ephemeral subtree, import guard, normal64KiB bounds, heap/page
  allocation rollback, O_TRUNC mapped-shrink rejection and exact releases.
- Genuine startup-lu6q4q7t confirms actual262144/4-byte shared mappings succeed;
 0HTTP. Extra trace-zyc7sv42 terminal0 12:41:45 shows8worker capacity exhausted
  (six EAGAIN), then ImmediateCrash. This is diagnosis, not browser acceptance.
- Build kernel247136 bytes/483sectors, within256KiB/low-memory link assertions.

## Pending
- Capacity16 and heap FPU metadata scope now LIVE77918 at12:48:24.
- Native actual pinned Cobalt rerun (no patched engine or fake success).
- Clean-runtime full architectural regression for all new changes before
  promotion. Frozen fm-prC903 full excludes this batch; cannot prove it.
- Five actual browser acceptance gates all unproven. No33.3, no push.
