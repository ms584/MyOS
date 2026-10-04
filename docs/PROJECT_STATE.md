# MyOS project state

Updated: 2026-10-04. Repo: /home/xytron404/myos (WSL Ubuntu), branch main.
Baseline a38b85b; native OSMesa execution follow-up (not yet PASS).
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
- Native C/C++ SDK: PHASE332_CSTDLIB_SDK.md, PHASE332_CSTRING_SDK.md,
  PHASE332_CXX_CONTAINERS.md, PHASE332_CALENDAR_SDK.md, PHASE332_CXX_LIFETIME.md.
- Renderer compiler/link closure PASS: PHASE332_RENDERER_CLOSURE.md.
  Actual native context now binds: PHASE332_NATIVE_RENDERER.md; no EGL/GLES backend.
- Cross-thread signal/pause/SIGINFO/alt-stack/restart still absent. Logs capped48MiB;
  never inspect activity.log via run_logged.py (use unwrapped tail -F).

## Current evidence (+07)
- New93613 exit0 10:55:33: scoped memory/hash checks, full suite,
  normal BIOS/UEFI restore,pinned contract/staging/OOM/hashes PASS; no live owner.
- Cobalt maps/relocates but79 missing strong imports prevent execution.
- Renderer snob6k4x incremental link exit0 13:49:17, 16298592 bytes,
  no strong undefined/DT_NEEDED; weak GLAPI initializer + TLS segment remain.
  Configure-only runtime is NOT safe native execution or Cobalt evidence.
- GCC _1j4xqjo: gthreads yes; whole make exit2 13:27:49 (before errno fix).
  Not rerun; no full C++ library. Ctime/errno now separately tested.
- Errno74750 BIOS/UEFI exit0 13:44:44; sealed ABI C11/C++98/17 PASS.
- Math72599 BIOS/UEFI exit0 13:43:03:2357 MPFR cases +rounding/workers PASS.
- Helpers3601 BIOS/UEFI exit0 13:48:04; guard/popcount/fault recovery PASS.
  Prior jobs terminal; normal four hashes unchanged14:54:41.
- Native OSMesa uses installed runtime, not configure-only runtime globals.
  BIOS native-wfnqmaee: context bound, missing0/weak-zero1; 300s test FAILED
  14:49:16 before pixels/shader/present/input/resources PASS. Not a deadlock proof.
- 86476 native-bcahc3qq terminal exit1 15:03:34: clear pixels PASS, vertex
  compile did not finish600s. CPU samples: process_vm_map first-fit search.
  Native-cold-600.log; no live VM owner; normal hashes unchanged15:03:54.
- Test kernel261304/cap262144,840bytes headroom. Normal253112/runtime29136;
  current BIOS/UEFI hashes PHASE332_OS_INFO.md, unchanged by this SDK-only work.

## Required next work
1. Fix evidenced allocator/page-search cost without weakening semantics;
   verify native pixels/input/teardown/resources BIOS+UEFI and relaunch.
   Then implement real EGL/GLES and SABI integration; do not substitute OSMesa smoke.
2. Native cross-thread signals/pause/SIGINFO; real EGL/GLES.
3. Run Cobalt itself: HTML, JavaScript/DOM/timers, keyboard/mouse/scroll and
   verified HTTPS with concurrent assets, interactive >=60s, close/relaunch.

Details: HANDOFF.md; test evidence: build/system-tests/. Never push.
