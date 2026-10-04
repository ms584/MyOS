# MyOS project state

Updated: 2026-10-04. Repo: /home/xytron404/myos (WSL Ubuntu), branch main.
Baseline0f3f768; native Cstring/C++98 follow-up to Cstdlib/gthread groundwork.
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
- Renderer real managed-RAM queries/empty-GNU fix: PHASE332_OS_INFO.md.
- Sealed GCC15.2.0 gthread adapter: PHASE332_CXX_PORT_PLAN.md; library build fails.
- Sealed division/UTF-8/upstream cstdlib: PHASE332_CSTDLIB_SDK.md; no shell/locale.
- Native bytewise collation/C++98 ABI headers: PHASE332_CSTRING_SDK.md.
- Allocation/atexit/DSO finalization SDK: PHASE332_CXX_LIFETIME.md; no exceptions.
- Renderer configure PASS/compile FAIL: PHASE332_RENDERER.md; no host libs/OpenMP.
- Cross-thread signal/pause/SIGINFO/alt-stack/restart still absent. Logs capped48MiB;
  never inspect activity.log via run_logged.py (use unwrapped tail -F).

## Current evidence (+07)
- New93613 exit0 10:55:33: scoped memory/hash checks, full suite,
  normal BIOS/UEFI restore,pinned contract/staging/OOM/hashes PASS; no live owner.
- Cobalt maps/relocates but79 missing strong imports prevent execution.
- Managed RAM/empty-GNU scoped30 launches/10 malformed rejects BIOS/UEFI PASS;
  exact PMM/data/8workers/relaunch/resources: PHASE332_OS_INFO.md.
- Whole renderer87879 compile1 10:19:42, target-config-yi0713xx:
  11 failed units/11 errors, prior13/16; os_misc/u_dl compile errors clear.
  No native backend or Cobalt success; compile counts are NOT completion%.
- Opt-in upstream C++ header profile; target stdint fixed, boundary PASS10:56.
  Integer154 macros/types PASS C/C++ hosted+freestanding; no native library proof.
  New30635 configure0 11:52:17 jf2fl8q3: real gthreads yes, native TLS disabled.
  Native22111 BIOS/UEFI exit0 11:59:17:8 launches/2cancels/exact resources.
  Library12608 build2; cstdlib/locale/stdio/time gaps, no container/renderer proof.
- New34118 exit0 12:14:54: Cstdlib C/C++ BIOS/UEFI12 launches, guard edges,
  8workers/exact resources; affected scan/MPFR/OOM BIOS/UEFI PASS. All terminal.
  C++98 gthread99435 exit0 12:27:36 BIOS/UEFI,8 launches/2cancels/resources.
- Cstring59789 exit0 12:30:55: C11/C++98/17 BIOS/UEFI12 launches, guards,
  8workers/exact resources. Integer/compiler boundary regression PASS12:32:29.
  Whole C++ build2: stdio/ctime/locale/math/omitted support sources remain;
  cstring and C++98 ABI syntax groups cleared. Normal hashes unchanged12:32:44.
- Signals/masks delivery PASS BIOS/UEFI: PHASE331_SIGNALS.md; stronger66362
  exit0 09:57:35,72 launches/12cancels/RAX/full-RFLAGS/exact resources.
- Test kernel261304/cap262144,840bytes headroom. Normal253112/runtime29136;
  current BIOS/UEFI hashes PHASE332_OS_INFO.md, unchanged by this SDK-only work.

## Required next work
1. Native target C++ gthread/header/services/library build and container tests.
2. Native cross-thread signals/pause/SIGINFO; real EGL/GLES.
3. Run Cobalt itself: HTML, JavaScript/DOM/timers, keyboard/mouse/scroll and
   verified HTTPS with concurrent assets, interactive >=60s, close/relaunch.

Details: HANDOFF.md; test evidence: build/system-tests/. Never push.
