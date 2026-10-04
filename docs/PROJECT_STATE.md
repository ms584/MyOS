# MyOS project state

Updated: 2026-10-04. Repo: /home/xytron404/myos (WSL Ubuntu), branch main.
Implementation checkpoint52267bb follows d506dd5: native self-signals tested.
Origin/main is d147ec8 (fetch 08:31 +07). Owner pushes manually.

## Scope and status
- Phase 33.1/33.2 remain OPEN: Cobalt has not executed in MyOS.
- Native fixture coverage: 163/242 strong imports; 79 plus weak TLS remain.
  Measured import coverage: 67.4%; actual Cobalt acceptance: 0/5 (0% verified).
  These are separate measures, NOT an overall phase completion percentage.
  Supported subsets/counts do not prove complete ABI or actual Cobalt operation.
- Phase 32/33 NOT PASSED; actual YouTube playback is unproven.
- Do not start Phase 33.3 before native Cobalt HTML/JS/DOM/input/HTTPS acceptance.

## Current implementation
- Clock/pthread/descriptors: PHASE331_WAIT.md, PHASE331_PTHREAD.md, PHASE331_SYSTEM.md.
- Files/records: PHASE331_FILES.md, PHASE331_STORAGE.md; orphan: PHASE331_UNLINK.md.
- TCP/AF_UNIX: PHASE331_TCP.md, PHASE331_SOCKET_OPTIONS.md, PHASE331_SOCKETPAIR.md.
- Live cancellation/fault/OOM: PHASE331_LIFECYCLE.md; PHASE332_FINALIZERS.md.
- Window/input: PHASE332_NATIVE_UI.md (NOT EGL/GLES); events: PHASE332_EVENTS.md.
- Guarded SABI launcher: PHASE332_LAUNCHER.md.
- TLS/thread/C11/Mesa utility evidence: PHASE331_NATIVE_TLS.md,
  PHASE331_THREAD_SERVICES.md, PHASE332_C11_SDK.md, PHASE332_THREAD_UTIL.md.
- Math/string/sort/rwlock/stat/scan/jump SDK evidence: HANDOFF.md + SDK docs.
- sprintf/vsprintf/getc/atomic fgets: PHASE332_STDIO_READ_SDK.md.
- SDK exit/_Exit + bootstrap FINI progress: PHASE332_EXIT_SDK.md.
- Native self-signal/pending/kernel-owned context/abort: PHASE331_SIGNALS.md.
- Allocation/atexit/DSO finalization SDK: PHASE332_CXX_LIFETIME.md; no exceptions.
- Renderer configure PASS/compile FAIL: PHASE332_RENDERER.md; no host libs/OpenMP.
- Cross-thread signal/pause/SIGINFO/alt-stack/restart still absent. Logs capped48MiB;
  never inspect activity.log via run_logged.py (use unwrapped tail -F).

## Current evidence (+07)
- Full97678 exit0 09:55:59: uninterrupted test_system.py, normal BIOS/UEFI
  restore and pinned Cobalt staging/OOM rollback PASS. All jobs terminal.
  Cobalt maps/relocates but 79 missing strong imports prevent execution.
- Whole renderer49393 terminal compile1 09:23:29, target-config-ybt5nn3q:
  13 failed units/16 errors, prior21/29; abort declaration errors clear.
  No native backend or Cobalt success; compile counts are NOT completion%.
- 2026-10-04 formatter75731 PASS sdk-u22lwj7g; stream final12440 exit0
  07:38:27: BIOS C11 sdk-zu4r_ko4 + UEFI C++17 sdk-urta56f3 PASS.
  Line bounds/binary/EOF/EAGAIN/8readers/3cancels/4relaunches/exact resources.
- Exit74114 BIOS/UEFI24 launches/profile + C++ lifetime PASS07:50:41;
  main/worker/constructor/recursive/normal-FINI/raw exit +3cancels/resources.
- Masks39400 exit0 09:02:26 BIOS/UEFI:127-bit helpers/pinned layouts/8workers.
- Delivery78959 exit0 09:15:46 BIOS/UEFI C/C++:72 launches/12handler cancels;
  nested/pending/masks,GP/FP/DF with yield,fault repair,actual SIGABRT,
  forged return,thread-exit slot reuse/nesting exhaustion/exact resources.
  Stronger RAX/full-RFLAGS66362 exit0 09:57:35: BIOS delivery-ft2by0y0,
  UEFI delivery-83dghx44,72 launches/12cancels/exact resources again PASS.
- Kernel-only -Oz recovers4096 test-image bytes; no cap/feature/assertion change.
  Current normal/test kernels253080/261272; test cap262144,872bytes headroom.
  Normal kernel253080/runtime29104; BSS end0x7d340 below BIOS stack0x80000.
  Normal BIOS/UEFI hashes and scoped nonmutation: PHASE331_SIGNALS.md.

## Required next work
1. Native cross-thread sends/wait interruption/pause; actual SIGINFO/context
   and required flags. Renderer ybt5nn3q still needs OS-info/C++/EGL.
2. Run Cobalt itself: HTML, JavaScript/DOM/timers, keyboard/mouse/scroll and
   verified HTTPS with concurrent assets, interactive >=60s, close/relaunch.

Details: HANDOFF.md; test evidence: build/system-tests/. Never push.
