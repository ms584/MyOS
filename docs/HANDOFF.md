# MyOS handoff — 2026-09-29

Current entry point: [PROJECT_STATE.md](PROJECT_STATE.md). Read it first;
this document retains historical context and may describe earlier checkpoints.

The latest Phase 33.1 continuation adds tested anonymous user VM, extra Ring 3
threads, FS-base TLS and atomic wait/wake/timeout primitives. See
`docs/PHASE331_RUNTIME.md`. These custom kernel primitives now back a fixed-address Ring 3 platform runtime.
Sixty-two tested allocator/VM/errno/pthread imports resolve through an exact allowlist;
180 strong Cobalt imports remain. Native Cobalt HTML/JavaScript/input
smoke remains the required gate before Phase 33.3.

## Next-session plan

The owner requested detailed planning only after pausing implementation. Read
[PHASE33_EXECUTION_PLAN.md](PHASE33_EXECUTION_PLAN.md). Resolved imports are
not proof of full contract semantics; clock attributes and signal masks need
audit before further API expansion. No tests were run for this docs-only plan.

## Phase 33.1/33.2 pthread checkpoint - 2026-09-29

Local work after commit 290f1dc adds a 4,960-byte Ring 3 platform runtime
covering all 48 pthread/semaphore imports required by the pinned Cobalt image.
Together with allocator and POSIX VM/errno support, 62 of 242 strong imports now
resolve and 180 remain. The acceptance fixture proves joinable and detached
thread lifecycles, detached slot/stack reuse, mutex/condition/semaphore/rwlock/
once behavior, timed waits, thread-specific keys with destructors, names, stack
attributes and scheduler compatibility. Detached kernel stacks and user mappings
are reclaimed. The exact implementation and limits are in
docs/PHASE331_PTHREAD.md.

python3 tests/test_elf_dynamic.py passed after the final stack-attribute fix.
The optional pinned Cobalt staging test passed during this slice with 62 resolved
imports, 180 missing imports and full staged-page reclamation. The full
tests/test_system.py run was started and passed boot, shell, graphics, mouse,
desktop, Settings, Calculator, Snake, audio, video, browser, release integration
and the Phase 26 app cases shown in build/activity.log; it was stopped at the
owner's request before completion. Run the complete suite next. A normal
bash build.sh image was restored after stopping it.

Neither Phase 33.1 nor Phase 33.2 is closed. Continue these twelve gates in
order, preserving real acceptance tests rather than no-op symbol stubs:

1. Finish the exact ABI register and keep every implemented import fixture-tested.
2. Preserve allocator, anonymous VM, errno and process cleanup coverage.
3. Preserve pthread synchronization, thread lifecycle and TLS-key coverage.
4. Add ELF PT_TLS/TLS relocations, destructors, needed dependency/finalizer support.
5. Add cancellation and waitable descriptors: poll, epoll, eventfd and pipes.
6. Add required file, storage, clock and system contracts.
7. Add the remaining POSIX socket/network contracts on the existing NIC/TCP/TLS stack.
8. Implement the real Evergreen SABI launcher, SbEventHandle and event loop.
9. Implement Starboard window, EGL/GLES bridge and keyboard/mouse input delivery.
10. Implement Starboard audio, decode-target and media/player contracts.
11. Run native Cobalt HTML/JavaScript with real input for at least 60 seconds and prove cleanup.
12. After combined runtime/engine closure: 33.3 standard media, 33.4 actual YouTube, then 33.5 Rickroll acceptance.

Next session: finish the outstanding full regression, audit existing pthread
clock/mask/ABI behavior, then design shared descriptors and implement clock/wait
slices. See PHASE33_EXECUTION_PLAN.md for the detailed updated sequence. The contract CSV currently groups the still-unimplemented entries as
41 file, 23 media, 23 network, 23 platform, 16 optional, 13 system, 10 graphics,
8 wait, 5 audio, 5 storage, 4 constants, 4 signals, 2 clock, 2 event,
1 certification and 1 weak-TLS symbol. The owner always pushes manually.

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
- The owner keeps the Git push step. Check local commits and origin/main before
  starting the next phase; do not assume this handoff is already pushed.
  Earlier commits: 6689afb (Phase 30), ac43b74 (Phase 28), fa52adc (Phase 29).
- Phase 31 now adds a native MYV1 local video player (80x60, 10 fps palette/RLE,
  11025 Hz unsigned PCM) on the framebuffer and SB16 with PIT-based sync.
  See docs/PHASE31.md for the QEMU tests, demo disk and limitations.
- Phase 32 is in progress, not accepted: MyOS now verifies the live YouTube
  HTTPS connection with SHA-384 and a GTS Root R1 certificate scoped to the
  exact www.youtube.com hostname. The Browser reports its current 64 KiB
  page-size gate; it still lacks JavaScript and online media decoding.
  The owner requires native MyOS playback at every step; WSL may build and
  test but must not fetch/decode/play YouTube media for MyOS. Native TLS
  streaming past 64 KiB has a 256 KiB QEMU integrity test; the GUI still
  uses its 64 KiB/160-line bounded text renderer. A live QEMU native TLS
  probe streamed 902,111 bytes from www.youtube.com on 2026-09-27, but
  no YouTube audio/video playback has been implemented. A 2026-09-28 native
  QEMU probe also received 1,344,776 bytes from the Rickroll watch URL;
  this is transport evidence only. Tor/search do not supply the missing
  JavaScript, HTML5 and codecs.
  See docs/PHASE32.md. Next work is the full YouTube player acceptance.
- Phase 33 is in progress but not passed. A new `video net HOST PORT PATH`
  command fetches a <=64 KiB MYV1 clip over verified HTTPS inside MyOS,
  validates HTTP status/length and plays framebuffer motion plus SB16 audio.
  The focused QEMU test rejects bad length and wrong-host TLS. This is a
  whole-clip synthetic format test, not YouTube media. Left/Right seek
  works only inside the loaded MYV1 clip.
  Phase 33 still requires actual online Rickroll video plus sound and
  controls in MyOS/QEMU;
  it cannot pass on an HTTPS fetch, search result, static frame, local
  MYV1 clip, Tor route, or host playback. See docs/PHASE33.md.
- Remaining Phase 24 hardware preparation and Phase 25 physical-machine
  acceptance follow; Phase 25 is deferred because the owner has no USB device.

Phase 31 full tests/test_system.py passed, including 30 native video frames,
measured 435/655 Hz QEMU SB16 WAV output, pause/resume/stop and malformed
file recovery. BIOS and UEFI packaged images both passed video/audio smoke
tests. The sample data disk mounted and played at 32 MiB RAM; 16 MiB rejected
it gracefully. Two existing GUI screenshot tests were made deterministic by
waiting for their matching desktop frame before reading QEMU pixels.

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

## Work rules for Phases 32–33

Start from docs/PHASE31.md, docs/PHASE29.md, docs/PHASE32.md and
docs/PHASE33.md. Do not count local MYV1, a host-side player or static
frames as online YouTube playback. Preserve boot, GUI, shell, storage, network
and audio baselines. Run focused QEMU tests, the required
python3 tests/test_system.py for architectural changes, and bash build.sh to
restore the normal image after test variants. Document limits plainly.
The owner wants a local commit ready for their manual push.

## Engine assessment, 2026-09-28

Static comparison and the official Cobalt x64 ELF audit are recorded in
[PHASE33_ENGINE_EVALUATION.md](PHASE33_ENGINE_EVALUATION.md). Cobalt 27 LTS /
Starboard 18 is the conditional native-port target. The first Phase 33.1 slice
now has an exact import-contract register, read-only large assets, restricted
Ring 3 ELF execution and native Cobalt import inspection. See [PHASE331.md](PHASE331.md)
for tests and limits. Phase 33.1 remains incomplete: external platform bindings, the native
Starboard runtime and HTML/JavaScript smoke are still open. Native engine
startup and actual YouTube playback remain unproven.
Phase 32/33 remain **NOT PASSED**.

## Phase 33.2 — dynamic loader continuation

The next loader milestone is documented in [PHASE332.md](PHASE332.md): staged
ET_DYN mapping, four relocation types, in-image/weak resolution, RELRO and
Ring 3 constructor startup for self-contained fixtures. Actual Cobalt maps
and processes relocations, resolves 62 tested allocator/VM/errno/pthread imports, and stops on
180 remaining strong imports, so it is released without execution. Full native-engine/HTML/JavaScript support
and the original Phase 33.1 gate remain open; this does not close Phase 32/33.

Phase 33.2 loader baseline: focused tests and the full system regression passed
on 2026-09-28; normal framebuffer image restored. See PHASE332.md for the
restricted loader scope and the still-open native-engine gate.

## Runtime continuation — 2026-09-28

Per-task x87/MMX/SSE context support has been added; see
[implementation and test status](PHASE331_FP.md). This supersedes the earlier
FP/SIMD limitation only. The owner requires completion of 33.1 and 33.2
before starting 33.3. Cobalt startup remains unimplemented; neither parent
gate is being marked complete.
