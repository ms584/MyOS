# MyOS project state

Updated: 2026-10-01. Repo: /home/xytron404/myos (WSL Ubuntu), branch main.
Native renderer target-configuration checkpoint follows f2a2955. No push.
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
- Native fullscreen window/pixels/input: PHASE332_NATIVE_UI.md; NOT EGL/GLES.
- Native callbacks/input/visibility/cancel/preload: PHASE332_EVENTS.md.
- Guarded 2 MiB SABI/SbEventHandle launcher: PHASE332_LAUNCHER.md.
- Native PT_TLS/GD/LD/IE/TLSDESC, bounded slots and detached cleanup:
  PHASE331_NATIVE_TLS.md; BIOS/UEFI tested.
- C++ guard SDK: PHASE332_CXX_GUARD.md; not a fixed runtime export.
- Timed mutex/pthread_exit/join/detach: PHASE331_THREAD_SERVICES.md.
- Pinned Mesa C11 SDK: PHASE332_C11_SDK.md; native BIOS/UEFI tested.
- Real Mesa thread utils/barrier/semaphore/dispatch TLS/masks/names port:
  PHASE332_THREAD_UTIL.md; native BIOS/UEFI PASS, not a graphics backend.
- Native math prototype/FP state: PHASE332_MATH_SDK.md; not renderer-linked.
- Allocation/atexit/DSO finalization SDK: PHASE332_CXX_LIFETIME.md; no exceptions.
- Target Mesa configure PASS; real compile still fails: PHASE332_RENDERER.md.
  No Linux libs/OpenMP; real SDK memset added, native BIOS fixture PASS.
- Pinned 128-byte signal masks; no signal delivery. Logs capped at 48 MiB;
  never inspect activity.log via run_logged.py (use unwrapped tail -F).

## Current evidence (2026-10-01, +07)
- Fresh uninterrupted tests/test_system.py PASS; normal image restored.
  Full + SDK guard + UEFI thread/TLS 52428 exited 0 at 10:23:34.
- C11/thread-utils/math/C++ SDK BIOS+UEFI PASS: detailed evidence in their docs.
- New memset + affected Mesa-util BIOS 13315 PASS 12:08:45, exact resources;
  no UEFI rerun for this SDK-only change. Kernel/runtime baseline unchanged.
- Target configure 97415 finished 12:10:01: configure 0, compile 1 (not PASS).
  Boundary link/no Linux ABI and nine forbidden-link routes PASS 12:12:42.
- Actual pinned Cobalt refusal/OOM checks 2578 exited 0 at 10:26:26:
  160 resolved, 82 strong imports missing; NOT executed or web acceptance.
- Normal: kernel 252952, image 1474560, runtime 29136, BSS 0x7d2a0.
  Image SHA256 0849bb23f54b1f9c0bdf10b7c89f881263c3209827d8d3eaa9466f10da61fa6a.
- Process-test kernel 261144; cap unchanged. Only runtime C uses -Oz.
- All jobs terminal; normal images unchanged, full result reused.
- Host Mesa has four Linux dependencies; no native renderer selected.

## Required next work
1. Wire sealed target math headers/library, close actual libc/header failures
   (sys/types.h, fcntl.h, inttypes.h, localtime_r), then target C++ and EGL/GLES.
   Audit absolute-wait edges; no host libc/GL substitute or busy-loop fallback.
2. Run Cobalt itself: HTML, JavaScript/DOM/timers, keyboard/mouse/scroll and
   verified HTTPS with concurrent assets, interactive >=60s, close/relaunch.

Details: HANDOFF.md; test evidence: build/system-tests/. Never push.
