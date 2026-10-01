# MyOS project state

Updated: 2026-10-01. Repo: /home/xytron404/myos (WSL Ubuntu), branch main.
Tested native jump/stack-allocation checkpoint follows 0e24ba7. No push.
Origin/main is 32b1fc2 (observed 15:27 +07). Owner pushes manually.

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
- Math/pure/native SDK and stream/stat/environment details: HANDOFF.md + SDK docs.
- Real smoothsort/Mesa TLS sort/PRNG/finalizer SDK: PHASE332_SORT_SDK.md.
- Real Mesa rwlock80/time/C++ aliases + full dependency map: PHASE332_RWLOCK_SDK.md.
- String/token/C-locale/math-profile batch: PHASE332_STRING_EXTENSIONS.md.
- Sealed conversion/scanf/UTF8 + real Mesa wrapper: PHASE332_SCAN_SDK.md.
- Sealed native jumps/registers/stack allocation: PHASE332_JUMP_SDK.md.
- Allocation/atexit/DSO finalization SDK: PHASE332_CXX_LIFETIME.md; no exceptions.
- Renderer configure PASS/compile FAIL: PHASE332_RENDERER.md; no host libs/OpenMP.
- Pinned 128-byte signal masks; no signal delivery. Logs capped at 48 MiB;
  never inspect activity.log via run_logged.py (use unwrapped tail -F).

## Current evidence (2026-10-01, +07)
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
- Final scanner/Mesa74650 BIOS/UEFI PASS, chain exit0 18:29:46: MPFR209,
  rounding/readonly guard edges/scanf/UTF8/OOM/8workers/resources.
  Formatter85049 PASS18:19:26. No kernel/runtime/image change.
- Native jump55677 BIOS PASS18:48:48/UEFI PASS18:49:14: C/C++, six launches,
  register restoration/timed preemption/nested stacks/8workers/exact resources.
- Remaining: exit/abort, OS-info/file locks/stdio details and
  genuine target C++ dependencies. No native backend or Cobalt execution.
- Actual pinned Cobalt refusal/OOM checks 2578 exited 0 at 10:26:26:
  160 resolved, 82 strong imports missing; NOT executed or web acceptance.
- Normal: kernel 252952, image 1474560, runtime 29136, BSS 0x7d2a0.
  BIOS SHA256 765a3f0c65e9746e80a14226ac2a142651ecbca1e3858cf2c5b2a61212335e5d.
- Process-test kernel 261144; cap unchanged. Normal UEFI restored, hashes
  in PHASE331_UNLINK.md; all jobs terminal, no shared-image owner.

## Required next work
1. Read PHASE332_JUMP_SDK.md and full u8ev3uvw compile.log. Audit exit/abort
   against PHASE332_FINALIZERS.md: raw SYS_EXIT skips FINI; pthread_exit is
   not process exit. Implement real lifecycle semantics, then OS-info/C++/EGL.
   Audit absolute-wait edges; no host libc/GL substitute or busy-loop fallback.
2. Run Cobalt itself: HTML, JavaScript/DOM/timers, keyboard/mouse/scroll and
   verified HTTPS with concurrent assets, interactive >=60s, close/relaunch.

Details: HANDOFF.md; test evidence: build/system-tests/. Never push.
