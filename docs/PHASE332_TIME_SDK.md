# Phase 33.2 — native Mesa clock/wait/calendar SDK

2026-10-01; checkpoint follows 62197f4. This is a renderer dependency slice,
not native EGL/GLES or Cobalt acceptance. Phases 33.1/33.2 remain OPEN.

## Target contracts

`tools/mesa_time_target.patch` selects `user/platform-mesa-time.c` inside real
Mesa 25.0.7 os_time.c. Monotonic timestamps and relative/absolute sleeps use
actual MyOS clocks. Infinite/overflowing relative deadlines map to -1 per
Mesa's interface. Zero/negative microsecond sleeps return without waiting.
Invalid internal clock results trap rather than returning fabricated time.

Mesa's wait interface supplies a volatile 32-bit word that writers need not
explicitly wake. Existing MyOS SYS_WAIT reads an aligned 64-bit word and
requires explicit wake: casting is unsafe and can lose notifications.
The adapter therefore performs atomic acquire observations separated by
blocking absolute monotonic timers (requested 1 ms, rounded by kernel ticks).
It never spins or reads adjacent bytes. Detection latency includes timer
rounding/scheduling; this is not an address-wake/futex implementation. Worker
stores in the fixture use release ordering. Callers still own word lifetime.

Real Mesa cnd_monotonic.c and C23 time.c remain the providers for monotonic
conditions and extended timespec_get. Standalone SDK builds retain their own
C23 wrapper; renderer builds suppress it to avoid duplicate ownership.
The timespec helper now carries an exact 1,000,000,000 ns sum into seconds.
Arbitrary invalid/overflowing timespec_add operands are not newly supported.

`tools/build_time_sdk.py` verifies the musl 1.2.6 archive SHA256
`d585fd3b613c66151fc3249e8ed44f77020cb5e6c1e635a616d3f9f82460512a`,
extracts only COPYRIGHT and the pure __secs_to_tm converter, and renames its
private helper/include boundary. No musl OS, syscall or timezone code is used.
gmtime_r/localtime_r use the guest's explicit UTC-only profile (isdst/gmtoff=0,
zone UTC). No TZ database, host timezone propagation, locale or DST override.
NULL inputs fail EINVAL; unrepresentable calendar years fail EOVERFLOW without
modifying output. Success preserves errno. struct tm is 56 bytes in this SDK;
this does not independently prove the pinned Cobalt calendar ABI.

## Native evidence

`python3 tests/test_mesa_time.py`: BIOS session 12363 exited 0 at 13:20:56 +07.
Artifacts: `build/time-evaluation/sdk-2nq2ubjk/`; serial:
`build/system-tests/mesa-time.log`. Real pinned os_time.c, cnd_monotonic.c,
C23 time.c and threads_posix.c compiled with target-only headers and no host
libraries; shared fixtures have no DT_NEEDED. Tests cover:

- RTC versus monotonic epoch; minimum elapsed sleep/finite deadline.
- Finite, zero, past and infinite unnotified waits on an int at offset four
  with adjacent guards; worker store without explicit wake.
- Exact nanosecond carry; monotonic condition expiry/invalid nsec and retained
  mutex ownership. This test does not cover condition signal/broadcast races.
- UTC epoch -1/0, leap 2000, nonleap 1900/2100, 2038, weekday/year-day and
  extreme timestamp output preservation/error handling.
- Three ordinary launches, eight indefinitely blocked workers cancelled via
  Escape, then relaunch with exact page/heap resource recovery.

C11 regression session 90595 exited 0 at 13:21:59. Earlier compile-only
failures exposed upstream sign-compare/unused-helper warnings; these remain
visible but nonfatal for the fixture. An early runtime attempt exited 75 but
its inline-assembly message helper incorrectly set registers before strlen;
that fixture defect was fixed, and the incomplete run is not counted PASS.

Renderer probe session 76188: configure 0, compile 1 at 13:22:00;
`build/renderer-evaluation/target-config-bepnfc8v/`. Remaining measured errors:
sys/types.h, fcntl.h, inttypes.h, stdio.h and missing LONG_MAX in SDK limits.h.
This does not select a backend or execute graphics. No kernel/runtime/image
changes; private frozen BIOS only. No new UEFI/full-suite claim; existing
45e08d7 full regression evidence remains valid for unchanged architecture.

Next: close measured libc/C++ build requirements, prove native context/present,
then Cobalt HTML/JS/DOM/input/verified HTTPS >=60 seconds and close/relaunch.
No Phase 33.3 and no push.
