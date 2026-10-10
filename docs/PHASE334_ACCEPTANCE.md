# Phase33.4 acceptance — tested MyOS/QEMU profile
Accepted2026-10-10, after requirement-by-requirement review and native expiry audit.
Phase33.5, parent Phase33 and Rickroll acceptance remain NOT PASSED.

## Tested configuration and production boundary
Production source da08b2db58df7b9d4f9d6cc850cbb77729ddc98b;
media-player source90dfe4b6d6f45097bad559d8aecc0bb818aafd8d2e796e81b300643d545e8943.
Optional native-underlay VP9/trace provider-sey3hj06 SHAba9620ca077d7942775d9ec9fcca0e8561e39c3989f17ae024e625d5122c2c85.
Cobalt27.lts.3/Evergreen7.3.2/API18 corefd8bbf7c60d4906a72e4f4ec89bc51c7dbc9bdd83fd1e4c1cd14a8eb3a8b730a.
QEMU1024MiB,KVM,CPUhost,RTL8139,SB16,800x600. Exact QEMU command and
normal-image/asset/core/provider identities are in the evidence JSONs.
HTTPS/TLS, JS/DOM/input, SABR/MSE/demux, VP9/Opus decode, rendering and audio
run inside MyOS. Host packages assets and records QEMU PNG/nativePCM only.
No host YouTube media fetch/player/downloader/decoder/transcoder/converting proxy.
No fake credentials or platform signature; public frontend worked without them.

Selected stream: WebM VP9itag278 + Opus251, nativeStats256x144@30,SDRbt709,SABR.
Native Opus stereo48000Hz -> SB16 mono11025Hz,U8. Presentation160x90;
source30fps is not a zero-drop30fps-display claim. Decoded VP9 cap640x360@30,
profile0/SDR8bit; HDR/DRM/H264/AAC remain unsupported with actual error reporting.
Owned mappings peaked568778752bytes/542.43MiB, not measured RSS/minimum RAM.
Queues/PCM/future-video windows are bounded; dropped frames and rebuffers remain
in metrics. The profile is the optional VP9/native-underlay package, not the
unchanged defaultVP8 provider or a claim that every YouTube format works.

## Requirement audit
| Requirement | Inspected authoritative evidence | Verdict |
| --- | --- | --- |
| 33.1–33.3 native prerequisites | Native-web replay referenced by PHASE334.md; checkpoint11 cobalt-standard600frames,38.533s motion/39.953s PCM,max39979us,controls/cleanup; first-video-native late-bounds/gap/seek/EOS tests. | PASS on declared profile |
| Native YouTube/watch JS/network/render/media and real navigation/input | checkpoint11 current-online actions/evidence/native PNGs/serial: direct aqz-KE-bpKQ, native homepage typing/search/result selection, watch and second launch. No consent prompt blocked these tested routes. | PASS |
| Real online movie>=30s + same-session sound | checkpoint11 current-online analysis: direct50.447s and search30.283s changing movie pixels, native497.971s PCM/492audible1s bins; human-reviewed movie and Stats PNGs. URL https://www.youtube.com/watch?v=aqz-KE-bpKQ . | PASS |
| Pause both/resume/seek/close/reopen and responsive MyOS | Native input/actions, zero changed pause pixels for10s, actual PCM_STATUS consumed/position/presented held, FLUSH/hardware-stop; resume,3seek tickets,two reopens; exact257743pages/SHM0/aliases0 and desktop/shell. | PASS |
| Actual media interfaces/codecs/stream selection, TLS media hosts, concurrent segmented loads | Native MSE standard and public SABR playback; sealed public trust roots and native certificate/hostname negatives in prerequisites. Native Googlevideo POSTs/segments and repeated stream formats; actual SABR transport recorded, no fabricated GET-range requirement. | PASS for selected transport/formats |
| Bounded buffering and A/V clock | Native player/source fixed queues/buffers, actual hardware-clock queryfailure0; public maxima88217/89876/89577/89970us<=100ms; RAM/drop/rebuffer metrics retained. | PASS |
| Timeout/loss/unsupported/error recovery | checkpoint11 timeout10s/14s stall and N recovery; NIC loss130s/exhaustion/held clock/automatic resumed link; malformed/unsupported/realENOMEM tests and exact cleanup; controlled403 restart. | PASS |
| Actual URL expiry/error and return to MyOS | checkpoint12 authentic original URI/body positive200/1087988bytes; checkpoint13 native complete UMP46 after real expiration, linked request44/stream47/TLS74 and identical actual outgoing body; long-session fresh-watch35.220s movie/188.510s PCM,max89945us, exact desktop/resources. HTTP200/CORS0 alone was rejected. | PASS; combined scoped native evidence |
| Date/URL/build/QEMU/format/resolution/fps/RAM/images/audio/logs | checkpoint11/current-online and checkpoint13/evidence/build-identities/qemu-command/PNG/WAV.gz/safe logs/SHA256SUMS; raw signed queries/body/privateCA stay ignored build and are hash-linked. Diagnostic footer/hash sampling limits disclosed. | PASS |
| Focused tests/full relevant regression/normal restored | checkpoint11 focused results all terminal0; checkpoint6 regression/result.json full test_system.py terminal0 plus BIOS/UEFI launcher/pair/restoration. No later production architecture change; current production diff since da08b2d is empty. checkpoint13 normal-restored.json all5 exact/no QEMU remaining. | PASS for recorded scope |
| Experiment guide/handoff/local checkpoint and next stage boundary | This acceptance, PHASE334_EXPERIMENT_GUIDE.md,HANDOFF.md,PROJECT_STATE.md and execution plan; local commits only. | PASS |

The historical observation/test manifests keep phase_passed=false because their
scopes were narrower. This audit combines actual inspected evidence; it does not
retroactively turn failed runs or arbitraryGET/CORS0 into passes.
Full architecture results are reused for unchanged production code/configuration;
later optional player behavior has focused/native standard/public/error checks.
The isolated diagnostic kernel is not a production filesystem change.

## Reproduction and remaining limits
Use the exact pinned provider/resources/fonts and commands recorded by the
experiment guide and each evidence bundle. Run through tools/run_logged.py.
Main public observer: tests/test_cobalt_youtube_observation.py; analyzer
tools/analyze_youtube_observation.py with motion1:150:205,2:240:275 and
pause1:213:224,2:283:295. Native expiry checker source/inputs are in checkpoint13;
its URI must come from a real authorized native session, remain private and wait
for actual expiration. SABR may signal expiry via UMP46 inside HTTP200.
A short follow-up after the native UMP probe encountered real302/429; it was
closed cleanly and is not a positive playback result. Public-service availability
and certification on other devices/frontends remain unclaimed.

Ready for33.5: pinned working profile, real YouTube network/media path, native
controls/search/selection, error/recovery checks and auditable captures.
33.5 must independently use dQw4w9WgXcQ on the normal image, >=30s music/video
with controls and close, repeat after fresh boot, and receive its interactive
visual/audio acceptance. Do not start/close it automatically or declare parent33,
Rickroll, real-hardware or wider Phase32 acceptance from this33.4 checkpoint.
