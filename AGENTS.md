# MyOS project working agreement

- Repository: `/home/xytron404/myos` in WSL Ubuntu.
- The owner always pushes to GitHub manually. Do not run `git push`.
- Use actual roadmap coverage in commit names, e.g. `Phase 4+8: ...`.
- The roadmap has phases 0–18. Refer to `docs/ROADMAP.md`; do not invent a
  conflicting phase numbering scheme.
- Build with `bash build.sh`; run `python3 tests/test_system.py` for architectural
  changes. Restore the normal image after fault-test variants.
- Do not claim real-hardware, USB, user-mode or security isolation support
  based solely on successful QEMU kernel tests.
