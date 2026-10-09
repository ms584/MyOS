# Phase 33.3  standard media
Updated 2026-10-09 08:31 +07. Status: OPEN; checkpoint 1 native decode only.
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

## Checkpoint 1  native standard asset demux/decode
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

## Remaining checkpoints and preacceptance envelope
2. Implement complete optional media-provider ABI, native Starboard player,
   decode target textures and actual audio sink; test callback ownership,
   invalid inputs, teardown and legacy provider compatibility. The existing
   64KiB RX runtime cannot absorb codec libraries; paired loader currently
   accepts only EGL/GLES provider getters. Design an explicit validated media
   export contract before adding that architecture.
3. Bounded sample/PCM/video queues, backpressure, shared A/V clock, pause/resume,
   seek ticket flushing/reset, EOS, stop/restart; focused fault/low-RAM tests.
4. Asset -> controlled verified HTTPS -> real Cobalt media element playback.
5. Capture >=30 seconds moving frames AND sound from the same QEMU session,
   exercise all controls, measure sync/resources, full regression, normal
   BIOS/UEFI restoration and local commit.

Initial acceptance target BEFORE playback measurements:160x90 at15fps,
mono11025Hz SB16 output from decoded Opus, <=100ms absolute presented-video
timestamp versus actually consumed-audio timestamp after preroll. QEMU512MiB;
incremental media allocation ceiling32MiB including codec state, stacks, sample/
PCM queues and decode textures. These are targets, NOT measured or passed.
Record RAM measurement method, drops, underruns and maxima before acceptance;
change this envelope explicitly if measured capabilities require it.

No Starboard player/decode-target/engine capability has been enabled by this
checkpoint. No visible video, audible playback, HTTPS media streaming, controls,
30-second session, A/V sync or RAM-budget acceptance exists yet. Phase33.3,
Phase32/33 overall and YouTube/Rickroll remain NOT PASSED. No Phase33.4 or push.
