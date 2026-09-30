# MyOS project state

Updated: 2026-09-30. Read this first; open detailed docs only as needed.
Repo: /home/xytron404/myos (WSL Ubuntu).
Prior baseline: c8c9c96, branch main; socketpair is the current local checkpoint.
After git fetch origin, origin/main has no new changes; commits remain local.
Check current Git status/HEAD each new chat.

## Scope and status
- Phase 33.1/33.2 remain OPEN: Cobalt has not executed in MyOS.
- 139/242 strong imports have fixture coverage; 103 remain plus a weak TLS symbol.
  Symbol counts and fixture passes do not prove full ABI compatibility.
- Phase 32 and Phase 33 are NOT PASSED; actual YouTube playback is unproven.
- Sequence: finish 33.1/33.2 -> 33.3 standard media -> 33.4 YouTube ->
  33.5 Rickroll. Check Phase 32 search/select/play separately.
- The owner pushes manually. Do not push.

## Current implementation slice
- Baselines 54ef570/4be9c3b cover pthread clocks/masks/lifecycle, shared
  descriptors/waits and tested system ABI. See PHASE331_WAIT.md and
  PHASE331_SYSTEM.md; the independent 256 KiB kernel-file cap remains.
- File descriptors now use refcounted objects with shared offsets/status flags,
  exclusive create, positional/vectored I/O, metadata, seek and truncate.
- /assets/NAME exposes large read-only random reads. Directories, temporary
  files, realpath and unlinkat have native fixtures. fsync/fdatasync commit
  the real ATA snapshot; volatile RAMFS reports ENOTSUP.
- Detailed supported subsets and remaining semantic gaps: PHASE331_FILES.md.
- AF_UNIX stream socketpair shares the descriptor lifetime/wait model, with
  duplex rings, duplicate status, shutdown, poll/epoll/FIONREAD and cancellation.
  Native tests pass; network TCP sockets remain absent. See PHASE331_SOCKETPAIR.md.
- Command logging is bounded: 16 MiB per log, two rotations (48 MiB total),
  feedback rejection; logger regression passes. Watch with unwrapped tail -F.

## Validation at checkpoint
- Focused dynamic fixtures passed, including 128 KiB asset random reads and
  8 KiB binary fsync/fdatasync persistence across reboot on a disposable v2 disk.
- Pinned Cobalt binary/header hashes and the 243-row contract register pass.
- Prior file full regression passed at 19:02 +07; normal image restored.
- Socketpair full regression passed at 19:38 +07; normal image restored.
- Pinned Cobalt staging passed at 256 MiB (139 bound, 103 missing), with all
  staged pages reclaimed; 64 MiB OOM rollback passed. This does not run Cobalt.

## Known gaps and next dependency
1. Continue Phase 33.1/33.2 only: implement the remaining loader TLS/finalizer,
   file/storage/system, socket/network and Starboard platform contracts with
   semantic tests; do not count exported symbol totals as completion.
2. Replace legacy single-stream TCP with independent cancellable connections;
   test concurrent requests, partial I/O, readiness, resolver/options and TLS.
3. Build the real Evergreen launcher, event loop, graphics/input bridge and run
   native Cobalt HTML/JavaScript/DOM/input plus HTTPS acceptance in MyOS.
- Renderer/backend, media/player bindings and that native acceptance remain
  open. Do not start Phase 33.3 before both parent gates genuinely pass.

## Read only what the task needs
- Rules/gates: AGENTS.md and docs/PHASE33_EXECUTION_PLAN.md.
- Runtime audit: user/platform-runtime.c, user/elf-platform-pthread.c,
  tests/test_elf_dynamic.py, docs/PHASE331_PTHREAD.md and
  docs/PHASE331_WAIT.md.
- Full regression: tests/test_system.py; evidence in build/activity.log and
  build/system-tests/. Never inspect activity.log through run_logged.py because
  that appends the output back into the same file; read only bounded excerpts.
- Import contracts: docs/PHASE331_ABI.md, docs/PHASE331_IMPORTS.csv.

## Completion rule
- Do not close Phase 33.1 or 33.2 until Cobalt itself opens HTML, executes
  JavaScript/DOM, receives real keyboard/mouse input, and uses HTTPS in MyOS for
  the documented endurance gate. Phase 33.3 has not started.
