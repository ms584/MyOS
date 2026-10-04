# MyOS project state

Updated: 2026-10-04. Repo: /home/xytron404/myos (WSL Ubuntu), branch main.
Baseline2ee96b4 + separate genuine provider package/native staging.
Origin/main db934b7 (fetch20:17:08 +07). Owner pushes; never push.

## Scope and status
- Phase 33.1/33.2 remain OPEN: Cobalt has not executed in MyOS.
- Native fixture coverage: 163/242 strong imports; 79 plus weak TLS remain.
  Measured import coverage: 67.4%; actual Cobalt acceptance: 0/5 (0% verified).
  Separate measures, NOT overall phase completion or complete ABI proof.
- Phase 32/33 NOT PASSED; actual YouTube playback is unproven.
- Do not start Phase 33.3 before native Cobalt HTML/JS/DOM/input/HTTPS acceptance.

## Current implementation
- Clock/pthread/descriptors: PHASE331_WAIT.md, PHASE331_PTHREAD.md, PHASE331_SYSTEM.md.
- Window/input/events: PHASE332_NATIVE_UI.md (NOT EGL/GLES), PHASE332_EVENTS.md.
- Guarded SABI launcher: PHASE332_LAUNCHER.md.
- TLS/thread/C11/Mesa utility evidence: PHASE331_NATIVE_TLS.md,
  PHASE331_THREAD_SERVICES.md, PHASE332_C11_SDK.md, PHASE332_THREAD_UTIL.md.
- Native self-signal/pending/kernel-owned context/abort: PHASE331_SIGNALS.md.
- Renderer real managed-RAM queries/empty-GNU fix: PHASE332_OS_INFO.md.
- Renderer compiler/link closure PASS: PHASE332_RENDERER_CLOSURE.md.
  Native BIOS+UEFI: PHASE332_NATIVE_RENDERER.md; allocator: PHASE331_ALLOCATOR.md.
  Real EGL core/driver pbuffer/current/deferred-destroy BIOS+UEFI PASS:
  PHASE332_NATIVE_EGL.md. Native EGL window BIOS+UEFI PASS:
  PHASE332_NATIVE_EGL_WINDOW.md. Dependency evidence, not Cobalt.
  Final-core-reference cleanup BIOS/UEFI/pbuffer PASS: PHASE332_EGL_LIFETIME.md.
  KHR fence/8 waiters after context teardown PASS: PHASE332_NATIVE_EGL_SYNC.md.
  Actual image storage/2D write-back/cube/3D/RBO: PHASE332_NATIVE_EGL_IMAGE.md.
  Context/version/debug/surfaceless evidence: PHASE332_NATIVE_EGL_CONTEXT.md.
  Persistent surfaces/linear128/sRGB188: PHASE332_NATIVE_EGL_COLORSPACE.md.
  Core1.5 wide sync/image/debug/no-CL: PHASE332_NATIVE_EGL_CORE15.md.
  Pinned EGL table/query: PHASE332_NATIVE_EGL_INTERFACE.md (not loader exports).
  Pinned GLES table/native dispatch: PHASE332_NATIVE_GLES_INTERFACE.md.
  Real provider package/TLS/staging/OOM: PHASE332_RENDERER_PROVIDER.md.
- Cross-thread signal/pause/SIGINFO/alt-stack/restart still absent. Logs capped48MiB;
  never inspect activity.log via run_logged.py (use unwrapped tail -F).

## Current evidence (+07)
- Full9925 exit0 16:08:50 covers allocator/VM; normal BIOS/UEFI restored.
- Pinned95008 exit0 16:11:52: Cobalt hash/staging/79-missing/RELRO and
  64MiB OOM/rollback/shell recovery PASS. No live test/image owner.
- GLES43457 PASS20:50:27. Provider BIOS64 stage PASS; UEFI128 stage +
  UEFI64/BIOS16 OOM matrix56309 PASS21:17:56, repeated/exact page return.
- Normal253880/runtime29904. Test kernel measured262072/cap262144 (72 bytes
  headroom). Full PASS; no kernel budget relaxation or RWX bypass.

## Required next work
1. Actual module/provider coexistence/export resolution remains; imports unchanged.
   Actual queried context version fixed; audit image numeric/backend limits,
   terminate/reinit, bind-allocation rollback and zero-sized pbuffer dimensions.
   Input is still after rendering teardown, not concurrent engine interaction.
   Solve72-byte test-kernel headroom before adding embedded runtime services.
   Production worker limit8 remains; actual Cobalt thread requirements unproven.
2. Fix any regression before new features; native cross-thread signals/pause/SIGINFO.
3. Run Cobalt itself: HTML, JavaScript/DOM/timers, keyboard/mouse/scroll and
   verified HTTPS with concurrent assets, interactive >=60s, close/relaunch.

Details: HANDOFF.md; test evidence: build/system-tests/. Never push.
