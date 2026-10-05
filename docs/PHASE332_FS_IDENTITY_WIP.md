# Phase 33.1/33.2 — filesystem inode identity prerequisite

Updated 2026-10-05 16:09 +07. PRIVATE ONLY; no POSIX import count increase.
Production server full1701 still LIVE; main source/tests/images stay frozen.
Read PHASE11/12/13 before changes. No33.3/push/phase closure.

## Why this prerequisite is required

Current user directory objects cache a name, not an inode. Adding rename while
leaving these unchanged would invalidate open directory descriptors and race
relative operations against moving directory names. Metadata and inotify also
need identity/lifetime rather than only names. No new exported rename yet.

## Private implementation

- RAMFS FS_DIRECTORY32 accepts readonly directory opens, increments the same
  node references as files. Last close reclaims replaced-unlinked directories.
  Directory read/seek errors and snapshot-import reference exclusion explicit.
- resolve_from/start and directory_base validate a pinned directory once inside
  IRQ exclusion; absolute names ignore dirfd, relative names use its inode.
  fs_open_at/stat_at/list_at/rename_at share this resolver. -100 is root/current
  baseline, not a claim of new per-process CWD or generic POSIX namespace.
- Atomic rename validates both paths, parents, types, destination emptiness,
  root protection and cycles BEFORE mutation. No allocation/failing operation
  after validation; replaced open files remain readable until last close.
  Moving a directory preserves open inode bases. Readonly asset names reject.
- Additive custom SYS_FS_OPERATION56, request56 bytes, four operations:
  OPEN_AT/STAT_AT/RENAME_AT/LIST_AT. Whole request/path/output validation before
  mutation/write. Unknown flags/op/reserved fields and size reject; user buffers
  copied through checked mappings, no unchecked kernel user-pointer access.
- Existing fs_entry48 and snapshots remain unchanged, v1/v2 import/export
  compatibility retained. This is NOT yet chmod/uid/timestamps/symlink metadata.

## Evidence

- ASAN/UBSAN source RAMFS terminal0 15:58:27 in
  /tmp/myos-fs-identity-35ISO4. Real algorithms; only privileged IRQ/console,
  allocator and immutable-asset boundaries stubbed. Open source/destination,
  directory aliases, relative lookup after move, replacement preserving old
  inode, empty directory replacement, cycle/type/root/readonly errors, allocation
  denial, snapshot export/import pin guard+OOM rollback, full-node rename.
- Native private copied mutable tree /tmp/myos-fs-native-u9bSyl. Kernel build
 17740 terminal0 16:02:20, PROCESS_TESTS1,242920 bytes475sectors, linker passed.
- Native82301 terminal0 16:06:31 BIOS+UEFI: four successful live launches/profile
  plus deliberate held-directory/file fault and Escape; exact pages and heap
 90880before/after on observed BIOS runs. Outputs fs-identity-uiauugy8 and
 fs-identity-tkdsh4_y. Tests actual custom56 ABI, relative inode lookup, dir alias
 after move, replacement old fd readability, fd reuse ENOTDIR, table exhaustion
 before create, invalid second rename path leaves source, RX output rejection,
 cross-page RW/readonly output rejected without partial write, malformed ABI.
- Existing filesystem private61299 terminal0 PASS16:11:08; normal private BIOS
  restored. Explicit storage52587 terminal0 PASS16:12:41: ATA reboot/checksum,
  invalid-tree fallback and unknown-image protection. Original test_storage.py
  lacks a CLI main; only explicit exercise is counted, not merely running it.

## Required next steps before POSIX exposure/promotion

1. Private prerequisite regression complete. Main1701 terminal0 PASS16:31:49.
   No filesystem source promoted; remaining adapter/namespace work below.
2. Native user directory object must hold the kernel inode descriptor, share it
   across dup/close, and release exactly once. Move openat/fstatat/unlinkat and
   fdopendir/readdir to atomic inode-relative calls (not path reconstruction).
3. Preserve immutable asset directory behavior and virtual /assets traversal,
   including root-relative access/dot-dot; current kernel DIR open does NOT
   claim an immutable asset-directory descriptor adapter (returns95).
4. Complete metadata, real mode/access/owner semantics, symlink resolution and
   bounds, snapshot version/compatibility, inotify identity/lifetime/readiness.
   Then export and test actual remaining file imports; do not count this as11.
5. A later promoted kernel change requires its own full regression, normal
   BIOS/UEFI restoration, actual paired count and local commit/no push.

Windows source mirrors: ramfs-identity.c, fs-identity.h, fs-operation-call.h,
syscall-fs-identity.c/.h, fs-unit-platform.h, fs-identity-unit.c,
elf-fs-identity.c, test_fs_identity.py. Repro run_fs_identity_unit.sh,
build_fs_identity_probe.sh, run_fs_identity_native.sh,
run_fs_identity_regression.sh. All operations through tools/run_logged.py.
Private mutable copies, independent build/QMP paths; readonly other dependencies.
