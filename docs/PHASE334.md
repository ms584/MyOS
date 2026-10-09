# Phase33.4 actual YouTube integration
Status: IN PROGRESS, NOT PASSED. Started 2026-10-09 +07.
Baseline main7f4336da8a590c2df68977cc07d50d36c1ccc31c. Local work only; no push.
Phase32/33 and Rickroll/33.5 remain NOT PASSED.

## Checkpoint1: prerequisites and public native feasibility
Read AGENTS.md, PROJECT_STATE.md, HANDOFF.md, PHASE33_EXECUTION_PLAN.md,
PHASE33.md, native-web acceptance, standard-media acceptance and reproduction.
Tracked baseline clean; owner untracked promotion logs/status/audits preserved.
No other QEMU/test_system jobs before observation.
Five current normal artifacts exactly match
wip/phase333/checkpoint6/final-normal-identity.json.
Full system regression7 terminal0 at2026-10-09 16:52:44, normal image restored;
final normal UEFI/focused results are recorded in PHASE333_ACCEPTANCE.md.
These valid unchanged-source results are reused, not relabelled as a new run.

Recorded native33.1/33.2 PNG/JSON/serial replay PASS20:41:12:
build/native-public-review-FJqyMz. This is recorded evidence validation,
not a fresh engine session. Actual HTTPS/JS/DOM/input/concurrency/cert negatives
and two closes were verified by the committed verifier.
Recorded33.3 original continuous session analyzer PASS20:41:56:
build/native-verified-tls-yzub9bzc;600decoded/no drops,
37.4s actual visible frame-ID span,14.9477fps,same-session40.062s PCM,
max17.890ms sync,total media RAM bound27103200B.
No missing prerequisite identified in these evidence checks.
Current pins: Cobalt27.lts.3/Evergreen7.3.2/Starboard18;
WebM VP8/Opus native player; other codecs/resolutions remain unclaimed.

## Feasibility method
tests/test_cobalt_youtube_observation.py packages only hash-verified upstream
core/resources/public trust store/fonts and accepted media provider.
It freezes the accepted normal image/runtime, boots MyOS512MiB/KVM/CPUhost/
RTL8139/SB16, and launches the actual native engine with HTTPS YouTube URLs.
No disposable CA added, host HTML/media fetch, browser/player, downloader,
decoder/transcoder, signature spoof or media proxy.
The host observes QEMU screen and actual native11025monoU8 SB16 output.
Each close must return shell and execute mem before another launch.
The evidence JSON stays phase_passed=false: observation exit0 cannot pass33.4.
Store original URL,date,commit,all image/core/provider hashes,QEMU arguments,
frames/audio/serial and cleanup observations. Normal image hashes checked after.
Do not infer successful HTTP/TLS/JS/media merely from engine startup.

Initial run20:40:02: build/phase334-public-35o8vqj2:
home https://www.youtube.com/,
watch https://www.youtube.com/watch?v=jNQXAC9IVRw,
TV https://www.youtube.com/tv. Results pending; no playback claim.
This public short video is a feasibility URL, not the final33.5 Rickroll gate.

## Authentication investigation
Official [device authentication documentation](https://developers.google.com/youtube/cobalt/docs/gen/cobalt/doc/device_authentication)
reviewed2026-10-09. Cobalt's initial request can carry a certification-scope/time
message and HMAC signature using a platform certification secret; absent signing
support it proceeds without the signature. This documents a potential deployment
constraint, not proof that the observed public endpoint rejects MyOS.
user/platform-system.c returns false for signing and ENOTSUP for unavailable
certification properties; user/elf-system-services.c verifies no forged outputs.
No legitimate certification credentials are present or invented.
Prove network/frontend failures before evaluating the WPE fallback with owner.

## Remaining gates
Real YouTube moving video+sound>=30s; true pause/resume/seek;
player close/reopen with shell/desktop responsive; search/select roadmap path;
real media-host HTTPS/certificates/redirects/ranges/concurrency;
measured stream/container/codec/resolution/fps/buffers/RAM;
timeout/loss/expired-URL/unsupported-format recovery;
focused checks and applicable full regression; normal image restored.
No33.5 or parent phase closure until its separate acceptance.


## Checkpoint2: measured codec gap and native VP9 candidate
See [original evidence](wip/phase334/checkpoint2/README.md).
Public home/watch/TV original observation completed exit0 at20:49:08 with exact
PMM/SHM/aliases after every close, shell mem, normal artifacts unchanged.
Audio capture contains zero frames. Home/TV welcome were real guest pixels;
watch error Your browser cannot play this video does not pass.
Diagnostic desktop query records explicitly request VP9/H264/AV1, no VP8;
Opus support2, video support0. This proves a codec-negotiation gap, not all
causes of failure or a certificate/authentication rejection.
TV guest input reaches home; attempted TV fragment does not play a video.
First fragment injection failed in QMP helper (missing # Shift+3), preserved.

Separate --vp9 candidate target build/configuration added from existing pins.
Upstream decoder640x360 size guard,8bit only, no host decoder.
Provider retains160x90 native presentation after guest-only downscale.
Declared candidate goal: VP9 profile0+Opus, at most640x360/30fps decode,
160x90 presentation, sync<=100ms and media RAM<=32MiB; actual higher-size/rate
limits remain unverified until online/dedicated tests. No capability promotion.
Native underlay40s160x90@15 fixture passes controls/EOS/restart/resources and
actual PNG/PCM focused gate,9.987ms maximum sync,8.71MB sampled player increment.
Two texture-mode trials FAILED113.734/111.163ms; latter records70ms render period.
Keep these failures distinct from the supported Cobalt native-underlay path.
Default production provider remains byte-identical41901a13, normal images unchanged.
New diagnostic VP9 online provider-z73ft99k built21:09:33; online test pending.
