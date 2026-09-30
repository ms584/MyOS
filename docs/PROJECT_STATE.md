# MyOS project state

Updated: 2026-09-30. Read this first; open detailed docs only as needed.
Repo: /home/xytron404/myos (WSL Ubuntu).
Implementation checkpoint before this slice: b6c74b5, branch main.
Local main matched origin/main after `git fetch origin` at the start of this slice.
Check current Git status/HEAD each new chat.

## Scope and status
- Phase 33.1/33.2 remain OPEN: Cobalt has not executed in MyOS.
- 75/242 strong imports resolve; 167 remain plus a separate weak TLS symbol.
  Symbol counts and fixture passes do not prove full ABI compatibility.
- Phase 32 and Phase 33 are NOT PASSED; actual YouTube playback is unproven.
- Sequence: finish 33.1/33.2 -> 33.3 standard media -> 33.4 YouTube ->
  33.5 Rickroll. Check Phase 32 search/select/play separately.
- Default: the owner pushes manually. Push only on explicit user instruction.

## Current implementation slice
- Added real realtime and monotonic nanosecond clocks, absolute/relative sleep,
  pthread condition clock attributes, timed waits and per-thread signal-mask
  state/inheritance. Invalid handles, TLS-key reuse, join ownership and detached
  thread resource reclamation now have adversarial fixture coverage.
- Added a shared refcounted runtime descriptor/open-object model with generation
  validation and wake-on-close cancellation. Eventfd, pipe/pipe2, poll, epoll
  and FIONREAD use it; ordinary MyOS file descriptors are forwarded through
  the same read/write/close entry points. Sockets remain future consumers.
- The self-test kernel outgrew the old 0x73000 linker guard. BIOS and UEFI
  reservations were audited and the in-memory guard now ends at 0x80000; the
  independent 256 KiB kernel-file cap is unchanged.

## Validation at checkpoint
- Focused `tests/test_elf_dynamic.py` passed, including the new pthread/clock
  and descriptor fixtures.
- The optional pinned Cobalt staging test passed: 75 tested imports resolved,
  167 strong imports remain, and normal plus 64 MiB-OOM staged pages reclaim.
- The full `tests/test_system.py` regression passed and restored the normal
  framebuffer image.
- The focused audio regression also passed after making boot waits tolerant of
  loaded hosts; VM teardown now escalates from terminate to kill if necessary.

## Known gaps and next scoped chat
1. Continue Phase 33.1/33.2 only: implement the remaining loader TLS/finalizer,
   file/storage/system, socket/network and Starboard platform contracts with
   semantic tests; do not count exported symbol totals as completion.
2. Reuse the descriptor lifetime/cancellation model for sockets and extend its
   contention coverage as blocking network operations arrive.
3. Build the real Evergreen launcher, event loop, graphics/input bridge and run
   native Cobalt HTML/JavaScript/DOM/input plus HTTPS acceptance in MyOS.
- Renderer/backend, media/player bindings and that native acceptance remain
  open. Do not start Phase 33.3 before both parent gates genuinely pass.

## Read only what the task needs
- Rules: AGENTS.md.
- Planned gates: docs/PHASE33_EXECUTION_PLAN.md.
- Runtime audit: user/platform-runtime.c, user/elf-platform-pthread.c,
  tests/test_elf_dynamic.py, docs/PHASE331_PTHREAD.md and
  docs/PHASE331_WAIT.md.
- Full regression: tests/test_system.py; evidence in build/activity.log and
  build/system-tests/. Never inspect activity.log through run_logged.py because
  that appends the output back into the same file; read only bounded excerpts.
- Import contracts: docs/PHASE331_ABI.md, docs/PHASE331_IMPORTS.csv.
- Prior context only if needed: docs/HANDOFF.md.

## Completion rule
- Do not close Phase 33.1 or 33.2 until Cobalt itself opens HTML, executes
  JavaScript/DOM, receives real keyboard/mouse input, and uses HTTPS in MyOS for
  the documented endurance gate. Phase 33.3 has not started.
- The owner pushes manually. This slice ends in a local commit only.
