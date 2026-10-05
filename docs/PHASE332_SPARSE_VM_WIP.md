# Phase 33.1/33.2 — sparse virtual reservation / entropy prerequisite

Updated2026-10-05 17:24 +07. PRIVATE ONLY. Main source eb31a69 unchanged,
normal BIOS/UEFI retained. No33.3/push/phase closure/API count increase.
Private copied mutable tree /tmp/myos-sparse-vm-BCcMw4.

## Implemented scope

- Process-owned sorted interval ledger,256 disjoint regions, merging adjacent
  ownership and preflighted partial unmap/split capacity. No fake addresses.
- Original496MiB low arena retained; independent256GiB high arena
  [USER_BASE+4GiB, USER_BASE+260GiB), outside image/runtime/errno/TLS ranges.
  Generic per-process page walker supports it without sharing user page tables.
- PROT_NONE reservation needs no physical pages or page-table subtree. RW/RX
  protect commits zeroed owned pages; W+X rejected. Staged inaccessible PTEs
  become accessible only after whole commit succeeds. Allocation failure drops
  only staged pages, preserving old contents/permissions/reservation.
- Sparse subtree skipping for protect/unmap/advise; prune only owned empty VM
  page tables, never image/kernel tables. Owner release frees ledger/pages.
- mincore distinguishes reservation membership from resident physical pages;
  checked copy denies uncommitted or protected pages. PROT_NONE on already
  committed data preserves contents/residency until reprotected or unmapped.
- Fixed mapping replacement and NOREPLACE overlap, hole remapping, aligned
  overreservation/trim. Anonymous mapping still eagerly commits nonzero prot;
  no demand-page-fault/COW/file mmap claim. Physical allocator remains256MiB cap.
- Exact readonly /dev/urandom descriptor uses real checked SYSTEM_RANDOM;
  <=256B short reads, shared dup/close lifetime, flags, metadata, seek ESPIPE,
  safe bad-buffer rejection and fail-closed unsupported hardware. No generic
  /dev namespace, writable entropy device or complete POSIX device claim.

## Evidence

- Build21853 terminal0 16:51:16,239936B469sectors PROCESS_TESTS0 private only.
- ASAN/UBSAN interval source:100000 model operations, full table merge/split
  capacity failure leaves ledger unchanged;32GiB intervals and bounds.
- Native42995 terminal0 16:56:00: BIOS CPU=max, BIOS qemu64/no entropy and UEFI
  CPU=max. Three good launches/profile, deliberate uncommitted-page fault,
  Escape cancellation and relaunch in entropy profiles; exact shell pages/heap.
  Outputs sparse-vm-rpa9rint,3ae254kk,y2gpqmbi. Tests32GiB reservation leaves
  free pages unchanged, three widely separated commits consume<=12 pages,
  huge RW failure preserves data, residency/rights, partial unmap/NO_REPLACE,
  fixed zero replacement,64GiB overreserve->aligned32GiB trim and full cleanup.
- Actual process VM source ASAN/UBSAN16:59:37 /tmp/myos-vm-commit-unit-9sU9Qx:
  injected allocator failure after0..6 successful pages for protect and0..7 for
  fresh mapping; exact live page counts/ledger and old content/permissions
  preserved. Only physical allocator/heap/TLB/console boundary mocked; real
  mapping/commit/rollback algorithms compiled. Initial fixture selected stale
  quoted work/process.h; corrected angle include before counting PASS.
- Native genuine Cobalt78989 terminal0 16:57:28 CPU=max,512MiB VM (managed PMM
  capped256MiB), output startup-diagnostic-qiu8vkev. Advances beyond entropy and
 32GiB reservation. SABI18 matched; engine log app_event_delegate.cc:444 warns
  crash extension unavailable, then FATAL immediate_crash.h:193 Check false,
  USER SIGNAL pid5 signal6. Zero HTTP requests, no missing28 probe called,
  exact pages/heap90624 reclaimed. Superseded by resource trace below.
  This is not any HTML/JS/input/HTTPS/endurance acceptance; web0/5 remains.

## Resource-load blocker identified

- Abort trace13585 terminal0 17:05:12: bounded main-stack12-frame chain,
  preserving SIGABRT. Core return5ff2910 constructs cobalt_shell.pak path,
  3cabb18/3cabb46 load resource pack, failed load3cb1e62 produces FATAL
  immediate_crash.h193; not the preceding crash-handler extension warning.
- Genuine upstream pinned CRX downloaded and SHA verified:
  7c67f29455f6c6fc8e32933e0ce696a301361737889b69546735740832e6df55.
  build/cobalt-resources-zvo4x92w under private engine tree, report.json has135
  checked content entries: pak, fonts/fonts.xml,133 public SSL certs. No TTFs.
  Pak1318438B SHA998e757a344344e45481a505335e5164a5deb41081f0374bde3f8b6290b43ad5.
  Resource-enabled88686 terminal0 17:13:15 still aborts, zero HTTP.
- Resource trace86372 terminal0 17:22:11; startup-diagnostic-_igcaz9c/startup.txt:
  open /assets/cobalt_shell.pak flags0 succeeds kernel fd3/runtime fd65;
  mmap(fd65,length0x141e26=1318438,flags1=MAP_SHARED,prot1=PROT_READ,offset0).
  Runtime currently explicitly rejects non-anonymous mmap with EINVAL. This
  is a real required file-mapping gap, not absent pak or missing import name.
  Engine FATAL/SIGABRT6, zero HTTP, exact pages/heap125184. The optional
  /proc/self/stat open fails ENOENT earlier but execution proceeds to pak.
- Full2835 terminal1 17:22:14: private clone omitted third_party/compiler-rt;
  fixture compilation fails before dynamic tests. This is harness dependency,
  not evidence of runtime regression/PASS. Added readonly third_party symlink,
  full retry90043 LIVE17:23:53 same tree, sparse-full-retry.log, normal restore
  and sparse BIOS/no-entropy/UEFI scopes queued. Poll SAME, freeze this tree.

## Required file-mapping semantics (design, NOT implemented)

Start with genuine immutable asset file mappings required by resource load;
do not silently substitute zero-filled anonymous bytes. Pin the original open
description through metadata/read/map publication; preserve shared file offset
under its lock; mapping lifetime must survive close/dup descriptor reuse.
Readonly immutable MAP_SHARED bytes may be materialized because backing cannot
change, but mprotect must reject adding write and madvise must not zero file
contents. MAP_PRIVATE writable copies require separate documented semantics.
Track file identity, original backing extent, offset and mapping protection
across partial unmap, fixed replacement, advice and protect. Validate offsets,
alignment, bounds, partial EOF-page zero tail and beyond-EOF behavior (SIGBUS or
explicit supported-limit rejection, never invented data). Copy/read/protect
failure must roll back owned allocations without corrupting source or offsets.
Do not claim mutable shared-file coherence, COW, executable file mapping, or
full POSIX mmap until implemented and exercised. Add native asset-byte checks,
offset/EOF/rights/close-reuse/partial-unmap/failure/fault/cancel/relaunch/resource
tests, then run actual Cobalt with the pinned resources and continue startup.

## Next

1. Implement and test genuine file-backed mmap above, then rerun pinned engine
   with resources. Never bypass assertions or modify core to accept failure.
2. Required existing VM/thread/resource/TLS regression and full architectural
   regression before source promotion. Poll SAME90043; no full PASS yet.
3. Ensure kernel process test variant fits linker bounds; current build only
   PROCESS_TESTS0. Diagnostic28 call-abort thunks NEVER promote/count as APIs.
4. Integrate typed constants/device metadata/pread unsupported errors review,
   add affected tests to production suite, normal image restoration and local
   checkpoint only after genuine required results; owner always pushes.

Mirrors: vm-area.h, process-sparse.c/.h, syscall-sparse.c, runtime-urandom.c,
vm-area-unit.c, vm-commit-unit.c, elf-sparse-vm.c, test_sparse_vm.py.
Repro build_sparse_probe.sh, run_sparse_native.sh, run_vm_commit_unit.sh,
run_sparse_cobalt.sh, run_sparse_abort_trace.sh, run_resource_trace.sh,
fetch_cobalt_resources.py, test_startup_diagnostic.py, retry_sparse_regression.sh.
runtime-abort-trace.c/runtime-resource-trace.c diagnostic only; never promote.
All operations logged via tools/run_logged.py.
