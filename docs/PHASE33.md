# Phase 33 — Rickroll playback acceptance (in progress; not passed)

**Status: NOT PASSED.** The owner added this hard gate on 2026-09-28. Do not
close Phase 33 until a person can watch the real online Rickroll video with
sound inside MyOS running in QEMU. Phase 32's broader YouTube work remains
open and is a prerequisite, not evidence that this gate has passed.

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

## Current gate

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
