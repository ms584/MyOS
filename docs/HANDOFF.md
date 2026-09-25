# MyOS handoff — 2026-09-26

Read root AGENTS.md and docs/ROADMAP.md before starting. The owner asked to
continue the approved phase sequence and **always pushes to GitHub personally**.
Make local commits when work is verified; never run git push. Log development
commands with python3 tools/run_logged.py -- ... so the owner can watch
tail -f build/activity.log in VS Code.

## Verified baseline

- Repo: /home/xytron404/myos in WSL Ubuntu, branch main.
- Phases 28, 29 and 30 have documented **QEMU baselines**. Phase 28 has native
  certificate-verified TLS 1.2; Phase 29 has a bounded kernel-mode text
  browser with real DuckDuckGo Lite HTTPS search, result links and cached Back;
  Phase 30 has QEMU SB16 PCM output and WAV capture.
- Phase 29 is not a modern CSS/JavaScript browser. YouTube playback has **not**
  been implemented. Physical hardware support has not been proven.
- The branch was synchronized with origin/main before this note was added.
  Relevant commits: 6689afb (Phase 30), ac43b74 (Phase 28),
  fa52adc (Phase 29).
- Next planned work: **Phase 31 video decoding, buffered playback, rendering
  and audio/video synchronization at a documented format and resolution**.
  Phase 32 separately requires actual online YouTube playback with sound and
  transport controls in MyOS. Remaining Phase 24 hardware preparation and
  Phase 25 physical-machine acceptance follow; Phase 25 is deferred because
  the owner has no USB device.

## Recent validation and unresolved observation

Phase 29 passed local and live QEMU browser tests, TLS and network suites,
the full tests/test_system.py regression, plus packaged BIOS/UEFI browser
tests. The production framebuffer image was restored afterward. Phase 30's
automated tests/test_audio.py passed previously; see docs/PHASE30.md.

The owner reported that manually entering audio tone 440 500 in QEMU appeared
to hang without a new myos> prompt. An automated QEMU probe using the same
-cpu max, RTL8139 NIC and WAV audio backend returned Audio tone queued
promptly, so the manual UI symptom is **not yet reproduced or fixed**.
The owner's finalized build/phase30-demo.wav was a valid 44100 Hz stereo
16-bit WAV, about 456.7 seconds long, with audible signal in its first
second. A short build/phase30-tone-clip.wav was created for playback.
Both are ignored build artifacts. The owner also saw a transient VS Code/WSL
E_UNEXPECTED / WebSocket 1006 connection failure, later reported normal.
Do not infer an OS bug or a WSL root cause from those screenshots alone.

If the manual audio hang recurs, reproduce with a QEMU serial log and check
whether Audio tone queued is emitted before changing the SB16 driver. The
shell command should queue the 500 ms tone and return promptly; QEMU itself
stays in the foreground until its window is closed or Ctrl+C is sent to the
host terminal. Avoid opening the WAV capture in VS Code while QEMU writes it.

## Work rules for Phase 31

Start from docs/PHASE30.md, docs/PHASE29.md, and the Phase 31/32 rows in
docs/ROADMAP.md. State the actual codec, container, test-media source, memory
budget and achieved A/V timing; do not count a host-side player or static
frames as native MyOS playback. Preserve boot, GUI, shell, storage, network
and audio baselines. Run focused QEMU tests, the required
python3 tests/test_system.py for architectural changes, and bash build.sh to
restore the normal image after test variants. Document limits plainly.
The owner wants a local commit ready for their manual push.
