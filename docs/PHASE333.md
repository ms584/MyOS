# Phase 33.3 - standard media
Updated 2026-10-09 09:55 +07. Status: OPEN; native player checkpoint passes,
real Cobalt visual playback is NOT passed.
Baseline main 158f360 / production source 9aece1e. No Phase33.4 work.

## Prerequisite audit
Read AGENTS.md, PROJECT_STATE.md, HANDOFF.md and PHASE33_EXECUTION_PLAN.md.
Fetched origin: origin/main=158f360, no incoming changes; tracked tree initially
clean. Owner's unrelated untracked promotion logs preserved.
Phase33.1/33.2 native web evidence is valid for its recorded QEMU/KVM profile:
real Cobalt JS/DOM/timers/input/controlled verified HTTPS/concurrent assets,
two launches, >60 seconds, cleanup and complete system regression. Independently
replayed committed PNG/JSON/serial evidence on 2026-10-09; PASS.
This is recorded evidence review, not a new browser execution.
See PHASE33_NATIVE_WEB_ACCEPTANCE.md and its original source/image identity audit.

## Pinned interface and dependencies
Machine-readable pins: PHASE333_MEDIA_LOCK.json. Notices: licenses/phase333/.
- Cobalt27.lts.3, Evergreen7.3.2, Starboard18, source
  3befd332486952f6603ae3baf2a8713878d6e200; actual core identity remains the
  existing PHASE33_COBALT_AUDIT.json pin.
- Container WebM; video VP8; audio Opus. VP9/AV1/H264/AAC/DRM are not implemented.
- libvpx1.15.2, d168454ecd099805c675d4a98c66f4891373302a:
  generic C VP8 decoder, single thread, no CPU dispatch, no encoders/VP9,
  PIC and MyOS SDK only. BSD3 and separate PATENTS grant retained.
- Opus1.5.2, ddbe48383984d56acd9e1ab6a090c54ca6b735a6:
  fixed point, float API/intrinsics/DRED/OSCE off, VLA scratch, hardening on.
  Upstream source groups build into a static archive; unused encoder objects
  are not linked into the decoding fixture. BSD3/COPYING patent references
  retained; no claim about legal clearance beyond upstream notices.
- Existing native musl1.2.6 MyOS string/format/file/jump/math SDK closure is
  hash-verified against accepted provider84sokh6g. No Linux libc/DT_NEEDED.
  The pure abs source is checked byte-for-byte before target compilation.
- Pinned Cobalt source inspection confirms WebMStreamParser handles tracks and
  clusters; audio/video clients recognize A_OPUS/V_VP8; SbPlayerBridge delivers
  elementary samples, timestamp/key-frame metadata to Starboard. The player API
  itself does not receive a container. Presence in source is NOT proof of
  engine-demux execution in MyOS; that is a later engine acceptance checkpoint.
  Sources/hashes were inspected at the pinned commit, not current upstream.
- Native checkpoint uses the small in-tree media-webm reader: length-bounded
  pull reads, 256KiB maximum packet, 16 nesting levels, timestamp-scale checks,
  no whole-file guest load. Borrowed packet expires after synchronous callback.
  This reader is a documented subset, not replacement of full Cobalt demux:
  VP8+Opus tracks required, one-byte track numbers, no lacing, encryption or
  unknown-length clusters. It does not implement seek, preskip, discard padding,
  queues or clocked presentation. Unsupported structures fail closed.
- libvpx target adaptation removes only an unused vpx_timer.h include from
  vp8/decoder/onyxd_if.c. No timer calls or decoder algorithm is replaced.
  Every extracted source is checked against the hash-pinned archive or this
  exact change before build. Initial unsupported configure flag and missing
  header/build-tool failures remain in ignored build/media-evaluation logs.

## Checkpoint 1 - native standard asset demux/decode
Final focused bundle: build/media-evaluation/native-decode-95k2xx2n.
Committed public evidence: wip/phase333/checkpoint1/.
Actual MyOS Ring3, non-root explicit KVM, 128MiB, normal BIOS image:
- Upstream unmodified opus-trimming-video-test.webm, SHA256
  31a8ef844e6732242029e3de1a0153e6cffd66a31537f1f034108d811d39a5b4.
- Two complete decodes: 383 VP8 frames, 107 Opus packets,
  616320 decoded sample frames at 48000Hz/stereo per run. 757336 PCM sample
  values exceed absolute500; Y-plane rolling hash15537460444265374182
  matches both runs. This is deterministic native output, not an independent
  reference-decoder or playback oracle.
- Fixture720x480 allowed only for this unclocked decoder checkpoint. VP8
  keyframe dimensions are checked before decoding; maximum720x480.
- Explicit 2MiB worker stack. Initial small launch-stack test hit its guard
  during a 23KiB PCM stack probe; disassembly identified the instruction.
  Guest survived and returned shell/exact resources; worker-stack fix then
  completed. Original failure logs preserved. This is not a codec fault.
- Invalid EBML and half-file truncation rejected in guest, shell responsive,
  two launches plus both failures recover free frames28375->28375,
  SHM/alias0->0 and kernel heap374272->374272. No late worker remains.
- Parser-only host ASan/UBSan corpus passed: valid fixture,1024 prefixes,
  2000 deterministic bit mutations and2000 random short buffers. This is
  parser memory-safety testing, not host media decode/playback.
- Earlier default TCG decoder-only run also completed two decodes/resources;
  no real-time performance or playback claim.
- All five normal artifacts still match main-normal.sha256 from accepted
  native-web baseline. No normal image/source was changed. No architecture
  regression is claimed from this fixture checkpoint; full system suite is
  still required at the player/loader integration checkpoint and phase end.

## Reproduce
Existing accepted Cobalt provider/SDK resource packages must be present.
Run all commands through the activity logger:
```sh
python3 tools/run_logged.py -- python3 tools/fetch_media_sources.py
python3 tools/run_logged.py -- python3 tools/build_media_codecs.py
python3 tools/run_logged.py -- python3 tests/test_media_webm_safety.py
python3 tools/run_logged.py -- python3 tests/test_media_native_decode.py --kvm
python3 tools/run_logged.py -- sha256sum -c docs/wip/native-pmm512/integration-20261008/main-normal.sha256
```
Omit --kvm for default TCG; never silently fall back. The test creates a new
disposable asset disk and frozen normal boot copy, never uses owner data disks.
Fetching here retrieves source archives/standard test asset only. Actual file
reads, demux and decoding above happen in MyOS. No host decoder is executed.

## Checkpoint 2 - native player, controls and bounded queues
Public evidence: wip/phase333/checkpoint2/. These are actual MyOS/QEMU
measurements of the native paired fixture, NOT Cobalt engine acceptance.
- Optional version1 readonly media-provider marker and17 explicit strong RX
  exports. Malformed markers/weak/missing/object exports fail closed before
  execution; absent marker retains legacy runtime media-unavailable bindings.
  Current BIOS ABI test provider-abi-9s5vkbi1 passes exact page rollback and
  responsive shell. UEFI equivalent is running; no premature PASS claim.
- Native generic VP8/Opus decoders feed4 RGBA160x90 frame slots, one pending
  <=256KiB borrowed packet per track,32768 mono Int16 PCM sample frames,
  and at most8 independently retained GL decode textures. API ownership and
  callbacks remain asynchronous; deallocation follows actual consumption.
- Actual consumed SB16 frames drive the shared A/V clock. Seek tickets destroy/
  flush the old sink, release packets, reset codecs/queues/anchors and preroll.
  Texture handles survive player destruction until explicitly released.
- PCM_CONFIGURE adds an opt-in1024-frame DMA half. Other clients retain8192
  frames per half /16KiB prebuffer; configure requires an owned stopped empty
  lease and rejects other sizes/fields. Extended PCM focused tests passed
  BIOS pcm-tcu_ly9c and UEFI pcm-gyhe8_c3: bounds,busy,IRQ,partial consumption,
  flush,lease/default reset,real440Hz output,no hardware and exact cleanup.
- Frozen original test asset: assets/tests/phase333/standard-160x90-40s.webm,
  SHA256 c04a3650cfbfd72ef82020e2828ef96e2432b8a81ed1ab26b376648b7047dbe2.
  Standard WebM VP8+Opus,40seconds,600 video frames,160x90@15fps.
  Host encoding only creates this input; no host decoding/transcoding is used
  as acceptance. Guest reads/demuxes/decodes/renders/plays it.
- native-player-vpxup2_y / player-optin1.log PASS on512MiB explicit non-root
  KVM:684 rendered textures;629 framebuffer-readback A/V samples,
  maximum84043us (declared threshold100000us). Readback decodes source frame
  identity cells AFTER native GLES draw, compares with actual consumed audio
  clock after swap; excludes initial/seek preroll and paused intervals.
  It is a sampled software-presented metric, not physical display latency.
- Original same-session QEMU SB16 WAV contains36.6075seconds /36 one-second
  tone windows above the test amplitude threshold. WAV is captured44100Hz
  stereo by QEMU from guest11025Hz mono8-bit SB16 output, not the source
  decoder PCM. Moving framebuffer PNGs and original WAV lossless zip retained.
- Pause/resume,seek15seconds,EOS,stop and same-process restart/close while
  playing pass;2425 sample buffers allocated/released; free pages126679
  before/after,SHM0,alias0,heap439040 before/after.
- Fault fixture player-faults-ivdlnv7f passes two native launches at64MiB:
  audio/video decode errors,rapid seek revisions,early close,audio-only/
  video-only creation,real guest mmap pressure causing player creation to
  fail,then release/recovery. Resource counters return exactly and shell
  continues. This does not cover HTTPS network faults or every damaged file.
- Original unsuccessful attempts remain preserved: default short-DMA change
  broke the legacy prebuffer assertion; opt-in design fixed it without
  weakening that assertion. Earlier A/V max116424us failed the declared
  target; nearest-frame selection at renderer pull then passed, without
  relaxing100ms. A harness run rejected mutable boot-image identity and was
  rerun using an immutable image copy.

## Real Cobalt engine status - OPEN
- Exact pinned core fetches controlled verified HTTPS WebM byte ranges and
  demuxes through its real MediaSource/WebM pipeline in MyOS. SourceBuffer
  serializes8KiB appends,5s forward backpressure and removes compressed
  history older than3s once >5s accumulates; telemetry references capped32.
  This bounded retention revision still needs its completed engine run.
- The pinned Evergreen USE_STARBOARD_MEDIA build has progressive URL demux
  disabled when FFmpeg is absent. A direct video src attempt correctly
  failed with error4; standard MSE is the supported inspected engine path.
  No YouTube code/service is involved.
- native-verified-tls-ywvd5_z9 has metadata160x90,playing/clock,keyboard
  pause/resume/seek/stop events. Its video rectangle is BLACK. Native
  player trace shows decode/audio progress, but no GetCurrentFrame pulls.
  Therefore DOM/media clock events are NOT visual playback acceptance.
- Diagnostic GPU profile log identifies Cobalt GPU context fatal failure:
  missing GL_ANGLE_robust_client_memory in raw native Mesa. No unsupported
  extension is advertised as a workaround. A separate frozen diagnostic
  image tests Cobalt's validating command decoder. Normal source/image
  restored automatically; diagnostic runs are not timing acceptance.
- Remaining: actual moving engine video+sound >=30seconds in one session,
  engine controls/EOS/restart,network stall/truncation/corruption/close tests,
  measured bounded stream retention and incremental RAM including renderer
  resources. Engine media clock currently advances slower than wall time;
  real-time behavior and A/V sync must be measured after the image path works.

## Reproduce player checkpoint
Run from repository root, using the logger for each command. Codec/renderer SDK
prerequisites above must already exist; build_media_provider prints its new
directory. Use that directory with --provider, never its provider.so filename.
    python3 tools/run_logged.py -- python3 tools/build_media_provider.py
    python3 tools/run_logged.py -- python3 tests/test_media_native_player.py --provider build/media-evaluation/provider-qxwp5i1z --boot-image build/media-evaluation/optin-dma-normal/myos-current.img --kvm
    python3 tools/run_logged.py -- python3 tests/test_pcm.py
    python3 tools/run_logged.py -- python3 tools/build_uefi.py
    python3 tools/run_logged.py -- python3 tests/test_pcm.py --uefi
The paths shown identify the recorded runs; fresh builds print different
directories. Media ABI tests require --core pointing to the generated native
player fixture shared object and support --boot-image/--uefi/--kvm.

## Remaining gates and preacceptance envelope
Initial target declared BEFORE playback measurements:160x90 at15fps,
mono11025Hz SB16 output from decoded Opus, <=100ms absolute presented-video
timestamp versus actually consumed-audio timestamp after preroll. QEMU512MiB;
incremental media allocation ceiling32MiB including codec state,stacks,sample/
PCM queues and decode textures. RAM budget has NOT been measured/passed.
Record method,drops,underruns,maxima before acceptance; do not silently relax
the envelope to turn a failed measurement into PASS.

The first full phase regression was interrupted before subsequent driver
changes; terminal1, NOT PASS. New complete tests/test_system.py is running
against current loader/PCM architecture (phase333-full-regression2.log).
Final focused tests,complete full regression and normal BIOS/UEFI restoration
are required before phase acceptance. Checkpoint1's unchanged-image statement
describes that historical checkpoint only; loader/PCM images have since changed.

Phase33.3,Phase32/33 overall and YouTube/Rickroll remain NOT PASSED.
No Phase33.4 work and no push.

## Checkpoint3: actual engine video, still OPEN (2026-10-09 10:36 +07)
The supported Starboard punch-out output activates Cobalt's Viz underlay.
Native syscall60 copies a bounded160x90 RGBA frame, composites it beneath
premultiplied UI alpha and clears it on seek/stop/owner exit. No ANGLE extension
is falsely advertised. Decode-to-texture remains available to the native fixture.
Both real engine sessions now show changing decoded frames and output SB16 PCM
from the same MyOS session over ordinarily verified HTTPS/MSE. First run has22
distinct video crops and40.60s captured audio. This supersedes the black-image
observation above, without retroactively passing it.

Guest pixel contract tests passed before and after skip-unchanged composition:
opaque occlusion/alpha, full bad-frame rejection, timestamp order/reset, moves,
hide/show, cancel/fault/relaunch and exact fixture resources. Host ASan/UBSan
state-machine checks also passed. The engine still leaves61440bytes of kernel
heap after exit; source, cause and repeatability need investigation. Captured
tone windows show real-time quality needs measurement. Neither sync nor complete
engine cleanup is passed. New provider aggregate presentation/audio-consumption
metrics compile successfully; a non-trace run is next.

Native asset-only accounted sampled media RAM is12636160bytes, including codec/
provider preloaded image delta and fixture stack. Engine/MSE/compositor32MiB
budget remains unmeasured. Existing PCM and media-provider ABI tests passed both
BIOS and UEFI. Safe evidence is in wip/phase333/checkpoint3/.

Full regression2 terminal1 at the final memory boot assertions: preceding
helpers restore PROCESS_TESTS=0, while boot_test requires process-probe markers.
The harness explicitly rebuilds the instrumented image before that group;
no assertion is removed. Focused memory suite passed. Full regression3 runs
against promoted native underlay architecture; final full PASS and normal
BIOS/UEFI restoration remain required. Do not build root concurrently.
No Phase33.4/YouTube claim and no push.

Full regression3 stopped at build, NOT PASS: instrumented kernel264524bytes
exceeds the historical256KiB script cap. Normal kernel is256352bytes.
The development cap is now288KiB (576sectors), also enforced in stage2 NASM.
Physical safety is unchanged: load starts0x10000, maximum file ends0x58000;
linker still requires all kernel+BSS below0x80000 stack and0x90000 DMA.
Floppy CHS/BIOS EDD sector reads use bounded destination segments, with no
wrap at this envelope. Full regression4 and normal BIOS/UEFI builds are pending.
No media sync/RAM acceptance threshold changed.

## Checkpoint4: full regression and UEFI, engine gate still OPEN
Full tests/test_system.py completed11:20:44 with exit0 and normal BIOS
restoration. Normal UEFI build plus real guest underlay pixel/lifecycle and
PCM tests passed13:28. Original full log ZIP,UEFI logs and normal image hashes
are in wip/phase333/checkpoint4/. Kernel source corresponds to3d3f76f.
Normal artifacts are frozen in build/media-evaluation/underlay-normal-final-20261009.

The traced native provider measured538 presentations,530 A/V samples,
max16439us. Two isolated register-diagnostic non-trace observations played
with542/541 presentations,max11148/10423us and2 drops. These do NOT establish
stable normal-image acceptance. Earlier two normal non-trace runs faulted in
pinned core at RIP0x800343612e just after MSE metadata; cause remains open.
Pinned upstream media/starboard bridge/renderer source was fetched and verified
against the pinned Git blob hashes for investigation; no arbitrary delay or
fake GL capability workaround was added.

The cold61440byte heap difference was identified as15 persistent4KiB profile/
cache file buffers owned by linked RAMFS inodes under /home/cobalt/,refs0.
A separate36608byte lazy asset index explains the pre-launch sampling difference.
No remaining new video buffer allocation was found. This identifies ownership,
not a bounded-relaunch pass; three normal non-trace launches in one VM are now
running to check cache growth and repeatability. Raw heaps and TLS keys remain
private in build. Only sanitized inode ownership summaries are public.

The old 'underruns=0' aggregate was an unmeasured placeholder and must not be
used as evidence. Source now counts actual transitions to empty audio source
while playing before EOS, reported as source-starvations. This does not measure
hardware underruns. Updated default provider passed corrupt packet/rapid seek/
real64MiB guest exhaustion/create recovery/close/relaunch/exact-resource tests
at13:44:56. Final PCM gap quality and engine32MiB RAM measurements remain open.
Native underlay test now supports --uefi. Observation runner supports bounded
MYOS_MEDIA_REPEATS=1..3,with per-launch evidence filtering. MYOS_MEDIA_HEAP_DIAGNOSTIC
and MYOS_MEDIA_KERNEL_ELF are diagnostic-only; use a matching frozen kernel ELF.
MYOS_MEDIA_QEMU_INTERRUPTS gave no CPU exception data on KVM and is not proof.
Do not reuse native-underlay-stage/build as normal: its current binary is a
fault-register diagnostic even though its source has been restored.

Final required gates remain stable normal engine,controlled-start full media
RAM accounting,measured PCM quality,EOS/restart and streaming network/truncate/
corrupt/OOM/close tests. No Phase33.3/YouTube pass and no push.

## Normal replay and engine lifecycle, 2026-10-09 14:04 +07
Three consecutive normal non-trace launches completed in the same VM:
542/544/546 video presentations, maximum measured consumed-audio vs
presented-video PTS error 17316/15230/9986 microseconds; no guest fault.
PMM, SHM and aliases returned exactly on each launch. Cold profile/cache files
consume61440 bytes; a second launch adds4096 bytes; third heap540416 before/after
is exact. Heap snapshots map retained data to linked profile/cache inodes.
See checkpoint4/normal-three-launch/ for original PCM, selected screenshots,
serial/resource metrics and ownership summaries. Earlier GP cause is unresolved;
three successful runs are evidence of replay stability, not proof of a fix.

A normal non-trace engine lifecycle session reached DOM ended at40s after
real pause/resume/seek15s, then restarted a fresh MediaSource within the SAME
Cobalt process, played again from0s, and stopped. Two native player instances
reported550/145 presentations and maximum sync52175/36092us respectively.
All PMM/SHM/aliases reclaimed on process exit. See checkpoint4/engine-lifecycle/.
Generation guards reject old request/update callbacks; stop aborts in-flight
XHR, clears src/load and revokes the old blob URL. No host media decoding.

Raw PCM diagnostics reveal long held samples (up to72.8ms in selected steady
windows) and reduced tone crossings. This is an OPEN sound-quality issue;
source-starvations=0 does not prove DMA/capture continuity or correct pitch.
Do not declare the whole phase passed from these sync counters.

Controlled-start RAM measurement is now running: page waits for B after UI
ready and3s settling; sample matching-kernel PMM free_count every0.5s during
fetch/demux/decode/render/audio/EOS/restart. Original32MiB budget remains.
Conservative accounting will add the entire16MiB reserved kernel heap and
provider mapped PT_LOAD pages to observed PMM incremental peak; this covers
underlay allocations and preload cost but sampling is not an exact high-water
bound. Final budget acceptance must state these limits or add allocation bounds.
