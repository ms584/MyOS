# MyOS project state

Updated: 2026-10-01. Repo: /home/xytron404/myos (WSL Ubuntu), branch main.
Native launcher checkpoint follows local 36a5cea; local commit only, no push.
Origin/main remains 54ef570 after fetch at 07:28 +07. Owner pushes manually.

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
- NEW `elf starboard`: actual SbEventHandle/SABI entry, owned argv/link, guarded
  2 MiB main stack and actual stack queries/limits: PHASE332_LAUNCHER.md.
- Pthread masks are the pinned 128-byte ABI; actual signal delivery is absent.
- Logging cap: 16 MiB per file, two rotations, 48 MiB total; feedback rejected.
  Never inspect activity.log through run_logged.py; use unwrapped tail -F.

## Current evidence (2026-10-01, +07)
- Pinned Cobalt/44-header hashes and 243-row contract register PASS again 06:49.
- Host-only Mesa probe passed 03:12; TLS/C++ dependency closure inspected 07:03
  but NOT ported or selected as native backend: PHASE332_RENDERER.md.
- Small-PIC runtime adopted after native dynamic/runtime/storage/UI/events
  regression PASS 06:31:41, including >60s lifecycle and exact reclamation.
- Native launcher PASS 06:39:26: pinned ABI/SABI, zero entry/sectionless,
  malformed exports, URL/args, 256 KiB stack use, atomic query rejection,
  getter-before-ctors, real input, guard/NX/getter faults, stop/cancel/relaunch.
- Expanded scoped regression PASS 06:47:58; actual Cobalt link/launcher still
  reject 82 missing imports before user execution; both OOM paths reclaim fully.
- Replacement FULL tests/test_system.py PASS observed 07:21; normal restored:
  kernel 253464, image 1474560, runtime 29840, BSS end 0x7d400; limits preserved.
- UEFI DMA/actual frames/PCM + DHCP/DNS PASS 07:21; launcher/events/native UI
  and ASan/UBSan PASS, sequence exit=0 at 07:22:32. No test remains running.
- Supplemental BIOS launcher PASS 07:27:28 with exact guard/NX/getter page-fault
  bits/addresses and twelve malformed exports. Normal image unchanged.

## Required next work
1. Native EGL/GLES/TLS dependency closure and remaining required contracts;
   preserve private runtime thread identity and test native TLS ownership.
2. Run Cobalt itself: HTML, JavaScript/DOM/timers, keyboard/mouse/scroll and
   verified HTTPS with concurrent assets, interactive >=60s, close/relaunch.

Details: HANDOFF.md, PHASE331_TCP.md; test evidence: build/system-tests/.
Never close 33.1/33.2 on symbol counts or fixture success. Never push.
