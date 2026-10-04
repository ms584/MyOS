# MyOS project state

Updated: 2026-10-04. Repo: /home/xytron404/myos (WSL Ubuntu), branch main.
Baseline a79d97c; native UTC/calendar/renderer dependency follow-up.
Origin/main is 3699a87 (fetch12:59 +07). Owner pushes manually; never push.

## Scope and status
- Phase 33.1/33.2 remain OPEN: Cobalt has not executed in MyOS.
- Native fixture coverage: 163/242 strong imports; 79 plus weak TLS remain.
  Measured import coverage: 67.4%; actual Cobalt acceptance: 0/5 (0% verified).
  Separate measures, NOT overall phase completion or complete ABI proof.
- Phase 32/33 NOT PASSED; actual YouTube playback is unproven.
- Do not start Phase 33.3 before native Cobalt HTML/JS/DOM/input/HTTPS acceptance.

## Current implementation
- Clock/pthread/descriptors: PHASE331_WAIT.md, PHASE331_PTHREAD.md, PHASE331_SYSTEM.md.
- Files/network/lifecycle/finalizers: HANDOFF.md + PHASE331_* dependency docs.
- Window/input/events: PHASE332_NATIVE_UI.md (NOT EGL/GLES), PHASE332_EVENTS.md.
- Guarded SABI launcher: PHASE332_LAUNCHER.md.
- TLS/thread/C11/Mesa utility evidence: PHASE331_NATIVE_TLS.md,
  PHASE331_THREAD_SERVICES.md, PHASE332_C11_SDK.md, PHASE332_THREAD_UTIL.md.
- Math/string/sort/stdio/scan/jump/exit SDK evidence: HANDOFF.md + SDK docs.
- Native self-signal/pending/kernel-owned context/abort: PHASE331_SIGNALS.md.
- Renderer real managed-RAM queries/empty-GNU fix: PHASE332_OS_INFO.md.
- Sealed GCC15.2.0 gthread adapter: PHASE332_CXX_PORT_PLAN.md; library build fails.
- Sealed division/UTF-8/upstream cstdlib: PHASE332_CSTDLIB_SDK.md; no shell/locale.
- Native bytewise collation/C++98 ABI headers: PHASE332_CSTRING_SDK.md.
- Real vector/hash/prime-rehash/target assertion: PHASE332_CXX_CONTAINERS.md.
- Native UTC/mktime/strftime/C++ ctime: PHASE332_CALENDAR_SDK.md; no CPU clock.
- Allocation/atexit/DSO finalization SDK: PHASE332_CXX_LIFETIME.md; no exceptions.
- Renderer configure PASS/compile FAIL: PHASE332_RENDERER.md; no host libs/OpenMP.
- Cross-thread signal/pause/SIGINFO/alt-stack/restart still absent. Logs capped48MiB;
  never inspect activity.log via run_logged.py (use unwrapped tail -F).

## Current evidence (+07)
- New93613 exit0 10:55:33: scoped memory/hash checks, full suite,
  normal BIOS/UEFI restore,pinned contract/staging/OOM/hashes PASS; no live owner.
- Cobalt maps/relocates but79 missing strong imports prevent execution.
- Whole renderer64295 configure0/compile1 13:24:22, target-config-snob6k4x:
  2 ASTC C++ units/10 errno errors, prior5/13; ctime/cache/mkdir/_IOLBF clear.
  No native backend or Cobalt success; compile counts are NOT completion%.
- GCC configure7633 exit0 12:36:36 _1j4xqjo: gthreads yes, native TLS disabled.
  Whole make exit2 13:27:49 (build-after-calendar.log): stdio/locale/errno
  remain; ctime errors cleared. No full C++ library.
- Cstdlib/Cstring/C++98 gthread BIOS/UEFI, integer/compiler boundary PASS:
  linked SDK docs/HANDOFF; no full library claim. Math/support-source gaps cleared.
- Native container/fatal paths/resources: PHASE332_CXX_CONTAINERS.md,62162 PASS.
- Calendar52566 exit0 13:15:24:264 oracle vectors,C11/C++98/17 BIOS/UEFI,
  bounds/guards/8workers/12 launches/resources. Real UTC, CPU clock unavailable.
- Regression23097 exit0 13:21:06: Mesa clock/wait +stream BIOS/UEFI PASS.
  All jobs terminal/no image owner; normal four hashes unchanged13:25:07.
- Test kernel261304/cap262144,840bytes headroom. Normal253112/runtime29136;
  current BIOS/UEFI hashes PHASE332_OS_INFO.md, unchanged by this SDK-only work.

## Required next work
1. Renderer: canonical errno constants, then actual GCC mutex/once/link closure.
   Full C++ still needs stdio/locale/errno; do not treat container subset as full.
2. Native cross-thread signals/pause/SIGINFO; real EGL/GLES.
3. Run Cobalt itself: HTML, JavaScript/DOM/timers, keyboard/mouse/scroll and
   verified HTTPS with concurrent assets, interactive >=60s, close/relaunch.

Details: HANDOFF.md; test evidence: build/system-tests/. Never push.
