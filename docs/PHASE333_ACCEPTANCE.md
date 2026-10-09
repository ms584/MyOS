# Phase33.3 standard media acceptance - 2026-10-09

Status: PASSED for the declared MyOS/QEMU profile. Phase33.4 is not started.
This closes standard media only. Parent Phase32/33, YouTube and Rickroll remain
unaccepted. Implementation checkpoint656865a; local closing commit includes
this audit, final evidence and guide. No push.

## Declared profile and measured result
Declared before acceptance:WebM/VP8/Opus,160x90@15fps, QEMU512MiB/KVM CPUhost,
RTL8139, native11025Hz mono SB16; >=30s moving video+sound,
absolute presented-video vs consumed-audio error<=100ms after preroll,
incremental media RAM<=32MiB. No limit was relaxed after a failed observation.

Pinned Cobalt27.lts.3/Evergreen7.3.2/Starboard18; native generic libvpx1.15.2
VP8 and fixed-point Opus1.5.2. Pins/demux source hashes/dependency configuration:
PHASE333_MEDIA_LOCK.json, PHASE333.md, docs/licenses/phase333.
Core SHA256:fd8bbf7c60d4906a72e4f4ec89bc51c7dbc9bdd83fd1e4c1cd14a8eb3a8b730a.
Production provider SHA256:41901a137421bf62e63585f8ea5339fd5b3addb50471b4129a5ea378b1514696.
Normal BIOS/kernel/MRT match the exact uninterrupted/control/adverse test inputs.

Continuous native engine session:600 decoded frames,0 dropped/corrupted,
598 sync samples,max17.890ms. Captured binary frame IDs38..599 span37.4s
at14.948fps; original same-session native PCM40.062s has440/660/879/550
positive crossings against source440/660/880/550Hz windows. Three preroll
constant gaps finish before0.465s; no steady held-sample gaps afterward.
Actual initial hardware underruns5, not an unmeasured zero.
Conservative total media RAM27103200B (25.848MiB).
Three controls/restart/relaunch sessions peak31313936/31268880/31199248B,
maximum29.863MiB, and all remain below33554432B. Maximum lifecycle sync46.733ms.
A restarted player dropped3 frames in one run; the uninterrupted run dropped0.

## Requirement-by-requirement audit
All paths below are under wip/phase333/checkpoint6 unless noted.

| Requirement | Authoritative evidence | Result |
| --- | --- | --- |
| Read instructions/plan/Git and close prerequisites before media | PHASE333.md initial audit; PHASE33_NATIVE_WEB_ACCEPTANCE.md native HTTPS/JS/DOM/input/120s/negative certificates and full regression; current native engine sessions preserve these paths | PASS |
| Inspect/pin container, demux, codecs, dependencies and licenses | PHASE333_MEDIA_LOCK.json; PHASE333.md; production-provider-report.json; hash-verified source archives and docs/licenses/phase333 | PASS |
| Read streams, demux and decode inside MyOS | Native asset production-asset/; actual pinned Cobalt HTTPS/MSE production-continuous/; decoded guest frame IDs and SB16 PCM | PASS |
| Starboard player, decode target and audio sink | Native asset real decode-target texture/pixel/ownership tests; engine supported punch-out/Viz underlay; real SB16 consumed-frame/IRQ counters | PASS |
| Bounded streaming/backpressure/timestamps/A-V/RAM | Native bounded sample batches,PCM/video queues; engine8KiB requests and5s-forward/3s-back eviction; complete512tick PMM allocation history plus conservative heap/provider delta in continuous-analysis.json and lifecycle-analysis.json | PASS |
| Pause/resume | Three actual input sequences; frozen visible frame92 and consumed/IRQ counters during pause; counters advance after resume | PASS |
| Seek flush and new timeline | Three native seeked events at15s; visible new frame IDs234/265,246,250; native asset decoder reset/sample ownership test | PASS |
| EOS, stop and restart | All three reach actual ended40s; fresh generation2 MediaSource/player; stop gives black viewport and halted/reset hardware counters; shell resources and subsequent launches | PASS |
| Asset first, then HTTPS/engine | Recorded initial asset checkpoints precede engine promotion; latest production-asset rerun and separate production-continuous/ | PASS |
| Corrupt/truncated input and decode errors | production-faults/ full-batch VP8/Opus errors; production-https-corrupt/ native parser/video error4; production-https-truncate/ length/network failure; new stream recovery and stop | PASS |
| Network stall | production-https-stall/8s range delay, real starvation1, media clock advances after recovery, input stop/shell/exact resources | PASS |
| Memory exhaustion and close while playing | production-engine-oom/ two real0-free-page events and RangeError; production-engine-close/ two forced closes; actual shell mem/relaunch/exact PMM/SHM/aliases and warm heap | PASS for OS response/reclamation |
| >=30s moving standard video with sound in same MyOS session | production-continuous/ original WAV and19 actual moving guest PNGs, continuous-analysis.json | PASS |
| Resolution/fps/sync/RAM declared and actual values reported | Original PHASE333.md preacceptance envelope; measurements above and original counter/pixel/PCM records | PASS |
| Focused tests and full regression | production asset/faults, transient PMM BIOS+UEFI, MID/HIGH pthread/underlay UEFI; full-regression7.log.gz terminal0 at16:52:44 | PASS |
| Restore normal images | final-normal-identity.json; final-normal-uefi-build.log.gz; final UEFI focused logs terminal0; root BIOS/UEFI/kernel/MRT hashes independently matched | PASS |
| Logged checkpoints, guide, handoff and local commits | tools/run_logged.py outputs; checkpoint1..6; PHASE333_EXPERIMENT_GUIDE.md, HANDOFF.md, PROJECT_STATE.md; local implementation656865a and closing commit | PASS |
| No forbidden substitute, push or33.4 work | Host only encodes original fixture/serves bytes/captures framebuffer/PCM; guest performs fetch/TLS/JS/demux/decode/render/playback. No host browser/player/decoder, MYV1 acceptance or YouTube claim | PASS |

## Scope and observed limits
The accepted engine path is supported Starboard punch-out/Viz native underlay.
Decode-to-texture is real and tested in the native asset fixture; pinned Cobalt
ANGLE texture-output extension requirements remain unavailable. No extension
is falsely advertised. VP9/AV1/H264/AAC/DRM, other resolutions/rates, public-site
compatibility and real hardware are not claimed.

The first concurrent production asset run exceeded100ms (120.959ms) and remains
in production-asset-first-failed/. Identical rerun87.883ms passed. Concurrent host
VM contention is a possible cause, not proved. Acceptance uses the declared
single media VM profile; do not discard the failure. The audio8/video1 candidate
failed324.012ms and was replaced before acceptance.

Latest OOM catches240MiB guest ArrayBuffer allocation failure while physical
free pages reach0. JavaScript remains responsive but media later stalls with
network allocation errors. Escape cancels all threads; shell mem responds;
second launch/close returns exact pages/SHM/aliases and heap515840B.
This proves the requested MyOS response/reclamation criterion, not uninterrupted
playback under OOM or universal graceful engine recovery. Earlier provider Mesa
faults are retained in old-provider-oom-relaunch/ and checkpoint5.

Cold profile/cache files retain61440B for full playback,40960B for partial close/
OOM; warm repeats are bounded/exact in these tests. Sanitized inode ownership
shows linked files with refs0, not outstanding decoder/sample/frame allocations.
The36608B lazy inode pool is established before the ELF live media baseline.
No raw heap or TLS private keys are published.

Reproduction: [experiment guide](PHASE333_EXPERIMENT_GUIDE.md).
Evidence: [checkpoint6](wip/phase333/checkpoint6/README.md).
