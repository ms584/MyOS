# MyOS project state

Updated: 2026-10-01. Repo: /home/xytron404/myos (WSL Ubuntu), branch main.
Native event/lifecycle checkpoint follows local 2831cfe; owner keeps the push step.
Origin/main remains 54ef570 after fetch at 06:04 +07. Owner pushes manually.

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
- Independent IPv4 TCP streams, endpoint timeval options and fixed deadlines,
  descriptor cancellation/dup/poll/epoll/FIONREAD: PHASE331_TCP.md and
  PHASE331_SOCKET_OPTIONS.md. AF_UNIX duplex: PHASE331_SOCKETPAIR.md.
- Long-running elf live lifecycle, cancellation/fault/OOM/relaunch reclamation:
  PHASE331_LIFECYCLE.md. Init/fini teardown: PHASE332_FINALIZERS.md.
- NEW: owned native fullscreen window, copied RGBA/XRGB framebuffer and raw
  PS/2 keyboard/mouse, wait/close/reuse/fault/cancel/relaunch reclamation:
  PHASE332_NATIVE_UI.md. NOT EGL/GLES or Cobalt execution.
- NEW native callbacks/cancel, lifecycle/preload, focused SbInputData,
  real visibility/hidden presents and owned heap queue: PHASE332_EVENTS.md.
- Pthread masks are the pinned 128-byte ABI; actual signal delivery is absent.
- Logging cap: 16 MiB per file, two rotations, 48 MiB total; feedback rejected.
  Never inspect activity.log through run_logged.py; use unwrapped tail -F.

## Current evidence (2026-10-01, +07)
- Pinned Cobalt/44-header hashes and 243-row contract register PASS 05:29:52.
- Stack/image safety limits preserved; build/kernel.map records layout checks.
- Host-only Mesa core/texture/glyph probe passed 03:12; 44 EGL/246 GLES slots
  audited. PT_TLS/shared dependencies block native: PHASE332_RENDERER.md.
- Foreground snapshot initially exceeded BSS; owned heap queue fixes the bound.
- Native events PASS 05:29:13, including infinite wait/worker teardown stress;
  SYS_WAKE count=0 was a real bug (wakes nobody), fixed to UINT32_MAX broadcast.
- Native UI + updated ASan/UBSan PASS 05:29:23. Normal kernel 253624 bytes.
- Replacement full tests/test_system.py PASSED 06:00:21 including new events,
  native UI, all prior runtime/lifecycle/storage/TCP, fault/RAM and VGA fallback.
- Normal image restored: kernel 253624, image 1474560; self-test kernel 261816,
  BSS end 0x7f460. All original stack/DMA/image limits preserved.
- Cobalt staging/RELRO/reclamation/64 MiB OOM PASS 06:04:49: 160 resolved,
  82 missing, weak TLS zero; engine NOT executed. UEFI package/DMA/PCM/network
  PASS 06:05; extended events PASS 06:10/06:11 BIOS/UEFI; UI PASS 06:06:18 UEFI.
- Isolated small-PIC probe saves 5238 runtime bytes; NOT natively validated or
  adopted. Reproduction: tools/measure-runtime-pic.sh and PHASE332_EVENTS.md.

## Required next work
1. Validate headroom strategy, implement real SbEventHandle/SABI launcher with
   guarded/sized main stack; native EGL/GLES/TLS and remaining contracts next.
2. Run Cobalt itself: HTML, JavaScript/DOM/timers, keyboard/mouse/scroll and
   verified HTTPS with concurrent assets, interactive >=60s, close/relaunch.

Details: HANDOFF.md, PHASE331_TCP.md; test evidence: build/system-tests/.
Never close 33.1/33.2 on symbol counts or fixture success. Never push.
