# Phase 33 — Rickroll playback acceptance (in progress; not passed)

**Status: NOT PASSED.** The owner added this hard gate on 2026-09-28. Do not
close Phase 33 until a person can watch the real online Rickroll video with
sound inside MyOS running in QEMU. Phase 32's broader YouTube work remains
open and is a prerequisite, not evidence that this gate has passed.

## Current checkpoint — 2026-10-09

Native33.1/33.2 and standard-media33.3 prerequisites were verified before the
owner-authorized33.4 work. Cobalt now runs HTTPS, JavaScript/DOM/input and native
media in MyOS. Real public YouTube BBB playback has moving video and same-session
SB16 audio; direct native pause/resume/seek and close/reopen have been exercised.
This is partial33.4 progress, not Rickroll acceptance. Latest1GiB candidate has
sync outliers beyond100ms and fails playback recovery after actual NIC loss.
Search/select, expiry/error handling, bounded repeated reopen and final accepted
profile still need proof. See PHASE334.md, PROJECT_STATE.md and checkpoint4.
The PMM1024 architectural full regression passed2026-10-10 00:15:27;
normal BIOS/UEFI and all5 hashes were restored and verified00:17:06.
Do not close33.4,33.5 or this parent gate from partial online playback.

## Required demonstration

1. Boot the normal MyOS build in QEMU, open the MyOS browser, and reach
   `https://www.youtube.com/watch?v=dQw4w9WgXcQ` by typing the URL or using
   MyOS search and selecting the result.
2. Play that online video in the MyOS display. Show moving frames for at
   least 30 seconds and audible music from the same playback session.
3. Pause: video and audio must stop. Resume: both must continue. Seek to a
   different time: playback must continue from the new position. Closing the
   player must leave MyOS responsive.
4. Record the test date, video URL, MyOS build/commit, QEMU command, display
   resolution, serial/network evidence, moving-frame evidence, captured audio,
   and the observed control results. Run the relevant automated QEMU regression
   suite and have the video/audio/control behavior checked interactively.

The network connection, certificate verification, fetched HTML, a search
result, title, thumbnail, static screenshot, synthetic frames, preloaded
MYV1 file, host browser/player, WSL decoder or Tor route alone **never
passes**. The source must be the online YouTube video and playback must
occur in MyOS. Do not use a Linux desktop as a substitute for MyOS.

## Verified online-video building block, 2026-09-28

The `video net HOST PORT PATH` shell command downloads one bounded MYV1 clip
over certificate-verified HTTPS directly inside MyOS, checks HTTP 200 and the
declared response length, then plays moving frames on the MyOS framebuffer
with PCM sound through SB16. Space pause/resume, Left/Right one-second seek, and Esc stop
controls work on this fetched clip. QEMU tests verify changed framebuffer
pixels and captured audio, and reject a wrong-host certificate or malformed
response length. The test server and synthetic clip are development fixtures;
the network fetch, parse, decode and playback execute in MyOS.

This is a **whole-clip download capped at 64 KiB**, not buffered continuous
media streaming. It supports only the custom MYV1 format; it does not parse
the YouTube watch page, select a YouTube stream, decode MP4/WebM, or seek
within YouTube media. MYV1 seek works only within the loaded clip.
It does not satisfy the Rickroll gate.

Run the focused QEMU test with:

```bash
cd ~/myos
python3 tools/run_logged.py -- python3 tests/test_phase33_online_video.py
```

For a MYV1 clip intentionally hosted on a trusted HTTPS server, boot MyOS
with its RTL8139 NIC and SB16 audio device, run `net up`, then type
`video net HOST PORT /clip.myv`. The TLS certificate must validate for HOST.
This command cannot play a YouTube URL.

## Historical initial gate — 2026-09-28

MyOS fetched 1,344,776 bytes from the watch URL over verified HTTPS in a
QEMU probe on 2026-09-28. This proves transport only. The current Browser
has no JavaScript/DOM/iframe/media runtime and its page buffer is 64 KiB;
the MYV1 player does not decode YouTube media. The new online MYV1
command changes neither of those limits. Tor or web search cannot
replace those missing capabilities.

The implementation must provide a native MyOS web/player runtime, online
media delivery, video decoding, audio output and playback controls. See
[PHASE32.md](PHASE32.md) for the measured limits and native-only scope.
Until the full demonstration above passes, leave this document and the
roadmap marked **not passed**.

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
and processes relocations; 62 strong imports resolve and 180 remain unresolved, so
it is released without execution. Full native-engine/HTML/JavaScript support
and the original Phase 33.1 gate remain open; this does not close Phase 32/33.

Phase 33.2 loader baseline: focused tests and the full system regression passed
on 2026-09-28; normal framebuffer image restored. See PHASE332.md for the
restricted loader scope and the still-open native-engine gate.

## Detailed Phase 33.1-33.5 plan

See [PHASE33_EXECUTION_PLAN.md](PHASE33_EXECUTION_PLAN.md) for the next-session
sequence, runtime/engine gates, standard-media stage (33.3), YouTube integration
(33.4), and final Rickroll acceptance (33.5). This is planning, not completion.

Checkpoint4 completion2026-10-10 00:17 +07: new PMM1024 full default
system suite passed00:15:27; normalUEFI/final launcher/pair BIOS+UEFI
passed, all5 normal identities exact, runner exit0. Evidence:
wip/phase334/checkpoint4/regression. Earlier running/pending statements
above are historical. YouTube performance/recovery acceptance still open.
