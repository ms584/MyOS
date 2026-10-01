# MyOS project state

Updated: 2026-10-01. Repo: /home/xytron404/myos (WSL Ubuntu), branch main.
Sealed memory/callback formatter checkpoint follows bd4da4b. No push.
Origin/main is 26c97aa after fetch at 12:11 +07. Owner pushes manually.

## Scope and status
- Phase 33.1/33.2 remain OPEN: Cobalt has not executed in MyOS.
- Native fixture coverage: 160/242 strong imports; 82 plus weak TLS remain.
  Supported subsets/counts do not prove complete ABI or actual Cobalt operation.
- Phase 32/33 NOT PASSED; actual YouTube playback is unproven.
- Do not start Phase 33.3 before native Cobalt HTML/JS/DOM/input/HTTPS acceptance.

## Current implementation
- Clocks/pthreads/keys/emutls and shared descriptors, eventfd/pipe/poll/epoll/
  FIONREAD: PHASE331_WAIT.md, PHASE331_PTHREAD.md, PHASE331_SYSTEM.md.
- Refcounted files/dirs, large/positional/vectored reads and real v2-disk fsync:
  PHASE331_FILES.md. Durable Starboard records: PHASE331_STORAGE.md.
- Concurrent TCP/dup/readiness/deadlines: PHASE331_TCP.md,
  PHASE331_SOCKET_OPTIONS.md; AF_UNIX duplex: PHASE331_SOCKETPAIR.md.
- Long-running elf live lifecycle, cancellation/fault/OOM/relaunch reclamation:
  PHASE331_LIFECYCLE.md. Init/fini teardown: PHASE332_FINALIZERS.md.
- Window/input: PHASE332_NATIVE_UI.md (NOT EGL/GLES); events: PHASE332_EVENTS.md.
- Guarded SABI launcher: PHASE332_LAUNCHER.md.
- Native PT_TLS/GD/LD/IE/TLSDESC, bounded slots and detached cleanup:
  PHASE331_NATIVE_TLS.md; BIOS/UEFI tested.
- C++ guard/C11 SDK: PHASE332_CXX_GUARD.md, PHASE332_C11_SDK.md.
- Timed mutex/exit/join/detach: PHASE331_THREAD_SERVICES.md.
- Real Mesa thread utils/barrier/semaphore/dispatch TLS/masks/names port:
  PHASE332_THREAD_UTIL.md; native BIOS/UEFI PASS, not a graphics backend.
- Math-only headers/static providers now in renderer cross-probe:
  PHASE332_MATH_SDK.md; final renderer is NOT linked or executed.
- Native Mesa clock/32-bit wait and UTC calendar: PHASE332_TIME_SDK.md.
- Target integer/descriptor headers + native rounding: PHASE332_INTEGER_SDK.md.
- Guest environment/identity and limits: PHASE332_ENVIRONMENT.md.
- Native memory/callback formatting and limits: PHASE332_FORMAT_SDK.md.
- Allocation/atexit/DSO finalization SDK: PHASE332_CXX_LIFETIME.md; no exceptions.
- Target Mesa configure PASS; real compile still fails: PHASE332_RENDERER.md.
  No Linux libs/OpenMP; native memset/memchr and real Mesa blob utility tested.
- Pinned 128-byte signal masks; no signal delivery. Logs capped at 48 MiB;
  never inspect activity.log via run_logged.py (use unwrapped tail -F).

## Current evidence (2026-10-01, +07)
- Fresh uninterrupted tests/test_system.py PASS; normal image restored.
  Full + SDK guard + UEFI thread/TLS 52428 exited 0 at 10:23:34.
- C11/thread-utils/math/C++ SDK BIOS+UEFI PASS: detailed evidence in their docs.
- Math 1895-vector/FP-worker BIOS 44692 PASS; blob/OOM BIOS 44517 PASS.
- Mesa time 12363 PASS 13:20:56: deadlines/calendar/8-worker cancel/relaunch.
- Integer headers/rounding/files: PHASE332_INTEGER_SDK.md evidence.
- Environment 14514 PASS 13:44:11: 8 workers/bounds/empty relaunch/resources.
- Formatter BIOS PASS 14:10:31; integer/thread utility regressions PASS.
- C/C++ format/env/math boundary PASS 14:09:20 (not native graphics).
- Target 46154 at 14:07:35: configure 0, compile 1 (not graphics PASS).
- SDK-only change: no new UEFI/full run; kernel/runtime baseline unchanged.
- Actual pinned Cobalt refusal/OOM checks 2578 exited 0 at 10:26:26:
  160 resolved, 82 strong imports missing; NOT executed or web acceptance.
- Normal: kernel 252952, image 1474560, runtime 29136, BSS 0x7d2a0.
  Image SHA256 0849bb23f54b1f9c0bdf10b7c89f881263c3209827d8d3eaa9466f10da61fa6a.
- Process-test kernel 261144; cap unchanged; all jobs terminal/images unchanged.
- Host Mesa has four Linux dependencies; no native renderer selected.

## Required next work
1. Close file/std/memory-stream lifecycle and measured libc/stat failures
   (PHASE332_FORMAT_SDK.md), then target C++ and native EGL/GLES.
   Audit absolute-wait edges; no host libc/GL substitute or busy-loop fallback.
2. Run Cobalt itself: HTML, JavaScript/DOM/timers, keyboard/mouse/scroll and
   verified HTTPS with concurrent assets, interactive >=60s, close/relaunch.

Details: HANDOFF.md; test evidence: build/system-tests/. Never push.
