# MyOS project state

Updated: 2026-10-01. Repo: /home/xytron404/myos (WSL Ubuntu), branch main.
Baseline before this TCP/DMA checkpoint: c835cfe (main). Local-only work.
Origin/main remains 54ef570 after the latest fetch. Owner pushes manually.

## Scope and status
- Phase 33.1/33.2 remain OPEN: Cobalt has not executed in MyOS.
- 149/242 strong imports have supported-subset fixture coverage; 93 remain
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
  Bounds/gaps: PHASE331_TCP.md; AF_UNIX duplex: PHASE331_SOCKETPAIR.md.
- Long-running elf live lifecycle, cancellation/fault/OOM/relaunch reclamation:
  PHASE331_LIFECYCLE.md. Init/fini teardown: PHASE332_FINALIZERS.md.
- Pthread masks are the pinned 128-byte ABI; actual signal delivery is absent.
- Logging cap: 16 MiB per file, two rotations, 48 MiB total; feedback rejected.
  Never inspect activity.log through run_logged.py; use unwrapped tail -F.

## Evidence at current checkpoint
- Concurrent Ring 3 TCP tests passed at 00:17 +07 on 2026-10-01: two native
  workers, independent patterned transfers, shutdown/EOF, readiness, dup/close,
  refused port, no NIC, exhausted pool, repeat launch and worker cancellation.
  Each measured page/heap cleanup returns to baseline.
- TCP state-machine ASan/UBSan tests passed at 23:55 and in the full suite:
  partial ACK/retry, wraparound, invalid/reset/FIN/duplicate/out-of-order,
  window bounds, timeout, handle ownership/reuse and pool teardown.
- Pinned Cobalt/header hashes and 243-row contract register passed at 01:10 +07.
- Two full runs exposed SB16 DMA at 0x70000 overlapping grown kernel BSS.
  BIOS DMA moved to 0x90000; UEFI allocates owned stack/DMA via boot ABI v2.
  Self-test-image video/audio passed at 00:46/00:47; UEFI network at 00:55.
  Final UEFI DMA/graphics/real PCM (435/655 Hz) passed at 01:29 +07.
- Native legacy HTTPS and network regression passed at 00:22/00:23 +07.
- Full tests/test_system.py passed at 01:28 +07 after the DMA/layout fix:
  native TCP, runtime/lifecycle, persistence, memory/fault and VGA fallback.
  Normal framebuffer image restored; kernel 241752, image 1474560 bytes.
- Cobalt-only staging passed at 01:30: 149 resolved/93 missing, weak TLS zero,
  relocation/RELRO/reclamation and 64 MiB OOM rollback. Engine NOT executed.

## Required next work
1. Implement resolver/interface/required socket options and real bounded I/O
   deadlines; audit/integrate native Cobalt TLS rather than substituting fetches.
2. Resolve remaining required signals/platform contracts and native renderer
   feasibility. Build the actual Evergreen/event/window/input adapters.
3. Run Cobalt itself: HTML, JavaScript/DOM/timers, keyboard/mouse/scroll and
   verified HTTPS with concurrent assets, interactive >=60s, close/relaunch.

Details: HANDOFF.md, PHASE331_TCP.md; test evidence: build/system-tests/.
Never close 33.1/33.2 on symbol counts or fixture success. Never push.
