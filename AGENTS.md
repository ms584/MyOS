# MyOS project working agreement

- Repository: `/home/xytron404/myos` in WSL Ubuntu.
- Read docs/HANDOFF.md for the current phase status and next task.
- The owner always pushes to GitHub manually. Do not run `git push`.
- Use actual roadmap coverage in commit names, e.g. `Phase 4+8: ...`.
- The revised roadmap has phases 0–32. Phases 0–13 retain their history;
  Phases 14–23 through Desktop 1.0, BIOS/UEFI and release tooling are complete
  QEMU baselines. Phase 24 has a validated virtual USB image; target-machine
  driver preparation is pending. Phase 25 is deferred by the owner (no USB).
  Approved execution order (2026-09-25): Phases 26–27 complete in QEMU, then 28–32,
  remaining Phase 24 hardware preparation, and Phase 25 acceptance. Preserve
  phase numbering; keep device drivers separate from higher-level services.
  Phases 26–27 have QEMU app-runtime/network baselines;
  Phase 30 has a tested QEMU SB16 PCM audio baseline.
  Phase 28 has native verified HTTPS; Phase 29 has a QEMU kernel-resident
  HTTPS text-browser/search baseline using an in-tree bounded renderer. Modern
  CSS/JavaScript browser behavior and YouTube remain open for later phases.
  Phase 32 targets actual YouTube playback in QEMU before physical testing.
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
