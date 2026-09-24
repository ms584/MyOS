# MyOS project working agreement

- Repository: `/home/xytron404/myos` in WSL Ubuntu.
- The owner always pushes to GitHub manually. Do not run `git push`.
- Use actual roadmap coverage in commit names, e.g. `Phase 4+8: ...`.
- The revised roadmap has phases 0–32. Phases 0–13 retain their history;
  Phases 14–20 graphics, windowing, Settings, Calculator, Snake and Desktop 1.0
  QEMU baselines are complete; Phase 21 is next, and Phase 32 targets
  actual YouTube playback. Refer to `docs/ROADMAP.md` for scope and gates.
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
