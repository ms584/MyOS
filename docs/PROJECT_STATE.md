# MyOS project state

Updated: 2026-10-01. Repo: /home/xytron404/myos (WSL Ubuntu), branch main.
Native TLS/SDK C++ checkpoint follows daa0cea; local only. No push.
Origin/main remains 54ef570 after fetch at 08:43 +07. Owner pushes manually.

## Scope and status
- Phase 33.1/33.2 remain OPEN: Cobalt has not executed in MyOS.
- Native fixture coverage: 160/242 strong imports; 82 plus weak TLS remain.
  Supported subsets/counts do not prove complete ABI or actual Cobalt operation.
- Phase 32/33 are NOT PASSED; actual YouTube playback is unproven.
- Do not start Phase 33.3 before native Cobalt HTML/JS/DOM/input/HTTPS acceptance.

## Current implementation
- Clocks/pthreads/keys/emutls and shared descriptors, eventfd/pipe/poll/epoll/
  FIONREAD: PHASE331_WAIT.md, PHASE331_PTHREAD.md, PHASE331_SYSTEM.md.
- Refcounted files/dirs, large/positional/vectored reads and real v2-disk fsync:
  PHASE331_FILES.md. Durable Starboard records: PHASE331_STORAGE.md.
- Concurrent TCP/dup/readiness/deadlines: PHASE331_TCP.md,
  PHASE331_SOCKET_OPTIONS.md; AF_UNIX duplex: PHASE331_SOCKETPAIR.md.
- Long-running elf live lifecycle, cancellation/fault/OOM/relaunch reclamation:
  PHASE331_LIFECYCLE.md. Init/fini teardown: PHASE332_FINALIZERS.md.
- Native fullscreen window/pixels/input: PHASE332_NATIVE_UI.md; NOT EGL/GLES.
- Native callbacks/input/visibility/cancel/preload: PHASE332_EVENTS.md.
- Guarded 2 MiB SABI/SbEventHandle launcher: PHASE332_LAUNCHER.md.
- NEW native PT_TLS/GD/LD/IE/TLSDESC, distinct FS/templates, bounded slots and
  syscall-time detached cleanup: PHASE331_NATIVE_TLS.md; BIOS/UEFI tested.
- SDK C++ guard provider native-tested; not yet renderer-linked or a fixed
  runtime export: PHASE332_CXX_GUARD.md. No import count increase.
- Pinned 128-byte signal masks; no signal delivery. Logs capped at 48 MiB;
  never inspect activity.log via run_logged.py (use unwrapped tail -F).

## Current evidence (2026-10-01, +07)
- Pinned Cobalt/44-header hashes and 243-row contract register PASS again 07:57.
- Host Mesa reduced-C++ build/probe PASS 08:17:49; 186->176 strong undefined,
  four Linux dependencies remain; NOT native: PHASE332_RENDERER.md.
- Native TLS GD/IE/TLSDESC BIOS PASS 07:49:05; expanded LD/zero-only/alignment,
  12 invalid/3 unsupported cases, RELRO/descriptor bounds, 36 worker lifetimes,
  exact NX/guard/worker faults, cancel/OOM/relaunch PASS 07:56:26.
- Normal: kernel 253624, image 1474560, runtime 29936, BSS 0x7d540.
- Full 72357 stopped 08:11:17 at v2 reboot's 40s boot bound; normal restored.
  Diagnostic reboot reached prompt by 40.01s and persisted app status 26.
  Apps use existing 60s VM boot budget; functional checks unchanged.
- Session 55894 reran apps/all later full checks, Cobalt staging and UEFI;
  exit 0 at 08:44:17. Earlier unchanged passes retained: resumed full coverage,
  NOT a fresh uninterrupted full invocation. Normal restored; no live tests.
- SDK C++ guard native PASS 08:28:46/08:31:04, abort/wake/publication, recursion
  fault, blocked-worker cancel/relaunch, exact resources; immutable private image.
- Apps/TLS alignment, storage/16+256 MiB, faults/VGA and UEFI TLS/launcher/
  events/UI/DMA/frames/captured PCM/DHCP/DNS all PASS. SDK guard UEFI PASS
  08:42:33. Actual Cobalt refused with 82 strong imports missing, NOT executed.

## Required next work
1. Target libc/C11/C++ and native EGL/GLES closure; remaining required contracts.
   Mesa needs real blocking pthread_mutex_timedlock and destructor-safe
   pthread_exit; never adopt its busy-loop timedlock fallback.
2. Run Cobalt itself: HTML, JavaScript/DOM/timers, keyboard/mouse/scroll and
   verified HTTPS with concurrent assets, interactive >=60s, close/relaunch.

Details: HANDOFF.md, PHASE331_TCP.md; test evidence: build/system-tests/.
Never close 33.1/33.2 on symbol counts or fixture success. Never push.
