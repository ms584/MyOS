# Phase 33.5 — Rickroll acceptance

Accepted 2026-10-12 on the declared MyOS/QEMU profile. Phase 33.5 and parent
Phase 33 are closed. Phase 32 was separately accepted after the owner explicitly
confirmed search → selection → playback with sound. This is QEMU acceptance.

The owner accepted A/V synchronization error **<=500 ms** on 2026-10-12.
The previous 100 ms target was revised explicitly, not silently removed.
Historical failures retain their original target and scope in the checkpoints
and `checkpoint22/pointer-boundary/pre-closure-documents.json.gz`.

## Actual final evidence

URL: https://www.youtube.com/watch?v=dQw4w9WgXcQ

Baseline Git commit: `76eaf768f82dfdee56f3cfdf32f0498dac92ad0d` plus the tested
source changes committed with this report. Final build hashes and the tested
source-diff hash are in [final-acceptance.json](wip/phase335/checkpoint22/pointer-boundary/final-acceptance.json).
No production source changed between final regression and these recordings.

| Gate | Observed result and evidence |
|---|---|
| 33.1–33.2 native web/runtime prerequisites | Actual source-core native web/tooltip fixtures in checkpoints 8/9; accepted loader/HTTPS/HTML/JS/DOM/input contracts retained; current full architectural regression PASS. |
| 33.3 standard media and 33.4 YouTube prerequisites | Accepted PHASE333_ACCEPTANCE/PHASE334_ACCEPTANCE plus affected current normal VP9 faults/OOM/lifecycle, late-bounds/gap and played-preview negative/positive tests. No document-only pass. |
| Normal image, native online Rickroll | New normal session 821qbsn4 opened the actual URL inside MyOS, closed and reopened; fresh uwzd1yy5 repeated after a new QEMU boot. Real Rickroll frames/title inspected. |
| >=30 s motion and same-session audio | Reopen recording 1wtj3hln:32 distinct movie samples before pause,42.495 s native PCM. Fresh mkl4wcyt:33 distinct movie samples before pause,54.989 s PCM. Owner accepts audible playback; tools do not recognize the song independently. |
| Pause/resume | Native K at40 s/45 s. Reopen movie identical43.21–44.25 s and PCM delivery flat42.61–44.96 s; fresh movie identical41.21–44.29 s and PCM409600 bytes flat41.03–44.94 s. Movie/PCM continue after resume. |
| Seek forward/backward | Native L at55 s/J at62 s; inspected new movie frames and +10/-10 overlays. Reopen position~0:42→1:00→0:52; fresh new backward position1:51. Cleared plane/new frames and focused seek/flush regression retained. Owner control/audio acceptance is recorded separately from pixel/PCM checks. |
| Close/desktop/reopen/fresh boot | Three normal closes over two boots: physical257743 pages exactly recovered, SHM0/aliases0; actual desktop Demo A screenshot and shell mem response. |
| A/V sync | Normal first player204.618 ms; reopen140.037 ms; fresh87.339 ms. All <=500 ms. Native scalar counters read without stopping the VM, hardware clock failures0. |
| Resource recovery | Post-close non-file heap777728 B in measured reopened/fresh cases; linked cache retained221184/282624/102400 B, unlinked capacity0. Retained linked cache is disclosed; exact warm-cache plateau/endurance is not claimed. Ordinary RAMFS64 slots/per-file64 KiB bound cache storage. |
| RAM measurement | Earlier normal unchanged core/provider70 s capture peak537649152 B, complete history during capture only; old mouse kernel. This excludes startup and is not a new-kernel peak/minimum-RAM claim. Guest configured1024 MiB. |
| Phase32 independent search | Native homepage query→official result→RickID,39 changing movie samples/39.787 s PCM, Stats confirms VP9(243)/Opus(251). Same unchanged core/provider; pointer fix retested independently. Owner explicitly confirms search/selection/sound on2026-10-12. |
| Focused/full regression | BIOS/UEFI/default-RAM absolute mouse outside-viewport/recovery, absent-vmport PS2 fallback and native underlay PASS. Full tests/test_system.py exit0 at15:36:04; normal five restored byte-for-byte. |

Detailed final results, commands, safe serial/network summaries and hashes:
[wip/phase335/checkpoint22/pointer-boundary](wip/phase335/checkpoint22/pointer-boundary).
The historical capture manifests keep phase_passed=false: they are observations,
not retroactively altered acceptance declarations. `final-acceptance.json` is the
final requirement audit with explicit owner confirmations.

## Configuration, engine and media path

QEMU10.2.1, KVM, CPU host,1024 MiB, BIOS800x600x32, RTL8139 user networking,
SB16/PulseAudio11025 Hz unsigned8-bit mono, SDL show-cursor=on; snapshot boot
and assets drives. Exact argv: final-reopen-network.json/final-fresh-network.json.
Host tools: GCC15.2.0, binutils2.46, NASM3.01, Python3.14.4.

Cobalt27.lts.3, Starboard18, Evergreen7.3.2, Mesa25.0.7, libvpx1.15.2,
Opus1.5.2, Clang21. Source-core SHA72a802f90fddd071971bca252a00ffbc824c3ef48ecc19197e6b6c0665848505;
provider SHAa98f65782c4e394874f5a887ab6a4d06c5b4ad6d7cdb97ddc560cf807a28a220.
Actual stream: YouTube native SABR/WebM, VP9 and Opus48 kHz stereo, guest
conversion to SB16 PCM. Auto source640x360 decoded to160x90 then scaled into
766x431 native plane. Do not describe these final captures as native144p/30fps.

Every playback step—fetch,TLS,JavaScript,demux,decode,render,audio—executes in
MyOS. Host tools build, emulate hardware and capture evidence. No host browser,
YouTube decoder/transcoder, prepared clip or MYV1 substitute was used.
Network summaries confirm native YouTube/Googlevideo DNS and TCP443 traffic;
raw encrypted PCAP stays private and hash-linked. Safe serial exports contain
only allowlisted media/resource observations, no signed media URLs or cookies.

## Recording and limits

Original authoritative PNG/WAV/timeline directories:

- build/phase335-pointer-boundary-manual/manual-session-821qbsn4/native-recording-1wtj3hln
- build/phase335-pointer-boundary-manual/manual-session-uwzd1yy5/native-recording-mkl4wcyt

The fresh directory contains one5.18 MB `launch-1-screen-audio.avi` review copy.
1 FPS screenshots and the AVI's approximate wall-clock mux are not an A/V sync
measurement; native hardware-clock counters are used for that. Original hashes
are in final-recording-manifest.json. Raw recording files stay local in ignored
build rather than adding large binaries to GitHub.

Loading can take minutes. UI input/overlays can lag; reopen pause settled about
2.6 s after input. The500 ms criterion measures A/V sync, not loading or input
latency. Lifetime dropped frames: reopen1330/2140; fresh2160/3828. Rebuffering
remains; acceptance does not promise Chrome parity, sustained30fps, all websites,
login/DRM/Shorts support or real hardware/USB acceptance.

Earlier cleanup reclaimed8605818880 B inside Ubuntu and71487576 logical bytes
of task-created Windows dependencies. Windows VHDX compaction/refund remains
separate maintenance; never delete ext4.vhdx. Final normal images are restored.

Start-to-finish Thai commands: [PHASE335_USER_GUIDE_TH.md](PHASE335_USER_GUIDE_TH.md).
