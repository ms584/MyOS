# Phase 33.2 private owned PCM transport

## Current checkpoint: 2026-10-05 11:40 +07 (supersedes historical notes below)

PRIVATE ONLY, no production PCM source/import promotion; actual Cobalt0/5.
Production resource full75305 remains live on its own unchanged source/image.

Transport now prebuffers16384 real frames and supports explicit short-EOS START.
STATUS coherently masks DMA1, resets count flip-flop and reads remaining count:
partial real samples and pending half boundary credited without double-count.
FLUSH settles played partial frames, preserves lease/counter, drops unplayed.
Mixer IRQ status rejects stale/spurious IRQ, start/stop clear pending IRQ.
Actual driver/bridge host ASan/UBSan qDbmtX PASS11:17:01, including partial200,
pending boundary8292, failed start, stale IRQ and mapping output atomicity.
Hardware reference: primary QEMU10.2.1
[DMA](https://github.com/qemu/qemu/blob/v10.2.1/hw/dma/i8257.c),
[SB16](https://github.com/qemu/qemu/blob/v10.2.1/hw/audio/sb16.c).
Private PCM D5q2sn session88609 exit0 11:19:17 BIOS+UEFI actual DMA/IRQ,
prebuffer/EOS/partial flush/stable counters/440Hz/exit cleanup/exact resources.

New platform-audio.c and target audio-sink-abi.h implement pinned API18 typed
mono Float32/Int16 conversion, circular source, internal-worker update/consume
callbacks, initial update BEFORE Create returns, partial pause/resume/EOS and
Destroy joins active callbacks before freeing; no late callback after return.
Callbacks hold no internal registry/state lock; initial reentrancy tested.
Registry in main shared RW/NX TLS1888..1975 (after allocator), no mutable RX.
One sink/process, absent/busy/unsupported -> NULL with no callbacks.
Thread exhaustion rolls back physical lease and worker allocation.
Native hardware remains unsigned8-bit MONO11025Hz; float/int16 are software
conversion inputs. Host stereo16bit44100Hz capture is QEMU conversion, NOT
native stereo support. No compressed codec, decode texture or player claims.

Private sink VwUNI5 session6637 exit0 11:30:25 BIOS+UEFI basic native scopes.
Latest /tmp/myos-owned-pcm-image-E2RLa6 session80619 exit0 11:35:56:
BIOS+UEFI Float32/Int16, wrapped8192-frame source, partial pause/resume,
blocked callback makes Destroy wait/no late callbacks, all8 workers occupied
then failed Create/no callback and successful retry after joins; absent/busy/
relaunch/exact pages/heap. Logs private-native.log/private-native-uefi.log;
output dirs build/pcm-byb0tiwc and build/pcm-yta0jiwk. Runtime38192/packed30847.
Session34476 exit0 11:40:47: four invalid initial frame/offset cases followed
by successful Create, BIOS+UEFI. Session6709 exit0 11:45:36 adds active audio
worker/device lease UD2 main fault (USER FAULT vector6) and Escape cancellation,
then successful sink relaunch and exact heap/pages, BIOS+UEFI. Logs
private-termination.log/private-termination-uefi.log, build/pcm-r5fvx3uu and
build/pcm-ihlt95kc. Initial22547 failed only expected message 'ELF fault';
actual USER FAULT vector6 and exact resources observed. Corrected assertion
checks vector and invalid-executable result; no production/source fix.

Additional Windows work/ sources: platform-audio.c/audio-sink-abi.h,
build-audio.sh/exports-audio.py, elf-audio-sink.c/test_audio_sink.py,
build_owned_audio_probe.sh/check_audio_source_rollback.sh. Five strong sink
imports PRIVATE; production remains196/242,46 missing. Still required native
other forced-kill paths, ongoing invalid sources, starvation limits, engine use,
promotion/integration/full regression. Read player.h211..678 before its ABI.
No fake decoder/player success; no33.3/push or phase closure.

## Historical checkpoint (superseded where noted above)

2026-10-05 11:10 +07. PRIVATE ONLY; no production source/imports changed for PCM.
No33.3 codecs, no Cobalt acceptance. Production full75305 live on resource batch.

## Implemented private transport

Actual SB16 driver keeps per-DMA-half counts of real samples. Refill does NOT
acknowledge consumption; IRQ completion adds that half's count, not silence.
One validated process PID owns device; other owners/legacy queue cannot inject
or stop its data. Acquire rejects absent device/busy queued or DMA frames.
Explicit release stops/clears owned queue; scheduler_stop_process also releases
PID on exit/fault/kill. Flush preserves lease/completed-frame counter but drops
pending data. DSP-start failure rolls queue/head/tail back instead of losing
samples. This is not hardware error recovery proven by QEMU; host mock only.

SYSTEM_AUDIO8 request32/status32 draft bridge derives owner from process, never
accepts a user-selected owner. Validates full output before acquire mutation;
copies up to4096 queue bytes into kernel buffer before queue mutation. No DMA
from user VA; reserved/null/whole-page validation errors do not publish output.

Native device remains unsigned8-bit MONO11025Hz. Host WAV stereo16bit44100Hz
is QEMU conversion, not a native capability. No float/stereo claim.

## Evidence

Actual-driver queue/refill/IRQ and actual pcm_call host ASan/UBSan PASS11:00:49,
/tmp/myos-owned-pcm-host-Nitzgv. Owner/capacity/failed start/flush/no early ack/
release +mocked user mapping output atomicity/bounds/PID tests; NOT native DMA.
Private image /tmp/myos-owned-pcm-image-b8z8ct:
initial98837 terminal1 11:04:51 solely waveform-detector threshold. Native
fixtures all passed. Capture analysis11:05:57: amplitude -18176..18687,2604
periods near440Hz, median441Hz; host interpolation edge crosses via+256, so
adjacent -3000/+3000 threshold is wrong. Fixed positive zero-crossing detector
with separate amplitude test and>200 sustained periods, no relaxed frequency.
96000 terminal0 11:07:54: BIOS and UEFI full fixture/actual waveform PASS.
Two no-device launches and four audio launches/profile (cleanup/explicit pair
twice), IRQ consumption16384, protected-page output atomicity, flush/release,
automatic process-exit cleanup, repeated launch and exact pages/heap recovery.
Logs private-native-detector.log/private-native-uefi.log; initial failed log kept.

## Sources and required next work

Windows work/ sb16-owned.c/audio-owned.h, pcm-call.h/pcm-syscall.inc,
syscall-pcm.c/scheduler-pcm.c, elf-pcm.c/test_pcm.py. Host helpers
sb16-owned-check.c/pcm-syscall-check.c/check_owned_pcm.sh/analyze_pcm.py;
build_owned_pcm_probe.sh creates a private copied tree, no production mutation.
Pinned audio_sink.h/media.h/decode_target.h fully read from cached contracts.
Player678-line contract not yet read/implemented. No five sink exports yet.

Still required: real worker callbacks (initial status before create returns,
destroy waits/no callback after), float32/Int16 ABI conversion and supported
channels, cold-start prebuffer/EOS rather than initial partial-half silence,
partial-DMA accounting on pause/flush (current counter only completes halves;
must not replay already played partial frames), stale pending IRQ across new
lease, source circular buffer seek/resume, malformed source states, worker/
thread cancellation and fault/kill native cleanup (hook exists but only exit
is native-proven). No descriptor success stubs, fake player/decoder handles or
compressed codec capability claims. Integrate only after relevant native scopes
and mandatory architecture regression; do not mutate resource full owner.
