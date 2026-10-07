# Phase33.1/33.2 — inode/shared rename and record-lock prerequisite

## Current checkpoint — 2026-10-07 17:20 +07

Main b34cf75 remains clean prerequisites; new FS/runtime not promoted.
Native fs-native-VzEALo foundation44313 terminal0 16:44:02 BIOS/UEFI PASS.
Runtime99309 terminal0 16:54:24: BIOS gyf30f09,UEFI kki4aef6 PASS native
inode DIR/rename/replaced-open-inode lifetime/relative traversal/unlinkat/
retained readdir fd-close-reuse/typed fcntl whole-output guards/immutable
assets/four relaunch/exact resources. Raw syscall and runtime_asset BIOS PASS.
Census17:12: main215registered/217paired,25missing; clean FS216/218,24missing
by true rename; no diagnostic abort probes counted. NOT full POSIX coverage.
FS full64642 FAILED missing audited contracts/window.h at native_ui, NOT PASS;
failure log/status preserved. Provisioned immutable pinned dependency. Combined
full76516 started17:37:14, fs-full.status RUNNING; freeze SAME private source
now includes CPU accounting/actual-size argv/path-offset544 fixes from
phase331-cpu-clock. Full test_system.py plus affected scopes and normal
BIOS/UEFI restore pending. No main promotion yet.

Runtime changes now wired: readonly AUTO_DIRECTORY atomic open avoids stat/
open race; real DIR kernel inode basis survives rename/dup, native fstat.
Streams retain the original open description, not reused fd/path. closedir
only closes matching fd generation/object, never unrelated reused descriptor.
Readonly assets/root virtual adapters are explicit bounded exceptions.
fcntl pointer args use va_arg(void *), scalar args va_arg(int); SEEK_CUR
locking serialized with file offset. ANY duplicated fd close releases owner
locks explicitly before object final reclamation. SYS56 validates whole
request, copied paths and output before mutation; R10 is NOT a call3 argument.
GETLK/SETLK real nonblocking; SETLKW ENOTSUP pending proper wait/cancel queue.
Host four-owner oracle is NOT native multi-process conflict/wakeup proof.

Actual Cobalt diagnostics oo45o3f3/ru5x9dz8 reached Ozone window creation
CHECK; h4mabvi1 instead worker clock_gettime EINVAL (scheduling-sensitive).
All controlled HTTP requests0. Diagnostic traps/traces never production APIs.
Separate CPU clocks source/tests in docs/wip/phase331-cpu-clock; native
BIOS/UEFI PASS, not full-tested/promoted. See PROJECT_STATE for live diagnostic.
Remaining FS9 requires real attributes/symlink/inotify lifetime, not no-op
success. Player/decode15 capability/handle/callback contracts remain open.

## Historical host-only checkpoint (superseded status below)

Updated2026-10-07 13:50 +07. PRIVATE WIP, not in main builds. No new import
claimed: main212registered/214strong paired of242,28missing; clean private
215/217,25missing, census13:48:38. The census checks actual GLOBAL UND242,
not the one additional WEAK symbol in audit names. No abort probes counted.
Full93459 failed audio-fixture8-slot assumption; fixed32 shared bound. Rerun
27417 terminal0 14:47:02 full/scopes/normal restore PASS. Work below remains
separate/unpromoted, not included in that regression or import census.
Atomic unlinkat from renamed DIR inode ASAN/UBSAN6WZkbi PASS14:03:58:
file/directory mode checks, invalid/non-directory fd, absolute ignores badfd,
unlinked open inode lifetime, allocation-denied mutation and readonly assets.
New fs-operation syscall include staged, not wired/native proven yet.

## Actual reason for this work

Genuine Cobalt startup-p2388a2x reaches Code Cache then calls missing rename;
LevelDB repeatedly calls fcntl command6 (F_SETLK) and gets-EINVAL. Zero HTTP.
Do not replace those operations with success stubs or count diagnostic traps
as APIs. Prior inode-only WIP cannot overwrite newer filename/statvfs/shared
sources. Main DIR objects currently cache paths; rename exposure while keeping
that representation would retarget relative operations after a directory move.

## Merged RAMFS implementation (host proof only)

- Bounded printable names and112-byte volume query retained; shared files
  retain page backing, existing64KiB normal-file/snapshot formats unchanged.
- resolve_from/directory_base/open_at/stat_at/list_at use pinned real inode.
  Readonly directory handles share node references; directory I/O rejects.
- Atomic rename validates parents/types/cycles/root/readonly assets before
  mutation. No allocation after validation. Replaced open file/directory
  inodes remain alive until final close. Shared-page aliases retain frames.
- Ephemeral /dev/shm is treated as a separate volume for rename: cross-volume
  moves return EXDEV, and moving its ancestor is rejected. Otherwise moving
  page-backed data out of the excluded subtree could make snapshots dereference
  absent heap data or serialize ephemeral state. This is explicit bounded
  namespace policy, not implemented copy/persistence conversion.
- Empty unlinked-directory lookup and immutable asset-directory adapters remain
  limited as in original prerequisite. Do not claim full POSIX filesystem.

## Real nonblocking advisory record locks (host proof only)

Pinned third_party/musl/include/fcntl.h confirms F_RDLCK0/F_WRLCK1/F_UNLCK2,
F_GETLK5/F_SETLK6/F_SETLKW7, and flock fields short/short/off_t/off_t/pid_t.
LP64 structure32bytes/start8/len16/pid24 asserted; padding is not validated.
The cached header is main/build/cobalt-config-audit-20261007/
third_party-musl-include-fcntl.h, source3befd332486952f6603ae3baf2a8713878d6e200.

- record-lock.c has64 bounded records, inode key + kernel file-table owner,
  actual trusted process PID for GETLK reporting. Dynamic pool is released
  when the last lock goes; no permanent allocation/large static BIOS BSS.
- Read/read sharing, write conflicts, own-process replacements, split/coalesce
  and partial unlock operate in scratch; ENOLCK/OOM leave live locks unchanged.
- fs_record_lock normalizes SEEK_SET/CUR/END, negative lengths and overflow;
  len0 extends through future EOF. SET read/write rights checked. No namespace
  name is used as lock identity, so rename/replacement cannot transfer locks.
- fs_close releases that file-table owner's locks for the inode on ANY kernel
  fd close. fs_record_release provides the explicit runtime dup-close hook:
  runtime dup shares one kernel handle, so waiting for object final release
  would violate POSIX close-any-fd semantics. This hook is NOT yet wired.
- F_SETLKW is explicitly ENOTSUP, not a spin/success stub. Blocking queue,
  wakeup/cancellation/deadlock behavior must be implemented before claiming
  full locking support. Immutable assets also reject locks as ENOTSUP.

## Evidence (do not broaden scope)

- ASAN/UBSAN2v52uh terminal0 13:32:21: merged RAMFS/shared/inode rename,
  old handles after move/replacement, shared aliases after inode death,
  allocation-denied rename, EXDEV/cycles/root/readonly and exact resources;
  preserved original shared snapshot-v1/v2/OOM checks. No native syscall proof.
- ASAN/UBSAN record-lock-unit-6Lh1HQ terminal0 13:41:13: independent2000-step
  four-owner byte-range oracle, conflict identity, read sharing, conversion,
  split/coalesce/EOF, atomic ENOLCK/OOM and exact pool teardown. Not fcntl proof.
- ASAN/UBSAN fs-identity-shared-unit-3kues4 terminal0 13:43:11: real RAMFS
  integration, locking across rename/replacement, separate inode identities,
  close-any-kernel-fd release, offset/negative lengths/rights/overflow and exact
  heap/pages/nodes. PMM/IRQ/asset boundaries mocked, not native isolation proof.
- No native build/scope or actual Cobalt run for this batch yet. No rename
  export or import-count change. No Phase33.3/push/phase closure.

## Next exact work

1. Poll SAME full93459; source cd frozen until terminal. Do not restart merely
   from observation timeout. Main8487d03/origin c33f49a fetched13:45 unchanged.
2. Wire custom SYS_FS_OPERATION56 into current volume/file-map syscall source,
   not older identity-only source. Check full request/paths and GETLK outputs
   before mutation. Owner and PID come from process, never caller fields.
3. Convert runtime DIR objects to hold a kernel inode handle with reference
   lifetime through dup/close, openat/fstatat/readdir/unlinkat/rename. Keep
   immutable asset-directory path adapter (safe because readonly), including
   root-relative assets and /.. checks; do not break asset traversal. Native
   root fs_entry name is '/' (not a legal component), useful for root adapter.
   Native DIR fstat uses SYS_FSTAT, never its stale cached path. readdir must
   hold original open description across close/reuse; no descriptor retarget.
4. Parse fcntl pointer arguments as pointer and int arguments as int. Pin file
   open description, serialize SEEK_CUR with offset-changing I/O. Add explicit
   per-fd close lock release before last-object reclamation (including dup),
   then native multi-process conflict/wakeup/close/reuse/cancel tests. Implement
   genuine F_SETLKW rather than pretending F_SETLK suffices for all POSIX locks.
5. Export true rename only after DIR lifetime is corrected. Run scoped native
   BIOS/UEFI + relevant storage and architectural regressions; promote clean
   sources only, restore normal images, update docs/local commit. Continue FS10
   and player/decode15 contracts, then actual five Cobalt acceptance gates.

Recovery inputs: docs/wip/phase332-fs-lifetime. record-lock unit and merged
RAMFS tests run in independent /home/xytron404/*-unit-* trees. Source/tests
remain unpromoted and cannot be compiled by main build.sh accidentally.
