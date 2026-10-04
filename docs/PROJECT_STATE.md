# MyOS project state

Updated: 2026-10-05. Repo: /home/xytron404/myos (WSL Ubuntu), branch main.
Baseline a2cf871 + tested kernel/TLS LTO budget checkpoint.
Origin/main db934b7 (fetchOct4 21:46:44 +07). Owner pushes; never push.

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
  Kernel budget/LTO and measured next-loader layout: PHASE332_KERNEL_BUDGET.md.
- Cross-thread signal/pause/SIGINFO/alt-stack/restart still absent. Logs capped48MiB;
  never inspect activity.log via run_logged.py (use unwrapped tail -F).

## Current evidence (+07)
- Full32406 exit0 Oct4 22:07:28 PASS; normal BIOS restored. Initial43137
  failed on LTO test-symbol names; unique-only aliases/adversarial units fixed it.
- Pinned44797 PASS Oct5 02:21:32: core hash/staging/79-missing/RELRO,
  64MiB OOM/rollback/shell recovery; NOT Cobalt execution.
- Provider9564 PASS02:23:05 BIOS64/UEFI128 stage + UEFI64/BIOS16 OOM,
  repeated/exact page return. Native81712 PASS02:26:20 BIOS2czt0ygc/UEFIs5wv1hcj,
  two launches/exact resources, not Cobalt; hashes unchanged02:26:29.
- Current test249792/cap262144 (12352 free); pre-LTO test262072 (72 free).
  Normal245692/runtime29904; BIOS/UEFI restored. Hashes in kernel-budget doc.
  All jobs terminal; no live build/image owner.

## Required next work
1. Actual module/provider coexistence/export resolution remains; imports unchanged.
   Actual queried context version fixed; audit image numeric/backend limits,
   terminate/reinit, bind-allocation rollback and zero-sized pbuffer dimensions.
   Input is still after rendering teardown, not concurrent engine interaction.
   Tested12352-byte headroom is available; preserve caps and low-memory guards.
   Production worker limit8 remains; actual Cobalt thread requirements unproven.
2. Fix any regression before new features; native cross-thread signals/pause/SIGINFO.
3. Run Cobalt itself: HTML, JavaScript/DOM/timers, keyboard/mouse/scroll and
   verified HTTPS with concurrent assets, interactive >=60s, close/relaunch.

Details: HANDOFF.md; test evidence: build/system-tests/. Never push.
