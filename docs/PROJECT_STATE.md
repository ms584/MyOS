# MyOS project state

Updated: 2026-09-30. Repo: /home/xytron404/myos (WSL Ubuntu).
Prior baseline: 57e2b97, branch main; lifecycle/pthread is the tested local checkpoint.
After git fetch origin, origin/main has no new changes; commits remain local.

## Scope and status
- Phase 33.1/33.2 remain OPEN: Cobalt has not executed in MyOS.
- 139/242 strong imports have fixture coverage; 103 remain plus a weak TLS symbol.
  Symbol counts and fixture passes do not prove full ABI compatibility.
- Phase 32 and Phase 33 are NOT PASSED; actual YouTube playback is unproven.
- Sequence: finish 33.1/33.2 -> 33.3 standard media -> 33.4 YouTube ->
  33.5 Rickroll. Check Phase 32 search/select/play separately.
- The owner pushes manually. Do not push.

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

## Validation at checkpoint
- Focused fixtures pass 128 KiB asset reads and 8 KiB binary reboot persistence.
- Pinned Cobalt binary/header hashes and the 243-row contract register pass.
- File/socketpair full regressions passed at 19:02/19:38 +07; normal image restored.
- Pinned Cobalt staging passed at 256 MiB (139 bound, 103 missing), with all
  staged pages reclaimed; 64 MiB OOM rollback passed. This does not run Cobalt.
- Latest focused lifecycle/pthread/persistence/staging passed at 19:51 +07.
- Lifecycle/pthread full regression passed at 20:17 +07; normal image restored.

## Known gaps and next dependency
1. Add tested single-image DT_FINI/FINI_ARRAY lifecycle needed by the pinned
   binary; continue required TLS/storage/Starboard contracts with semantic tests.
2. Replace legacy single-stream TCP with independent cancellable connections;
   test concurrent requests, partial I/O, readiness, resolver/options and TLS.
3. Build the real Evergreen launcher, event loop, graphics/input bridge and run
   native Cobalt HTML/JavaScript/DOM/input plus HTTPS acceptance in MyOS.
- Renderer/backend and native acceptance remain open. Do not start Phase 33.3.

## Read only what the task needs
- Rules/gates: AGENTS.md and docs/PHASE33_EXECUTION_PLAN.md.
- Runtime audit: user/platform-runtime.c, tests/test_elf_dynamic.py.
- Full regression: tests/test_system.py; evidence in build/system-tests/.
  Never inspect activity.log through run_logged.py: output feeds the same log.
- Import contracts: docs/PHASE331_ABI.md, docs/PHASE331_IMPORTS.csv.

## Completion rule
- Do not close Phase 33.1 or 33.2 until Cobalt itself opens HTML, executes
  JavaScript/DOM, receives real keyboard/mouse input, and uses HTTPS in MyOS for
  the documented endurance gate. Phase 33.3 has not started.
