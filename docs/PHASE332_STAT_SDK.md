# Phase 33.2 target stat / real Mesa file utility

2026-10-01 checkpoint following 32b1fc2. Phase 33.1/33.2 remain OPEN;
native Cobalt/graphics acceptance is unproven. No Phase 33.3 or push.

## Changes

- user/sdk/sys/stat.h matches the existing 144-byte x86-64 pinned-musl stat
  provider: mode 24, size 48, atim/mtim/ctim 72/88/104, reserved 120.
  C/C++ assertions verify layout; sys/types.h adds the required LP64 types.
- user/platform-file-sdk.c implements source open/stat/unlink adapters over
  existing openat/fstatat/unlinkat. No Linux host libc/syscalls or fabricated
  new metadata. open's optional mode is consumed only for O_CREAT; existing
  runtime filesystem mode limitations remain unchanged.
- tools/mesa_file_target.patch fixes real Mesa os_file.c fdopen-failure handle
  leakage and three negative ENOMEM assignments. Target fcntl calls explicitly
  use the existing runtime long operand convention. General source-level int
  varargs compatibility still needs the runtime audit, not hidden by this port.
- Renderer cross-probe applies the patch to freshly extracted sealed source;
  format/provider builder includes actual file adapters. No success-only stubs.

## Native evidence (+07)

tests/test_stat_sdk.py verifies Mesa 25.0.7 SHA256
592272df3cf01e85e7db300c449df5061092574d099da275d19e97ef0510f8a6,
extracts actual os_file.c/h and detect_os.h, checks/applies target patch and
executes actual utility code in MyOS. Frozen normal BIOS, 64 MiB, no DT_NEEDED.
Initial native 59812 exit 0 15:19:51, sdk-gj_2dlq9.

- struct stat output canaries and field values, bytewise fstat/stat/fstatat
  agreement; invalid fd/path/flags leave the caller output unchanged.
- Real mutable file, directory descriptor and read-only asset metadata.
  Real Mesa unique creation/exclusive failure, binary 4096-byte full-file read,
  NUL terminator, caller ownership, dup/cloexec/shared offset/last-close.
- Exhaust actual native allocator to malloc(1), then release one page so
  runtime descriptor allocation can succeed while Mesa buffer allocation fails.
  ENOMEM stays positive, caller size untouched, all eight file handles reopen.
- Expanded fixture uses test-only linker malloc/realloc wrappers, normally
  delegating to the actual runtime, to force SDK fdopen, initial read-buffer
  and final-trim failures. This separately checks the exact error paths, buffer
  ownership and close rollback; production providers have no injected failures.
- Three launches reclaim exact pages/heap. SDK C/C++ integer audit includes
  stat header/layout; file-stream cancellation/OOM regression follows.
  Final expanded batch 16406 exit 0 15:23:28: sdk-girrz58t stat/injection
  fixture, formats-jb_523e6 C/C++ header audit and sdk-88fsmfcq file streams.

## Bounds / remaining work

Existing PHASE331_FILES.md limits are not cured by headers: fixed owner/modes,
zero timestamps, placeholder inode identities, no symlinks/permissions model,
no chmod/chown/statvfs. fstat supports files/directories, not pipe/socket stat.
unlink-while-open returns EBUSY; this must be resolved before claiming Mesa
anonymous temporary-file semantics. No mkstemp success is advertised.
os_same_file_description cannot determine distinct descriptor identity on this
target; no kcmp/sysctl emulation is claimed. Growing-file read/realloc failure
branch is corrected but not separately forced by this fixture.

Only SDK/provider/target-port changes. Kernel/runtime/normal images untouched;
no new full/UEFI regression or Cobalt import increase claim. Next: proper
anonymous temporary lifetime, random/abort/scanning, OS-info/atexit/socket port,
then native EGL/GLES and Cobalt HTML/JS/DOM/input/verified HTTPS endurance.
