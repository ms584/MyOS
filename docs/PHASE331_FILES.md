# Phase 33.1 file descriptor continuation

Scope: the pinned Cobalt x64 file/directory ABI on the existing descriptor
registry. Phase 33.1/33.2 remain open until native Cobalt passes the combined
HTML/JavaScript/DOM/input/HTTPS endurance gate.

## Implemented and exercised

- Regular files are refcounted open objects. Duplicates share the kernel file
  offset and append/nonblocking status; close-on-exec is descriptor-local.
  Closing a duplicate does not release the kernel handle before the final ref.
- openat with AT_FDCWD, read/write, seek, positional and vectored I/O,
  fstat/fstatat, truncate, access, getcwd, mkdir, isatty and pathconf.
  Atomic exclusive create rejects an existing file before truncating it.
  Append seeks and writes in one interrupt-disabled syscall.
- Large reads/writes use new calls 32/33 while legacy calls 6/7 retain their
  256-byte limit. Kernel write validation covers every source page before
  mutation. Read validates the whole destination before advancing the offset.
- Immutable asset files are available at /assets/NAME, backed directly by the
  asset disk. The fixture verifies 128 KiB metadata, nonaligned cross-sector
  reads, reads across 64 KiB, EOF and read-only rejection without copying the
  asset into RAMFS.
- Directory descriptors, opendir/fdopendir/readdir/readdir_r/closedir use the
  pinned musl_dirent layout. fdopendir transfers ownership; closedir releases
  the descriptor. Directory entry names retain the existing 31-byte bound.
- mkstemp/mkdtemp use bounded unique-name retries and exclusive creation.
  realpath checks the original path before canonicalizing, so file/.. fails
  the directory check. unlinkat supports files and empty directories.
- fsync/fdatasync use syscall 34 to commit the real dual-snapshot data disk.
  A volatile RAMFS without a recognized data disk reports ENOTSUP.
  Immutable asset sync succeeds without disk mutation.

## Evidence and limits

tests/test_elf_dynamic.py executes user/elf-platform-file.c in Ring 3.
user/elf-platform-file-sync.c writes 8 KiB of binary data on a disposable v2
disk, commits it, and verifies it in a second QEMU boot. The read-only fixture
asset disk is SHA256-checked before/after all guest operations.
The pinned binary and wrapper hashes pass tools/check_cobalt_contract.py.

These are tested supported subsets, not complete POSIX implementations:
8 kernel file handles, 32 runtime descriptor slots, 64 RAMFS nodes, 64 KiB
mutable files, 127-byte paths and 31-byte RAMFS names remain limits.
Only AT_FDCWD is supported for *at calls; arbitrary relative directory-fd
resolution remains open. Metadata has fixed modes/owner, zero timestamps and
placeholder inode identities. No chmod/chown/statvfs success is advertised.
Directory stream contention, atomic vectored I/O on pipes, links, rename,
inotify, metadata mutation and file-backed mmap remain pending.
Open-file unlink still reports EBUSY under the original RAMFS contract.
64-bit offsets are bounded by the backing store; sparse unbounded seek is
not implemented. Standard descriptors and non-file metadata have limited
compatibility. These limits must be resolved where the real engine uses them.

135 strong imports have fixture coverage, with 107 strong imports still
unimplemented plus a separately zero-bound weak TLS initializer.
Full tests/test_system.py passed on 2026-09-30 at 19:02 +07 and restored the
normal framebuffer image. Updated pinned Cobalt staging passed at 256 MiB
(135 resolved, 107 missing, complete staged-page reclamation) and at 64 MiB
(honest OOM rollback). Missing imports still prevent actual engine execution.
tests/test_run_logged.py passes bounded rotation, feedback rejection, long
lines and child exit propagation; activity.log was 3.8 MB after this regression.
The runner retains at most three 16 MiB logs. Watch using unwrapped tail -F.
