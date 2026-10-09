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


## Checkpoint3 - full VP9 codec and thread pressure
See wip/phase334/checkpoint3 and PHASE334_EXPERIMENT_GUIDE.md.
Opt-in cobalt-trace native stderr/VLOG launcher focused BIOS/UEFI checks pass.
Full SDR VP9 codec strings from actual YouTube were rejected by the short parser;
optional full suffix support now passes native capability/40s fixture tests.
Online240s selects640x360@30, native create resource failure with pthread EAGAIN,
zero PCM. No online pass. Candidate40 workers fits existing runtime TLS boundary;
focused lifecycle/TLS PASS21:44:05; new full regression pending. Normal image hashes changed
for launcher/thread work. Default provider still byte-identical41901a13.
Broad guest RAM diagnostic was rejected by auto-review, never executed, removed.
Scoped native logs replace it; redact signed/tracking URL query values for commit.


2026-10-09 21:59 +07 update:40 workers fixes native create. Actual BBB YouTube
VP9 640x360@30 decoded/rendered in MyOS, PNG motion and native same-session
11025Hz monoU8 WAV2021508frames/183.357s. Pause counters fixed, resume advances,
seek changes ticket/position; Escape shell/resources exact. NOT PASSED:
post-resume sync outlier623121us exceeds100ms candidate gate.
All raw/PNG/PCM preserved in online640-sync-failed; no fake success.
Small320x240 bound focused PASS native-player-qjz8d23k. Public frontend omits
dimensions after width640/height360 probe rejection, selects unsupported output;
SbPlayerBridge invalid mode, zero PCM. online-small-bound-frontend-failed.
Next quality selection through real native PS/2 UI, no payload/capability bypass.
Full default regression started21:58:23, running; normal BIOS restore + UEFI and
final launcher checks queued. Phase33.4/33.5/Rickroll/parent remain NOT PASSED.

## Checkpoint4 diagnostic candidate — 2026-10-09 23:29 +07
Still NOT PASSED. See PROJECT_STATE.md / checkpoint4. Original40-worker
full suite passed22:39:10 and normal5 hashes restored; PMM1024 candidate
requires a new full suite. Native E8201024 focused low/high BIOS/UEFI
passed. Quiet512 quality144p trial failed with V8 allocation failure/int3
after only3s metrics; stored failed evidence, no30s pass. Quiet1GiB real
YouTube moves video with same-session PCM, closes/reopens, and buffers
through native NIC loss until120s media exhaustion. Automatic recovery
not yet verified.360p and144p sync outliers still violate100ms candidate
budget. Scoped decode/presentation/wakeup timing instrumentation is
diagnostic only; no performance policy change or broad RAM capture.

Checkpoint4 completion2026-10-10 00:17 +07: new PMM1024 full default
system suite passed00:15:27; normalUEFI/final launcher/pair BIOS+UEFI
passed, all5 normal identities exact, runner exit0. Evidence:
wip/phase334/checkpoint4/regression. Earlier running/pending statements
above are historical. YouTube performance/recovery acceptance still open.


## 2026-10-10 01:03 +07 — Phase 33.4 checkpoint 6 (partial, NOT accepted)

The actual VM-area limit of 1024 removed the observed reservation failures in public session
`build/phase334-public-4i4dsiw5` (zero reservation ENOMEM), without fabricating physical RAM.
Native YouTube UI selected 144p; Stats for nerds proves 256x144@30, VP9
`vp09.00.51.08.01.01.01.01.00` + Opus, SABR. Pause held both the consumed PCM
counter and rendered movie ROI; resume/seek were exercised. Reopening to Home, native typing,
searching and selecting the official Blender result reached real moving video again.
Both closes opened a desktop window through native mouse input and returned to shell;
free pages were exactly 257743 before/after each launch and desktop check. All five normal
image identities remained exact. Evidence: `docs/wip/phase334/checkpoint5/area1024-online-sync-failed/`.

This is still a negative acceptance: maximum native A/V error 560083 us overall;
the 144p player recorded 351299 us. Do not count HTML/search/thumbnail or fixture playback
as Phase 33.4 acceptance. The source snapshots/provider report identify the pre-batch provider.

A bounded VP9-underlay worker candidate now drains only existing input batches (audio <=8,
video <=4), retires due video while draining and keeps the existing four-frame/PCM limits.
VP8-underlay binary remains SHA256 41901a137421bf62e63585f8ea5339fd5b3addb50471b4129a5ea378b1514696.
Native fixture `native-player-oeicqx89` passed controls/seek/EOS and exact pages/heap cleanup.
Public YouTube sync for this candidate is PENDING.

A native idle TCP fixture reproduced missing carrier-loss notification on the previous kernel.
TCP now checks actual NIC carrier for established non-EOF streams, reports ENETDOWN (100),
and notifies readiness; restoring carrier cannot revive the failed tuple. Native BIOS and UEFI
tests passed blocked recv, poll error, SO_ERROR consumption, new connection after restoration,
and exact cleanup. Host peers supply test bytes only, never production media/TLS.
The baseline harness initially waited for EOF through a disconnected NIC; its corrected
version holds the peer until the loss observation and reproduces the kernel defect cleanly.
`test_tcp_link_loss` is included in full regression.

Focused evidence/source/build identities: `docs/wip/phase334/checkpoint6/`.
FULL REGRESSION IS PENDING for VM-area 1024 + carrier-loss + latest source.
The earlier checkpoint-4 full PASS covers PMM 1GiB, not these changes.
Next: actual online candidate controls/network recovery; then full regression, final normal
BIOS/UEFI launcher/pair checks, exact image restoration and local commit. No push.
Phase 33.4 remains ACTIVE/incomplete. Phase 33.5, parent Phase 33 and Rickroll remain unaccepted.
