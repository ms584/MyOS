# MyOS project state

Updated: 2026-10-01. Repo: /home/xytron404/myos (WSL Ubuntu), branch main.
Native window/input checkpoint follows local 2f4425f; owner keeps the push step.
Origin/main remains 54ef570 after fetch at 04:02 +07. Owner pushes manually.

## Scope and status
- Phase 33.1/33.2 remain OPEN: Cobalt has not executed in MyOS.
- 156/242 strong imports have supported-subset fixture coverage; 86 remain
  plus the weak TLS symbol. Counts do not prove full ABI compatibility.
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
  PHASE332_NATIVE_UI.md. NOT EGL/GLES, SbInputData dispatch or Cobalt execution.
- Pthread masks are the pinned 128-byte ABI; actual signal delivery is absent.
- Logging cap: 16 MiB per file, two rotations, 48 MiB total; feedback rejected.
  Never inspect activity.log through run_logged.py; use unwrapped tail -F.

## Current evidence (2026-10-01, +07)
- Previous full system regression PASSED 02:49 on runtime 8003d20; normal
  kernel was 242808 bytes. Its Cobalt staging/UEFI checks passed 02:50.
  Those results predate the new native UI changes; they do not validate them.
- Native window/input fixture and supplementary ASan/UBSan passed 04:00,
  then again on the corrected self-test build at 04:06. Actual VRAM/screen,
  real key/mouse, waits, ownership, queue bounds and exact page/heap checked.
- Pinned Cobalt/header hashes and 243-row contract register passed 04:01.
- First new full attempt stopped at self-test link: BSS 0x809c0 exceeds 0x80000.
  Stripe scratch saves 4864 bytes; self-test BSS 0x7f6c0, kernel 256280 bytes.
  Original stack/image safety limits preserved; build/kernel.map records them.
- Replacement full tests/test_system.py PASSED 04:35 including native UI and
  VGA ENOTSUP fallback. Normal kernel 248088, image 1474560, BSS end 0x7d6c0.
- Cobalt staging/RELRO/reclamation/64 MiB OOM PASSED 04:37: 156 resolved,
  86 missing, weak TLS zero; engine NOT executed. Packaged UEFI DMA/PCM/network
  PASSED 04:37 and native UI/input/fault/cancel/relaunch/cleanup PASSED 04:38.
- Host-only Mesa core/texture/glyph probe passed 03:12; 44 EGL/246 GLES slots
  audited. PT_TLS/shared dependencies block native: PHASE332_RENDERER.md.

## Required next work
1. Implement native renderer/winsys EGL/GLES, pinned SbEventHandle lifecycle,
   callback cancellation and focused SbInputData translation/dispatch. Complete
   resolver/interfaces/options/signals and native Cobalt TLS as required.
2. Run Cobalt itself: HTML, JavaScript/DOM/timers, keyboard/mouse/scroll and
   verified HTTPS with concurrent assets, interactive >=60s, close/relaunch.

Details: HANDOFF.md, PHASE331_TCP.md; test evidence: build/system-tests/.
Never close 33.1/33.2 on symbol counts or fixture success. Never push.
