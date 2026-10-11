# MyOS project working agreement

- Repository: `/home/xytron404/myos` in WSL Ubuntu.
- Read docs/PROJECT_STATE.md first for current status and next task.
  Consult docs/HANDOFF.md only when additional background is needed.
- The owner always pushes to GitHub manually. Do not run `git push`.
- Use actual roadmap coverage in commit names, e.g. `Phase 4+8: ...`.
- The revised roadmap has phases 0–33. Phases 0–13 retain their history;
  Phases 14–23 through Desktop 1.0, BIOS/UEFI and release tooling are complete
  QEMU baselines. Phase 24 has a validated virtual USB image; target-machine
  driver preparation is pending. Phase 25 is deferred by the owner (no USB).
  Approved execution order: Phases 26–27 complete in QEMU, then 28–32,
  Phase 33 Rickroll playback acceptance (added 2026-09-28), remaining
  Phase 24 hardware preparation, and Phase 25 acceptance. Preserve
  phase numbering; keep device drivers separate from higher-level services.
  Phases 26–27 have QEMU app-runtime/network baselines;
  Phase 30 has a tested QEMU SB16 PCM audio baseline; Phase 31 has a
  tested local MYV1 video/PCM playback baseline in QEMU.
  Phase 28 has native verified HTTPS; Phase 29 has a QEMU kernel-resident
  HTTPS text-browser/search baseline using an in-tree bounded renderer. Modern
  CSS/JavaScript browser behavior and YouTube remain open for later phases.
  Phase 32 targets actual YouTube playback in QEMU before physical testing;
  the current HTTPS YouTube page-size probe is only partial progress.
  Phase 33 is a strict end-to-end Rickroll gate: no visible online video
  and audible audio in MyOS means no pass. See docs/PHASE33.md.
  Refer to docs/ROADMAP.md for scope and gates.
  Do not treat planned capabilities as implemented.
- Build with `bash build.sh`; run `python3 tests/test_system.py` for architectural
  changes. Restore the normal image after fault-test variants.
- Do not claim real-hardware, USB, user-mode or security isolation support
  based solely on successful QEMU kernel tests.

- Run development commands through `python3 tools/run_logged.py -- ...` so the
  owner can watch `tail -f build/activity.log` in VS Code. Do not log secrets.
  Review `docs/PHASE11.md` through `docs/PHASE13.md` before extending file
  operations or persistence. GUI apps must state whether they run in kernel
  mode or isolated user processes.

## Token-efficient development mode

- One major phase or explicitly scoped subphase per chat. Finish at a tested
  checkpoint; do not automatically start the next phase or create new chats.
- Use the current working tree, Git and test evidence as source of truth.
  Read docs/PROJECT_STATE.md once at chat start, then check Git status/HEAD.
  Resolve stale documentation against code and evidence; do not assume PASS.
- Read only task-related files and dependencies. Use targeted searches/line
  ranges; expand scope only for a concrete reason. No whole-repository scan
  or rereading unchanged files without a reason.
- Make the smallest correct diff. No unrelated refactoring or extra features.
- Do not re-audit completed phases without an impact or evidence reason.
  Still run affected regression and mandatory architectural checks above.
- Choose necessary builds/tests from the change impact. Reuse valid results
  for unchanged code/configuration. After required checks and acceptance pass,
  stop rechecking unless changes, failures or unresolved concerns justify it.
- Diagnose errors from evidence before changing code; avoid speculative fixes.
- Save full logs to files. Read summaries and relevant error excerpts, not
  entire successful build logs. Do not paste full files unless necessary.
- Do not explain unrequested basics. Keep progress concise; final response:
  changed / test result / next step, including material limits or pending tests.
- Update docs/PROJECT_STATE.md at each checkpoint and before pausing/ending,
  even if the phase is incomplete. Keep it short (about 60 lines or fewer):
  baseline commit/branch, scope, evidence, blockers, relevant files, next step.
  Link detailed docs/logs rather than copying history. HANDOFF.md is background;
  PROJECT_STATE.md is the current entry point. Never mark pending tests passed.

## Resource budget — owner request 2026-10-11

- Run one QEMU/media/regression job at a time. Use each test's validated profile (normal online 1024 MiB; existing focused fixtures may use 512 MiB); do not reduce RAM to conceal a failure.
- For new host compilation, start with at most two workers and reuse incremental objects. Keep this cap unless the owner changes the resource budget.
- Prefer low-priority diagnostic execution; record native screenshots at 1 FPS unless a specific failure requires more. Keep one authoritative PNG/WAV/timeline bundle; do not generate redundant AVI copies.
- Reuse valid unchanged tests. Diagnose a concrete failure with one focused run before repeating long acceptance sessions; required affected and architectural regression still applies.
- Before adding large outputs, inventory their size and free space. Reclaim only verified generated/rebuildable files; preserve source, pinned dependencies, normal/manual images and original acceptance/failure evidence. Record exact paths and bytes reclaimed.
- Do not delete the WSL VHDX. Guest free space and Windows reclaimed space are different measurements; offline compaction must wait until all jobs finish and must be reported separately.
