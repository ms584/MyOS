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

