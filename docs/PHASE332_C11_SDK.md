# Phase 33.2 target C11 SDK checkpoint

2026-10-01, kernel/runtime baseline 45e08d7. Phase 33.1/33.2 remain OPEN.
This supplies a real pthread-backed C11 dependency for a future target renderer;
it does not select a renderer or prove Cobalt HTML/JS/DOM/input/HTTPS operation.

## Implementation and limits

- vendor/mesa-c11 contains three pristine pinned Mesa 25.0.7 sources, hashes
  and licenses; see ORIGIN.md. Separate tools/mesa_c11_target.patch is applied
  with exact git apply checks to a private build directory, never to the vendor.
- user/sdk contains a minimal target-only C11 prerequisite header subset.
  Pinned Cobalt storage: mutex 80, condition 96, once 64, both attributes 40,
  pointer/key 8; mutex/condition alignment 8. These are not Linux pthread types.
- HAVE_PTHREAD uses real MyOS pthreads. Native realtime timed mutex waits are
  selected; the upstream trylock/busy-loop fallback is not selected.
- Target patch propagates mutex/attribute failures, distinguishes trylock busy
  from errors, rejects a null start routine and maps thrd_sleep failures to -1
  only for EINTR, -2 for other errors. No signal delivery/EINTR test is claimed.
- user/platform-c11-time.c provides nanosleep and standard TIME_UTC timespec_get;
  Mesa's c23_timespec_get maps UTC to realtime and MONOTONIC to monotonic.
  CPU-time and raw clocks return unavailable without changing the output.
- SDK assert traps an owned process fault, not SIGABRT. stdlib declares only
  malloc/free; unistd intentionally declares no unsupported operations.
- -nostdinc plus GCC intrinsic headers and -ffreestanding exclude host libc
  headers. Native fixture links no DT_NEEDED host libraries; undefined symbols
  resolve to the actual MyOS runtime. This is not a complete libc/C11 port.
- These SDK objects are not fixed runtime exports or included in build.sh yet.
  There is no Cobalt import-count increase. Kernel/image sources did not change.

## Native evidence

tests/test_c11_sdk.py creates and verifies private immutable BIOS/UEFI boot
snapshots; it never rebuilds shared images. It compiles the pinned code against
the target SDK, boots MyOS, runs three successful launches, cancels one blocked
process and runs again. All completed lifetimes verify exact page/heap recovery.

Tested: eight workers and failed ninth creation without changing the output
handle; once publication; distinct TSS and repeated destructors; ordinary return
and explicit signed thrd_exit results; sleep success/invalid errors; UTC and
monotonic time; unavailable clocks; recursive/timed mutex and trylock busy;
realtime/default and explicitly monotonic condition deadlines with lock reacquire.
Cancellation stops the main joining thread plus eight condition-waiting workers.

Expanded BIOS PASS 10:46:43 +07; UEFI PASS 10:47:11 +07 on 2026-10-01.
Logs: build/system-tests/c11-sdk.log and c11-sdk-uefi.log.
Normal image hashes unchanged:

- BIOS: 0849bb23f54b1f9c0bdf10b7c89f881263c3209827d8d3eaa9466f10da61fa6a
- UEFI: 5a28e2ae2c8b6b205b3ad19fd87027d08249daa9f6efb78f352906c818fdcf3c

The fresh full system regression for unchanged 45e08d7 remains valid; scoped
SDK native checks cover this change. No redundant full kernel rebuild/rerun.

## Remaining engine work

Wire the target provider into the real renderer build, close libc/math/C++
dependencies, then prove native GLES/EGL context/shader/texture/text/presentation.
Cobalt itself must pass HTML/JS/DOM, input, verified HTTPS/concurrent assets,
interactive >=60 seconds and close/relaunch before either phase closes.
No Phase 33.3 work or push.
