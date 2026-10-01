# MyOS project state

Updated: 2026-10-01. Repo: /home/xytron404/myos (WSL Ubuntu), branch main.
Native target math checkpoint follows 6e4bd4a. Local only; no push.
Origin/main remains 54ef570 after fetch at 10:11 +07. Owner pushes manually.

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
- NEW native PT_TLS/GD/LD/IE/TLSDESC, distinct FS/templates, bounded slots and
  syscall-time detached cleanup: PHASE331_NATIVE_TLS.md; BIOS/UEFI tested.
- SDK C++ guard provider native-tested; not yet renderer-linked or a fixed
  runtime export: PHASE332_CXX_GUARD.md. No import count increase.
- NEW timed mutex/main+worker pthread_exit, main join/detach and exclusive
  ownership: PHASE331_THREAD_SERVICES.md; constructor-time exit unsupported.
- NEW pinned Mesa C11 provider on MyOS-only ABI headers; native BIOS/UEFI
  clocks/threads/once/TSS/cancel tested: PHASE332_C11_SDK.md. Not renderer-linked.
- Real Mesa thread utils/barrier/semaphore/dispatch TLS/masks/names port:
  PHASE332_THREAD_UTIL.md; native BIOS/UEFI PASS, not a graphics backend.
- Native math prototype/FP state: PHASE332_MATH_SDK.md; not renderer-linked.
- Pinned 128-byte signal masks; no signal delivery. Logs capped at 48 MiB;
  never inspect activity.log via run_logged.py (use unwrapped tail -F).

## Current evidence (2026-10-01, +07)
- Fresh uninterrupted tests/test_system.py PASS; normal image restored.
  Full + SDK guard + UEFI thread/TLS 52428 exited 0 at 10:23:34.
- C11/Mesa util scoped BIOS+UEFI PASS, including cancel/relaunch/resources;
  detailed times and coverage in PHASE332_C11_SDK.md/PHASE332_THREAD_UTIL.md.
- Math 1895 MPFR vectors/FP isolation/relaunch: BIOS PASS 11:25:38,
  UEFI PASS 11:26:11. No host library in native fixture; exact resources.
- Actual pinned Cobalt refusal/OOM checks 2578 exited 0 at 10:26:26:
  160 resolved, 82 strong imports missing; NOT executed or web acceptance.
- Normal: kernel 252952, image 1474560, runtime 29136, BSS 0x7d2a0.
  Image SHA256 0849bb23f54b1f9c0bdf10b7c89f881263c3209827d8d3eaa9466f10da61fa6a.
- Process-test kernel 261144; cap unchanged. Only runtime C uses -Oz.
  All handles terminal; no live tests. Images unchanged, full result reused.
- Host Mesa still has four Linux dependencies; no native renderer selected.
  PHASE332_RENDERER.md and HANDOFF.md retain measured closure/older evidence.

## Required next work
1. Target libc/C11/C++ and native EGL/GLES closure; no busy-loop fallback.
   Audit remaining absolute-wait edge cases and native pthread object semantics.
2. Run Cobalt itself: HTML, JavaScript/DOM/timers, keyboard/mouse/scroll and
   verified HTTPS with concurrent assets, interactive >=60s, close/relaunch.

Details: HANDOFF.md, PHASE331_TCP.md; test evidence: build/system-tests/.
Never close 33.1/33.2 on symbol counts or fixture success. Never push.
