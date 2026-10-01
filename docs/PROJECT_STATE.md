# MyOS project state

Updated: 2026-10-01. Repo: /home/xytron404/myos (WSL Ubuntu), branch main.
Tested open-file unlink checkpoint follows b789051. No push.
Origin/main is 32b1fc2 (observed 15:27 +07). Owner pushes manually.

## Scope and status
- Phase 33.1/33.2 remain OPEN: Cobalt has not executed in MyOS.
- Native fixture coverage: 160/242 strong imports; 82 plus weak TLS remain.
  Supported subsets/counts do not prove complete ABI or actual Cobalt operation.
- Phase 32/33 NOT PASSED; actual YouTube playback is unproven.
- Do not start Phase 33.3 before native Cobalt HTML/JS/DOM/input/HTTPS acceptance.

## Current implementation
- Clocks/pthreads/keys/emutls and shared descriptors, eventfd/pipe/poll/epoll/
  FIONREAD: PHASE331_WAIT.md, PHASE331_PTHREAD.md, PHASE331_SYSTEM.md.
- Files/dirs/read/fsync: PHASE331_FILES.md; records: PHASE331_STORAGE.md.
- Concurrent TCP/dup/readiness/deadlines: PHASE331_TCP.md,
  PHASE331_SOCKET_OPTIONS.md; AF_UNIX duplex: PHASE331_SOCKETPAIR.md.
- Live cancellation/fault/OOM: PHASE331_LIFECYCLE.md; PHASE332_FINALIZERS.md.
- Window/input: PHASE332_NATIVE_UI.md (NOT EGL/GLES); events: PHASE332_EVENTS.md.
- Guarded SABI launcher: PHASE332_LAUNCHER.md.
- Native TLS/bounded slots/detached cleanup: PHASE331_NATIVE_TLS.md (BIOS/UEFI).
- C++ guard/C11 SDK: PHASE332_CXX_GUARD.md, PHASE332_C11_SDK.md.
- Timed mutex/exit/join/detach: PHASE331_THREAD_SERVICES.md.
- Mesa thread utils: PHASE332_THREAD_UTIL.md (BIOS/UEFI, not graphics).
- Math/header cross-probe: PHASE332_MATH_SDK.md; renderer NOT linked/executed.
- Pure/native SDK: PHASE332_TIME_SDK.md, PHASE332_INTEGER_SDK.md,
  PHASE332_ENVIRONMENT.md, PHASE332_FORMAT_SDK.md, PHASE332_STREAM_SDK.md,
  PHASE332_MEMSTREAM_SDK.md, PHASE332_STRING_SDK.md, PHASE332_STAT_SDK.md.
- Open-file unlink/name reuse/last-close: PHASE331_UNLINK.md.
- Allocation/atexit/DSO finalization SDK: PHASE332_CXX_LIFETIME.md; no exceptions.
- Target Mesa configure PASS; real compile still fails: PHASE332_RENDERER.md.
  No Linux libs/OpenMP; native memset/memchr and real Mesa blob utility tested.
- Pinned 128-byte signal masks; no signal delivery. Logs capped at 48 MiB;
  never inspect activity.log via run_logged.py (use unwrapped tail -F).

## Current evidence (2026-10-01, +07)
- Previous baseline full/SDK/UEFI 52428 PASS 10:23:34, before unlink change.
- Full coverage completed as passed prefix 83161 + resumed suffix 89247
  exit 0 16:33:42, all remaining tests/VGA/finally; normal restored.
  NOT uninterrupted full-run PASS. Earlier runs hit media 30s boot timeout;
  diagnostic proved shell in 30.74s. Media boot waits now common 60s,
  all assertions retained; details/provenance PHASE331_UNLINK.md.
- Unlink/Mesa/snapshot 79689 PASS 15:47:30; sdk-nh2mfw41, private normal boot.
  Live handles/name reuse/128 close cycles/cancel/relaunch/exact reclaim;
  actual Mesa anonymous files and v1/v2 live-orphan snapshot/reboot PASS.
- Fresh normal UEFI/unlink 96289 native test PASS 16:34:55, sdk-oeghfxmx;
  enclosing hash command typo failed, corrected artifact evidence 16:36:11.
- Earlier stat/headers/streams 16406 PASS 15:23:28; strings C/C++ 58821 PASS.
  C11/thread/math/format/environment/memstream details in respective docs.
- Renderer 99936 exit 1 15:57:35, target-config-qjsmm8xd: configure/boundary
  PASS, compile FAIL. mkstemp cleared; abort/rand/sscanf/atexit/OS-info,
  socket ENOSYS and C++ u_qsort <thread> remain. No host libraries.
- Actual pinned Cobalt refusal/OOM checks 2578 exited 0 at 10:26:26:
  160 resolved, 82 strong imports missing; NOT executed or web acceptance.
- Normal: kernel 252952, image 1474560, runtime 29136, BSS 0x7d2a0.
  BIOS SHA256 765a3f0c65e9746e80a14226ac2a142651ecbca1e3858cf2c5b2a61212335e5d.
- Process-test kernel 261144; cap unchanged. Normal UEFI restored, hashes
  in PHASE331_UNLINK.md; all jobs terminal, no shared-image owner.

## Required next work
1. Close remaining libc/OS-info/C++ renderer failures, then native EGL/GLES.
   Audit absolute-wait edges; no host libc/GL substitute or busy-loop fallback.
2. Run Cobalt itself: HTML, JavaScript/DOM/timers, keyboard/mouse/scroll and
   verified HTTPS with concurrent assets, interactive >=60s, close/relaunch.

Details: HANDOFF.md; test evidence: build/system-tests/. Never push.
