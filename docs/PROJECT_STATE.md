# MyOS project state

Updated: 2026-10-04. Repo: /home/xytron404/myos (WSL Ubuntu), branch main.
Tested stdio dependency checkpoint follows d147ec8. No push.
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
- Allocation/atexit/DSO finalization SDK: PHASE332_CXX_LIFETIME.md; no exceptions.
- Renderer configure PASS/compile FAIL: PHASE332_RENDERER.md; no host libs/OpenMP.
- Pinned 128-byte signal masks; no signal delivery. Logs capped at 48 MiB;
  never inspect activity.log via run_logged.py (use unwrapped tail -F).

## Current evidence (+07; older entries 2026-10-01)
- Full coverage completed as passed prefix 83161 + resumed suffix 89247
  exit 0 16:33:42, all remaining tests/VGA/finally; normal restored.
  NOT uninterrupted full-run PASS. Earlier runs hit media 30s boot timeout;
  diagnostic proved shell in 30.74s. Media boot waits now common 60s,
  all assertions retained; details/provenance PHASE331_UNLINK.md.
- Unlink/Mesa/snapshot79689 BIOS +96289 UEFI PASS; exact resources,
  v1/v2 live-orphan snapshot/reboot; full provenance PHASE331_UNLINK.md.
- Earlier RW/time/string/stat/stream/sort/thread/math proofs: respective SDK docs.
- Whole renderer87828 compile1 18:55:07, target-config-u8ev3uvw: configure/
  boundary PASS;33 failed units/47 errors, prior53/80; NOT completion percent.
- Scanner74650 and jump55677 BIOS/UEFI PASS; detailed provenance in SDK docs.
- 2026-10-04 formatter75731 PASS sdk-u22lwj7g; stream final12440 exit0
  07:38:27: BIOS C11 sdk-zu4r_ko4 + UEFI C++17 sdk-urta56f3 PASS.
  Line bounds/binary/EOF/EAGAIN/8readers/3cancels/4relaunches/exact resources.
- Seven affected Mesa units compile0 07:37:21 in existing u8ev3uvw build.
  Old whole report retained; no new whole-renderer success/count is claimed.
- Remaining: exit/abort, OS-info/file locks/buffered stdio and
  genuine target C++ dependencies. No native backend or Cobalt execution.
- Actual pinned Cobalt refusal/OOM checks 2578 exited 0 at 10:26:26:
  160 resolved, 82 strong imports missing; NOT executed or web acceptance.
- Normal: kernel 252952, image 1474560, runtime 29136, BSS 0x7d2a0.
  BIOS SHA256 765a3f0c65e9746e80a14226ac2a142651ecbca1e3858cf2c5b2a61212335e5d.
- Process-test kernel 261144; cap unchanged. Normal kernel/runtime/BIOS/UEFI
  hashes unchanged 2026-10-04 07:34:29; all jobs terminal, no image owner.

## Required next work
1. Read PHASE332_STDIO_READ_SDK.md and original u8ev3uvw compile.log. Audit exit/abort
   against PHASE332_FINALIZERS.md: raw SYS_EXIT skips FINI; pthread_exit is
   not process exit. Implement real lifecycle semantics, then OS-info/C++/EGL.
   Audit absolute-wait edges; no host libc/GL substitute or busy-loop fallback.
2. Run Cobalt itself: HTML, JavaScript/DOM/timers, keyboard/mouse/scroll and
   verified HTTPS with concurrent assets, interactive >=60s, close/relaunch.

Details: HANDOFF.md; test evidence: build/system-tests/. Never push.
