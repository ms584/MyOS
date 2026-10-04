# MyOS project state

Updated: 2026-10-04. Repo: /home/xytron404/myos (WSL Ubuntu), branch main.
Baseline bdab91c + native Mesa EGL pbuffer/context and safety follow-up.
Origin/main452faaa (fetch15:51 +07). Owner pushes; never push.

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
- C/C++/math/string SDK evidence and remaining library gaps: HANDOFF.md + SDK docs.
- Native self-signal/pending/kernel-owned context/abort: PHASE331_SIGNALS.md.
- Renderer real managed-RAM queries/empty-GNU fix: PHASE332_OS_INFO.md.
- Renderer compiler/link closure PASS: PHASE332_RENDERER_CLOSURE.md.
  Native pixels/input/teardown/relaunch BIOS+UEFI PASS; not Cobalt backend.
  PHASE332_NATIVE_RENDERER.md; packed allocator: PHASE331_ALLOCATOR.md.
  GLES profile +desktop regression BIOS/UEFI PASS: PHASE332_NATIVE_GLES.md.
  Glyph alpha/text512 pixels GLES BIOS/UEFI +desktop PASS: PHASE332_NATIVE_TEXT.md.
  Real EGL core/driver pbuffer/current/deferred-destroy BIOS+UEFI PASS:
  PHASE332_NATIVE_EGL.md. EGL1.4 partial backend, not native EGL windows/Cobalt.
- Cross-thread signal/pause/SIGINFO/alt-stack/restart still absent. Logs capped48MiB;
  never inspect activity.log via run_logged.py (use unwrapped tail -F).

## Current evidence (+07)
- Full9925 terminal exit0 16:08:50 covers new allocator/VM; normal BIOS/UEFI
  restored. Old full93613 is historical, not this architecture's evidence.
- Cobalt maps/relocates but79 missing strong imports prevent execution.
- GCC _1j4xqjo: gthreads yes; whole make exit2 13:27:49 (before errno fix).
  Not rerun; no full C++ library. Ctime/errno now separately tested.
- Native91959 BIOS exit0 15:25:50 j4m9973h; UEFI46868 exit0 15:28:03 cefi3_3l.
  Real shader/texture/1024 pixel oracle/QMP frame/key input/teardown,2 launches
  each, exact resources. NOT EGL/GLES/text/backend selection/Cobalt acceptance.
- Packed allocator65329 exit0 15:31:30: BIOS yoexdyr8/UEFI9ndojdgg,3 launches
  each/8workers/collision/wrap/packing/zero/alignment/OOM/exact resources PASS.
- Full/affected9925 PASS: blob e83rz9tb; lifetime BIOS l8wpahe4/UEFI oxo9w78y,
  OOM/finalizers/cancel/relaunch/exact resources. Hashes PHASE331_ALLOCATOR.md.
- Pinned95008 exit0 16:11:52: Cobalt hash/staging/79-missing/RELRO and
  64MiB OOM/rollback/shell recovery PASS. No live test/image owner.
- Slabs fix small-object OOM; generic translator replaces NX-violating JIT.
- Normal253880/runtime29904. Test kernel measured262072/cap262144 (72 bytes
  headroom). Full PASS; no kernel budget relaxation or RWX bypass.

## Required next work
1. Native EGL window/platform, SABI EGL/GLES tables and loader coexistence.
   Genuine EGL core14/14 +driver16/16 compile; BIOS/UEFI pbuffer PASS.
   Negative texture/pixmap and largest-pbuffer guards PASS both profiles;
   affected pure-GLES PASS82634 exit0 17:58:02. No native-window/module/table yet.
   Solve72-byte test-kernel headroom before adding embedded runtime services.
2. Fix any regression before new features; native cross-thread signals/pause/SIGINFO.
3. Run Cobalt itself: HTML, JavaScript/DOM/timers, keyboard/mouse/scroll and
   verified HTTPS with concurrent assets, interactive >=60s, close/relaunch.

Details: HANDOFF.md; test evidence: build/system-tests/. Never push.
