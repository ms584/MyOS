# MyOS project state

Updated: 2026-10-01. Repo: /home/xytron404/myos (WSL Ubuntu), branch main.
Runtime baseline 8003d20; renderer measurement continuation is local only.
Origin/main remains 54ef570 after the latest fetch. Owner pushes manually.

## Scope and status
- Phase 33.1/33.2 remain OPEN: Cobalt has not executed in MyOS.
- 150/242 strong imports have supported-subset fixture coverage; 92 remain
  plus the weak TLS symbol. Counts do not prove full ABI compatibility.
- Phase 32/33 are NOT PASSED; actual YouTube playback is unproven.
- Do not start Phase 33.3 before native Cobalt HTML/JS/DOM/input/HTTPS acceptance.

## Current implementation
- Clocks/pthreads/keys/emutls and shared descriptors, eventfd/pipe/poll/epoll/
  FIONREAD: PHASE331_WAIT.md, PHASE331_PTHREAD.md, PHASE331_SYSTEM.md.
- Refcounted files/dirs, large/positional/vectored reads and real v2-disk fsync:
  PHASE331_FILES.md. Durable Starboard records: PHASE331_STORAGE.md.
- NEW: independently owned IPv4 TCP streams (32 machine-wide, 8 KiB RX/4 KiB
  TX each), process-private generation handles, nonblocking kernel syscalls,
  per-connection retry/sequence/EOF, descriptor cancellation and timer-driven
  network wakeups. Runtime socket/connect/send/recv/shutdown, address getters,
  selected SO getters, dup/fcntl, poll/epoll and FIONREAD have native fixtures.
  NEW: shared per-endpoint timeval options and fixed I/O/connect deadlines;
  see PHASE331_SOCKET_OPTIONS.md for tested subsets and outstanding options.
  Bounds/gaps: PHASE331_TCP.md; AF_UNIX duplex: PHASE331_SOCKETPAIR.md.
- Long-running elf live lifecycle, cancellation/fault/OOM/relaunch reclamation:
  PHASE331_LIFECYCLE.md. Init/fini teardown: PHASE332_FINALIZERS.md.
- Pthread masks are the pinned 128-byte ABI; actual signal delivery is absent.
- Logging cap: 16 MiB per file, two rotations, 48 MiB total; feedback rejected.
  Never inspect activity.log through run_logged.py; use unwrapped tail -F.

## Current evidence (2026-10-01, +07)
- Native TCP, state-machine ASan/UBSan and socket-deadline tests passed 02:18:
  ownership/reuse, partial I/O/retry, sequence/window/EOF, cancellation/cleanup.
  Timeval validation/rounding/overflow, endpoint/dup sharing, fixed waits and
  actual TCP packet-stall recovery pass twice. Persistent failure survives
  public SO_ERROR consumption through the Ring 3 boundary.
- Pinned Cobalt/header hashes and 243-row contract register passed at 02:05.
- First full rerun stopped at 02:12 for the consumed-error fix; not a PASS.
- Replacement full tests/test_system.py PASSED at 02:49: native sockets,
  runtime/lifecycle, persistence, low-memory/fault recovery and VGA fallback.
  Normal framebuffer image restored: kernel 242808, image 1474560 bytes.
- Cobalt-only staging PASSED at 02:50: 150 resolved/92 missing, weak TLS zero,
  relocations/RELRO/reclamation and 64 MiB OOM rollback; engine NOT executed.
- Packaged UEFI owned DMA/stack, real pixels/frames/PCM (435/655 Hz) and
  RTL8139/DHCP/DNS tests PASSED at 02:50.
- Host-only Mesa core/texture/glyph probe passed 03:12; 44 EGL/246 GLES slots
  audited. PT_TLS/shared dependencies block native: PHASE332_RENDERER.md.

## Required next work
1. Implement resolver/interfaces/remaining options and native Cobalt TLS.
   Runtime checkpoint committed locally; no push.
2. Resolve remaining required signals/platform contracts and native renderer
   feasibility. Build the actual Evergreen/event/window/input adapters.
3. Run Cobalt itself: HTML, JavaScript/DOM/timers, keyboard/mouse/scroll and
   verified HTTPS with concurrent assets, interactive >=60s, close/relaunch.

Details: HANDOFF.md, PHASE331_TCP.md; test evidence: build/system-tests/.
Never close 33.1/33.2 on symbol counts or fixture success. Never push.
