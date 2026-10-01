# MyOS project state

Updated: 2026-10-01. Repo: /home/xytron404/myos (WSL Ubuntu), branch main.
Target stat/real Mesa file checkpoint follows 32b1fc2. No push.
Origin/main is 26c97aa after fetch at 15:08 +07. Owner pushes manually.

## Scope and status
- Phase 33.1/33.2 remain OPEN: Cobalt has not executed in MyOS.
- Native fixture coverage: 160/242 strong imports; 82 plus weak TLS remain.
  Supported subsets/counts do not prove complete ABI or actual Cobalt operation.
- Phase 32/33 NOT PASSED; actual YouTube playback is unproven.
- Do not start Phase 33.3 before native Cobalt HTML/JS/DOM/input/HTTPS acceptance.

## Current implementation
- Clocks/pthreads/keys/emutls and shared descriptors, eventfd/pipe/poll/epoll/
  FIONREAD: PHASE331_WAIT.md, PHASE331_PTHREAD.md, PHASE331_SYSTEM.md.
- Files/dirs/read/fsync: PHASE331_FILES.md; records: PHASE331_STORAGE.md.
- Concurrent TCP/dup/readiness/deadlines: PHASE331_TCP.md,
  PHASE331_SOCKET_OPTIONS.md; AF_UNIX duplex: PHASE331_SOCKETPAIR.md.
- Live cancellation/fault/OOM: PHASE331_LIFECYCLE.md; PHASE332_FINALIZERS.md.
- Window/input: PHASE332_NATIVE_UI.md (NOT EGL/GLES); events: PHASE332_EVENTS.md.
- Guarded SABI launcher: PHASE332_LAUNCHER.md.
- Native TLS/bounded slots/detached cleanup: PHASE331_NATIVE_TLS.md (BIOS/UEFI).
- C++ guard/C11 SDK: PHASE332_CXX_GUARD.md, PHASE332_C11_SDK.md.
- Timed mutex/exit/join/detach: PHASE331_THREAD_SERVICES.md.
- Mesa thread utils: PHASE332_THREAD_UTIL.md (BIOS/UEFI, not graphics).
- Math/header cross-probe: PHASE332_MATH_SDK.md; renderer NOT linked/executed.
- Native Mesa clock/32-bit wait and UTC calendar: PHASE332_TIME_SDK.md.
- Target integer/descriptor headers + native rounding: PHASE332_INTEGER_SDK.md.
- Guest environment/identity and limits: PHASE332_ENVIRONMENT.md.
- Native memory/callback formatting and limits: PHASE332_FORMAT_SDK.md.
- Native unbuffered file/console stream lifecycle: PHASE332_STREAM_SDK.md.
- Native memory streams/real Mesa wrapper: PHASE332_MEMSTREAM_SDK.md.
- Native strings/allocator declarations: PHASE332_STRING_SDK.md.
- Target stat/open/unlink + real Mesa utility: PHASE332_STAT_SDK.md.
- Allocation/atexit/DSO finalization SDK: PHASE332_CXX_LIFETIME.md; no exceptions.
- Target Mesa configure PASS; real compile still fails: PHASE332_RENDERER.md.
  No Linux libs/OpenMP; native memset/memchr and real Mesa blob utility tested.
- Pinned 128-byte signal masks; no signal delivery. Logs capped at 48 MiB;
  never inspect activity.log via run_logged.py (use unwrapped tail -F).

## Current evidence (2026-10-01, +07)
- Fresh uninterrupted tests/test_system.py PASS; normal image restored.
  Full + SDK guard + UEFI thread/TLS 52428 exited 0 at 10:23:34.
- C11/thread/math/C++ BIOS+UEFI and time/integer/environment: evidence in docs.
- Strings C/C++ 58821 PASS 15:04:02; integer/blob regression 2027 PASS 15:02:28.
- Stat/Mesa/failure rollback + C/C++ headers/file streams 16406 PASS 15:23:28.
- Formatter/file-stream regression 97536 PASS 14:52:39; integer/thread PASS.
- C/C++ format/env/math boundary PASS 14:09:20 (not native graphics).
- Streams/cancel 32852 PASS 14:53:56: live 64 KiB memory buffer reclaimed.
- Memory streams 72224 PASS 14:51:37: real Mesa/OOM/seek/8 workers/relaunch.
- Target 55958 at 15:21:15: configure 0, compile 1, target-config-j9niz748.
- SDK-only change: no new UEFI/full run; kernel/runtime baseline unchanged.
- Actual pinned Cobalt refusal/OOM checks 2578 exited 0 at 10:26:26:
  160 resolved, 82 strong imports missing; NOT executed or web acceptance.
- Normal: kernel 252952, image 1474560, runtime 29136, BSS 0x7d2a0.
  Image SHA256 0849bb23f54b1f9c0bdf10b7c89f881263c3209827d8d3eaa9466f10da61fa6a.
- Process-test kernel 261144; cap unchanged; all jobs terminal/images unchanged.
- Host Mesa has four Linux dependencies; no native renderer selected.

## Required next work
1. Close temporary lifetime/libc/OS-info failures (PHASE332_STAT_SDK.md),
   then target C++ and native EGL/GLES.
   Audit absolute-wait edges; no host libc/GL substitute or busy-loop fallback.
2. Run Cobalt itself: HTML, JavaScript/DOM/timers, keyboard/mouse/scroll and
   verified HTTPS with concurrent assets, interactive >=60s, close/relaunch.

Details: HANDOFF.md; test evidence: build/system-tests/. Never push.
