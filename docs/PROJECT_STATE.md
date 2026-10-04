# MyOS project state

Updated: 2026-10-04. Repo: /home/xytron404/myos (WSL Ubuntu), branch main.
Tested owned-image exit checkpoint follows0643303; full regression PASS. No push.
Origin/main is d147ec8 (fetch 07:29 +07). Owner pushes manually.

## Scope and status
- Phase 33.1/33.2 remain OPEN: Cobalt has not executed in MyOS.
- Native fixture coverage: 160/242 strong imports; 82 plus weak TLS remain.
  Measured import coverage: 66.1%; actual Cobalt acceptance: 0/5 (0% verified).
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
- Allocation/atexit/DSO finalization SDK: PHASE332_CXX_LIFETIME.md; no exceptions.
- Renderer configure PASS/compile FAIL: PHASE332_RENDERER.md; no host libs/OpenMP.
- Pinned 128-byte signal masks; no signal delivery. Logs capped at 48 MiB;
  never inspect activity.log via run_logged.py (use unwrapped tail -F).

## Current evidence (+07; older entries 2026-10-01)
- Full78177 uninterrupted test_system.py PASS; chain exit0 2026-10-04
  08:23:55 includes normal BIOS/UEFI restore + hashes. No assertions removed.
  Earlier aggregate prefix/suffix evidence remains historical in UNLINK docs.
- Unlink/Mesa/snapshot79689 BIOS +96289 UEFI PASS; exact resources,
  v1/v2 live-orphan snapshot/reboot; full provenance PHASE331_UNLINK.md.
- Earlier RW/time/string/stat/stream/sort/thread/math proofs: respective SDK docs.
- Whole renderer1929 compile1 2026-10-04 07:56:52, target-config-uay8w01f:
  configure/boundary PASS;21 failed units/29 errors, prior33/47; NOT completion%.
- Scanner74650 and jump55677 BIOS/UEFI PASS; detailed provenance in SDK docs.
- 2026-10-04 formatter75731 PASS sdk-u22lwj7g; stream final12440 exit0
  07:38:27: BIOS C11 sdk-zu4r_ko4 + UEFI C++17 sdk-urta56f3 PASS.
  Line bounds/binary/EOF/EAGAIN/8readers/3cancels/4relaunches/exact resources.
- Seven affected Mesa units compile0 07:37:21 in existing u8ev3uvw build.
  Old whole report retained; no new whole-renderer success/count is claimed.
- Exit74114 BIOS/UEFI24 launches/profile + C++ lifetime PASS07:50:41;
  main/worker/constructor/recursive/normal-FINI/raw exit +3cancels/resources.
- Full78177 and renderer1929 terminal; no running jobs or shared-image owner.
- Remaining: abort/raise/signal/sigaction, OS-info/file locks/buffered stdio and
  genuine target C++ dependencies. No native backend or Cobalt execution.
- Actual pinned Cobalt refusal/OOM checks 2578 exited 0 at 10:26:26:
  160 resolved, 82 strong imports missing; NOT executed or web acceptance.
- Normal: kernel 252952, image 1474560, runtime 29136, BSS 0x7d2a0.
  BIOS SHA256 abd0f7add9a9c7c9a5fb4f86d967c030a4c4f2cf6344860bed2e96580ec045c8.
- Normal/test kernel252952/261144; cap unchanged. New bootstrap changes hashes.
  Normal BIOS/UEFI restored; complete hashes in PHASE332_EXIT_SDK.md.

## Required next work
1. Use uay8w01f compile.log: actual signal contracts/abort, OS-info/C++/EGL.
   SDK exit is NOT a new pinned-core binding; no fake signal behavior.
   Audit absolute-wait edges; no host libc/GL substitute or busy-loop fallback.
2. Run Cobalt itself: HTML, JavaScript/DOM/timers, keyboard/mouse/scroll and
   verified HTTPS with concurrent assets, interactive >=60s, close/relaunch.

Details: HANDOFF.md; test evidence: build/system-tests/. Never push.
