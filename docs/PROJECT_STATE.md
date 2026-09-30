# MyOS project state

Updated: 2026-09-30. Repo: /home/xytron404/myos (WSL Ubuntu).
Base checkpoint 18191dd, branch main; TLS follow-up passed; Starboard storage
slice is committed locally and verified by focused plus full dynamic regression.
Fetched origin: no new origin/main commits. Owner pushes manually.

## Scope and status
- Phase 33.1/33.2 remain OPEN: Cobalt has not executed in MyOS.
- 144/242 strong imports have fixture coverage; 98 remain plus a weak TLS symbol.
  Counts and fixtures do not prove full ABI compatibility.
- Phase 32 and Phase 33 are NOT PASSED; actual YouTube playback is unproven.
- Sequence: finish 33.1/33.2 -> 33.3 media -> 33.4 YouTube -> 33.5 Rickroll.
  Check Phase 32 search/select/play separately.

## Current implementation slice
- Clocks/pthreads, shared descriptors, eventfd/pipe/poll/epoll/FIONREAD and
  system ABI: PHASE331_WAIT.md, PHASE331_PTHREAD.md, PHASE331_SYSTEM.md.
- Files/dirs use refcounted shared offsets; large immutable asset random reads,
  positional/vectored I/O and real fsync/reboot persistence: PHASE331_FILES.md.
- Duplex socketpair with shutdown/readiness/cancellation: PHASE331_SOCKETPAIR.md.
  Network TCP sockets remain absent; the 256 KiB kernel-file cap remains.
- Command logging is bounded: 16 MiB per log, two rotations (48 MiB total),
  feedback rejection; logger regression passes. Watch with unwrapped tail -F.
- New elf live mode runs beyond 60 seconds; Escape stops/reaps blocked workers.
  Repeat launch, page/heap/file cleanup, normal exit, fault and OOM are tested.
  See PHASE331_LIFECYCLE.md: force cancellation is not a Cobalt event/input bridge.
- Pthread masks now initialize the pinned 128-BYTE ABI; semaphore errno and
  concurrent overflow, and pthread name bounds are tested. No signal delivery.
- DT_FINI/reverse FINI_ARRAY, exit-status preservation, malformed/wrapped targets,
  fault/timeout/cancel and repeat-launch cleanup pass native fixtures.
  See PHASE332_FINALIZERS.md. This is not the Cobalt event-loop launcher.
- `pthread_key_t` now uses the pinned pointer ABI (`void *`); full-width bounds,
  canaries, all 32 slots and destructor passes are tested. Upstream compiler-rt
  emutls runs as a Ring 3 fixture; this does not execute Cobalt itself.
- Starboard storage now uses validated two-generation record files and v2-disk
  fsync commits, with tombstones for durable deletion. Ring 3 reboot tests cover
  immediate read-back, name/one-open validation, persistence and deletion; see
  PHASE331_STORAGE.md. This is not a Cobalt browser acceptance test.

## Validation at checkpoint
- Pinned actual Cobalt staging passed at 256 MiB (144 strong imports resolved,
  98 missing), with all staged pages reclaimed; 64 MiB OOM rollback passed at
  23:13 +07. The image was not executed; this does not run Cobalt.
- Lifecycle/pthread regression passed at 20:17; finalizer focus at 20:24 and
  full regression at 20:53 +07 (normal kernel 232368 bytes).
- TLS key/emutls ELF regression passed at 21:27; weak-TLS audit and full
  tests/test_system.py passed at 21:54 +07 (normal kernel 232368 bytes).
- Full tests/test_system.py passed at 22:54 +07 (normal framebuffer image
  restored; kernel 234864 bytes). After the final storage tombstone semantic
  correction, the focused storage and full tests/test_elf_dynamic.py passed at
  23:01 +07; normal image is 1474560 bytes with kernel 234896 bytes.
- The final `tests/test_elf_dynamic.py --cobalt build/engine-evaluation/libcobalt.so`
  run passed at 23:13 +07: 144 resolved/98 missing and 64 MiB OOM rollback.
  Actual Cobalt execution and web acceptance remain unproven.

## Known gaps and next dependency
1. Continue missing required pthread/signal, networking and renderer
   interfaces. The TLS fixture is not the real Cobalt weak initializer
   executing.
2. Replace legacy single-stream TCP with independent cancellable connections;
   test concurrent requests, partial I/O, readiness, resolver/options and TLS.
3. Build the real Evergreen launcher, event loop, graphics/input bridge and run
   native Cobalt HTML/JavaScript/DOM/input plus HTTPS acceptance in MyOS.
- Renderer/backend and native acceptance remain open. Do not start Phase 33.3.

## Evidence locations
- AGENTS.md, PHASE33_EXECUTION_PLAN.md; runtime: user/platform-runtime.c.
- Tests: tests/test_elf_dynamic.py, tests/test_system.py; build/system-tests/.
- Import contracts: PHASE331_ABI.md, PHASE331_IMPORTS.csv.
- Never inspect activity.log through run_logged.py: output feeds the same log.

## Completion rule
- Do not close Phase 33.1 or 33.2 until Cobalt itself opens HTML, executes
  JavaScript/DOM, receives real keyboard/mouse input, and uses HTTPS in MyOS for
  the documented endurance gate. Phase 33.3 has not started.
