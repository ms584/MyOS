# Phase 33.1/33.2 — sparse virtual reservation / entropy prerequisite

Updated2026-10-05 17:01 +07. PRIVATE ONLY. Main source1e1b749 unchanged,
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
  exact pages/heap90624 reclaimed. Cause of new FATAL NOT yet identified.
  This is not any HTML/JS/input/HTTPS/endurance acceptance; web0/5 remains.

## Next

1. Trace genuine FATAL caller without bypassing assertions or changing core.
2. Required existing VM/thread/resource/TLS regression and full architectural
   regression before source promotion. No full result for this private source.
3. Ensure kernel process test variant fits linker bounds; current build only
   PROCESS_TESTS0. Diagnostic28 call-abort thunks NEVER promote/count as APIs.
4. Integrate typed constants/device metadata/pread unsupported errors review,
   add affected tests to production suite, normal image restoration and local
   checkpoint only after genuine required results; owner always pushes.

Mirrors: vm-area.h, process-sparse.c/.h, syscall-sparse.c, runtime-urandom.c,
vm-area-unit.c, vm-commit-unit.c, elf-sparse-vm.c, test_sparse_vm.py.
Repro build_sparse_probe.sh, run_sparse_native.sh, run_vm_commit_unit.sh,
run_sparse_cobalt.sh. All operations logged via tools/run_logged.py.
