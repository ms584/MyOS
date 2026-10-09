# Phase33.3 checkpoint6: bounded batches and exact PMM peaks
Status OPEN. No Phase33.4/YouTube work or claim. No push.

## Player changes
The single-sample callback cadence starved actual SB16 PCM despite the old
source-starvations counter staying zero. Bounded incoming audio8/video4 batches
allow the pinned engine to fill decoded queues without a callback round trip
for every packet. PCM32768 samples, four decoded video slots, max packet256KiB
and worker stack remain unchanged. Incoming payload maximum3MiB; metadata is
copied, payload ownership released exactly once on decode/failure/seek/destroy.
Native full-batch corrupt VP8/Opus ownership, repeated immediate teardown,
seek and actual64MiB guest exhaustion/create recovery pass twice, exact resources.

An intermediate audio8/video1 candidate FAILED324.012ms sync and is excluded.
The audio8/video4 candidate had600decoded/no drops/max47.491ms continuous40s.
Production provider-i8xbwfmz is the promoted source, not the isolated candidate.

## Production uninterrupted engine session
production-continuous/ is the actual pinned Cobalt in MyOS session using verified
HTTPS/MSE, native WebM demux, native VP8/Opus, native underlay and SB16.
Host only served original bytes and captured guest hardware; no host decoder.
Captured binary frame identities38..599 prove37.4s moving video, measured14.948fps.
600decoded,no drops/corruption;598 sync observations,max17.890ms vs100ms.
Actual native11025Hz monoU8 hardware capture40.062s:
440/660/879/550 positive crossings in source440/660/880/550Hz windows.
Three short constant PCM gaps occur during preroll before0.465s; none afterward.
Five initial hardware underruns must not be described as zero underruns.
Selected PNGs and original WAV are from this same session.
tools/analyze_media_session.py validates this session, not the whole phase.

## Exact bounded allocator history
Every successful physical allocation records the current10ms tick low-water,
512 slots/5.12s. Read-only QMP snapshots overlap their START ticks, so allocations
concurrent with a snapshot remain covered by the next one. Coverage gaps are
reported and the session analyzer rejects them. No memory writes or reset API.
A real guest transient16MiB allocation/free probe passed twice:11991 pages before
and after,7828 pages minimum retained after free. PMM cleanup is exact.
Kernel BSS end0x71ff8 stays below0x80000; metadata uses8KiB fixed boot reservation.

Production history85 rows all covered, max gap230ticks <512. Physical peak delta
26124288B; conservative heap increment606176B (runtime highwater minus baseline
live, including earlier runtime peaks); page-rounded preloaded media provider
delta372736B versus accepted renderer. Total bound27103200B <33554432B.
Includes native engine/MSE queues/stacks/decoder/texture/underlay allocations.
The unchanged boot-reserved heap is counted by its incremental used bytes.
PMM/SHM/aliases return exactly; explicit shell mem responds after exit.
Cold61440B profile/cache retention is linked-file ownership audited checkpoint5.

## Native asset and regression
production-asset/ rerun passed controls/EOS/restart/stop, measured87.883ms sync,
actual captured tones>=30s, changing decoded textures, exact samples/resources.
First concurrent asset run FAILED120.959ms and is retained separately, not passed.
It ran alongside Cobalt, focused fault QEMU and full regression; the successful
rerun used the same provider/image with fewer competing VMs. Host contention is
a possible explanation, not a proved cause or reason to discard the failure.
Real engine acceptance is separately supported by the uninterrupted session above.

Full6 PASS15:44:02 covers the preceding MRT/heap architecture; focused MID/HIGH
pthread and native-underlay UEFI pass15:40/15:47. Full7 for PMM history ACTIVE.
Final production controls/relaunch and adverse HTTPS/OOM runs are pending.
Latest normal BIOS frozen identity is included; final normal UEFI restoration pending.
Do not call this checkpoint complete Phase33.3.

Public serial copies trim trailing whitespace only; original private logs remain unchanged.
