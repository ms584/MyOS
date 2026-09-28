# Phase 32 — YouTube playback (in progress; acceptance open)

**Actual YouTube playback in MyOS does not work yet.** Do not label this phase
complete. The goal remains: open YouTube in the MyOS browser, search, select
and play an online video with audible sound, pause/resume and seek in QEMU.

## Verified gate, 2026-09-27

A live QEMU probe opened `https://www.youtube.com/` using MyOS networking and
certificate-verified TLS. The site currently chains through Google Trust
Services WR2 to GTS Root R1. The SHA-256 fingerprint of the embedded
GTS Root R1 certificate is
`d947432abde7b7fa90fc2e6b59101b1280e0e1c7e4e40fa3c6887fff57a7f4cf`.
Google's [published CP/CPS](https://pki.goog/repo/cp-cps-tls/6.2/GTS-CP-CPS-TLS.html)
lists that fingerprint. The root is loaded only for the exact
`www.youtube.com` hostname; certificate and hostname verification remain
mandatory. MyOS's Mbed TLS build now enables SHA-384, which is needed to parse
this root. Offline wrong-host, untrusted and expired certificate checks still
pass.

The verified HTTPS connection now reaches the response, then the Browser
reports `HTTPS page exceeds 64 KiB`. The live page was about 894 KiB without
compression on this test date; a watch page was about 1.6 MiB from the host.
These sizes are observations, not stable contracts. MyOS's browser currently
collects at most 64 KiB and only renders a small HTML-to-text/link subset.
It has no JavaScript engine, DOM, iframe, cookies, HTML5 media or YouTube
video/audio decoders. Phase 31's MYV1 player plays local synthetic clips and
does not decode YouTube formats. MyOS's kernel image limit is 256 KiB and its
heap is 4 MiB, so a modern browser engine cannot simply be linked into the
existing build.

YouTube's [IFrame Player API](https://developers.google.com/youtube/iframe_api_reference)
documents JavaScript and HTML5 postMessage requirements for embedded
playback. The [YouTube system requirements](https://support.google.com/youtube/answer/78358)
call for a current supported browser. A page title or thumbnail is not video
playback. A host browser window is not playback inside MyOS.

## Tests

```bash
cd ~/myos
python3 tools/run_logged.py -- python3 tests/test_https.py
python3 tools/run_logged.py -- python3 tests/test_browser_https.py
python3 tools/run_logged.py -- python3 tests/test_system.py
python3 tools/run_logged.py -- python3 tests/test_youtube_probe.py
python3 tools/run_logged.py -- python3 tests/test_phase32_stream.py
python3 tools/run_logged.py -- python3 tests/test_youtube_stream.py
python3 tools/run_logged.py -- python3 tests/test_rickroll_probe.py
```

The YouTube probe is live and optional: external site behavior can change.
It passes only when verified YouTube HTTPS reaches the known 64 KiB Browser
gate. The streaming test uses a local QEMU-accessible TLS server and checks
response length and checksum. The second live probe streams the YouTube
homepage using native MyOS TLS; on 2026-09-27 it received 902,111 bytes
with FNV64 `D42844EF8FCB3D14`. Live results can change. None of these
tests is Phase 32 playback acceptance. On 2026-09-28, a separate native
QEMU probe fetched the exact Rickroll watch URL
`https://www.youtube.com/watch?v=dQw4w9WgXcQ` and received 1,344,776
bytes over certificate-verified HTTPS. The page was not rendered or played.

## Native-only scope and transport progress

The owner requires every playback component to run inside MyOS. WSL is only
the development/build host and must not fetch, decode, transcode or play
YouTube media on behalf of MyOS. There is no host-media bridge fallback.

MyOS now has a bounded, callback-based TLS receive path independent of the
Browser's 64 KiB page buffer. `net https-count HOST PORT PATH` streams and
counts up to 8 MiB of a verified HTTPS response with a diagnostic FNV64
checksum. A QEMU test transferred a 256 KiB binary response and verified
the exact count and checksum. This command is a transport diagnostic, not
a media downloader or YouTube player. The Browser itself still has its
64 KiB response and 160-line limits.
Tor changes the network route but does not supply the missing web/media
runtime; Tor Browser itself is Firefox-based. Search can locate a video
link, but cannot decode it in the existing MyOS Browser.

Google's [Cobalt Starboard porting guide](https://cobalt.googlesource.com/cobalt/+/refs/heads/24.lts.stable/cobalt/site/docs/starboard/porting.md)
describes a native YouTube-capable browser runtime, but MyOS lacks its
required OS, graphics and media platform implementation. This is an
architecture candidate, not a completed or selected port. The official
[IFrame Player API](https://developers.google.com/youtube/iframe_api_reference)
requires JavaScript and HTML5 postMessage, also absent from MyOS.

## Remaining acceptance work

1. Port or implement a compatible in-MyOS browser/player runtime with the
   official YouTube player interface; document dependencies and supported
   sites. Do not substitute a host-side player.
2. Support responses larger than 64 KiB and the required page/runtime APIs.
3. Add actual online media decoding and buffered network delivery to the
   MyOS framebuffer and audio device, with pause/resume and seeking.
4. Run an end-to-end QEMU test that searches YouTube, selects a real video,
   observes moving video pixels and audible sound, then exercises controls.
   Record the date, video URL, resolution, evidence and limitations.

Do not complete this phase merely because HTTPS or local MYV1 playback works.
