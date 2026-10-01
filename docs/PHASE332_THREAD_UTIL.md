# Phase 33.2 native Mesa thread utility port

2026-10-01; SDK baseline 30f06b6, kernel/runtime baseline 45e08d7.
Phase 33.1/33.2 remain OPEN. No renderer/backend or Cobalt acceptance claim.

## Actual target source, not Linux substitutes

Four additional pristine files from the pinned Mesa 25.0.7 archive are retained
under vendor/mesa-c11/src/util with their copyright/license notices. ORIGIN.md
records hashes. tools/mesa_thread_util_target.patch applies with exact git apply
checks to disposable copies only. Its __myos__ path selects real MyOS pthread
names and Mesa's mutex/condition portable barrier. The fallback now returns true
for exactly one serial thread per generation, matching the native-barrier path,
and asserts nonzero participant count. The original fallback returned true to
every participant. Kernel sources and boot images do not change.

The target build undefines Linux compiler platform macros, defines __myos__,
uses -nostdinc/-ffreestanding and GCC intrinsic headers. It neither applies the
host renderer patch nor links host libc/C11. util/macros.h omits its unused
stdio include on MyOS; this does not implement or claim stdio support.

SDK additions: pinned 128-byte sigset_t and mask helpers (range 1..127, cleared
padding), actual runtime name/mask declarations, plus standalone memcpy/strlen
used by the renderer's thread-name truncation. These do not implement signal
delivery. No new fixed-runtime export or Cobalt import coverage is claimed.

Unsupported util capabilities follow the existing upstream fallback conventions:
current CPU = -1 (unknown), affinity = false, CPU-time profiling = 0 (unavailable).
Zero is not a measured CPU time; do not enable CPU-time profiling based on it.
Monotonic elapsed time is NOT fabricated as thread CPU time. Other unported
Mesa utilities still need their own contract checks; no complete renderer closure.

## Native test evidence

tests/test_mesa_thread_util.py compiles the real patched util source plus pinned
C11 and target SDK. It verifies all seven pristine hashes and no DT_NEEDED;
uses private hash-verified BIOS/UEFI snapshots, never rebuilding shared images.
user/elf-mesa-thread-util.c tests eight workers over 24 pairs of reusable barriers:
shared payload visibility, exactly one serial thread per generation and distinct
native TLS dispatch variables retaining their own values. It tests the actual
thread-create mask path (all supported bits except SIGSYS/SIGSEGV and unblockable
SIGKILL/SIGSTOP), cleared padding and restoration of the parent mask. Actual
name truncation yields the exact 15-byte name, using target memcpy/strlen.
The semaphore wakes main; all join results are checked. It faults on a zero-count
barrier, cancels eight workers blocked at a nine-participant barrier, and relaunches.
Each completed lifetime verifies page/heap recovery; no signal delivery is claimed.

Three normal launches plus fault/cancel/relaunch passed:

- BIOS pipeline 83457 exited 0 at 10:59:54 +07.
- UEFI pipeline 54086 exited 0 at 11:00:26 +07.
- Logs: build/system-tests/mesa-util.log and mesa-util-uefi.log.
- Kernel/runtime/image remain the fully tested 45e08d7 baseline. Scoped SDK
  checks cover this change; that unchanged full regression remains valid.

Remaining work: wire the target utilities into a complete native renderer build,
close libc/math/C++ dependencies, prove GLES/EGL/presentation, then native Cobalt
HTML/JS/DOM/input/verified HTTPS >=60s and close/relaunch. No Phase 33.3 or push.
