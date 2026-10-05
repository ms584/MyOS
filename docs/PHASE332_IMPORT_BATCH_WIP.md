# Phase 33.1/33.2 — remaining import batch

Updated 2026-10-05 14:36 +07. No Phase33.3 work or pushes.

## Production scope complete — 14:59 +07

77672 terminal0 PASS14:58:33. Native message/media BIOS+UEFI, actual core/provider
paired BIOS+UEFI adversarial/rollback/repeat/exact pages/heap PASS. Outputs:
message-1bygatjr/message-s2p0rpiq, media-policy-4rr6sgzg/media-policy-pv1kwerq,
pair-2qmtr66b/pair-x3t_38lw. Normal images retained; no live image owner.
Actual211/24287.2%, registry209/24286.4%,31 missing. Local source checkpoint.
Current full evidence is audio baselinef895303 only; later new ten-export source
has the scoped proof above, not a claim that its entire ALL suite was rerun.

Normal SHA256 observed14:58:33:
- kernel.bin: d3dd4cae2363effd194292421cd2a44c993a8b3ac8223117cfae1dab787ddb90
- platform-runtime.bin: 156d7c79b86695e9c3bc506a270f7086fe95510e0f752ad96a12980bffc8a59c
- platform-runtime.mrt: 3648c436278c6fda1bab5672a8a1a93eda20db141568c528b31e90043785128c
- platform-runtime.manifest.json: b4394cef482aaa6918f9fe6ffe6d2650318ee969eb9be8a6ae2b876478bffaed
- myos-current.img: 9d763652977b732d2b9ca321e822619260d021c08b0f14d2556a4cad2292ad6d
- myos-uefi.img: db154f0ac50a84ffc2de65c961000ff62bbde4a346ee1abc50974badb752a9ed

## Production promotion — 14:54 +07

Audio source checkpointf895303 committed14:50:15, clean before promotion.
Ten-export batch installed14:51:38 via integrate_import_batch.sh, pipeline77672
LIVE. Main normal BIOS+UEFI built; native media/message BIOS PASS. Actual main
core paired serial14:53:33 missing31/provider0 =>211/24287.2%; registry209/242
86.4%. Entire main paired/UEFI scopes pending; freeze source/images until
terminal. Logs build/import-batch-{build,uefi-build,message-*,media-*,pair-*}.log.
Full58842 applies to AUDIO baselinef895303, not later ten-export source. These
new changes are user runtime/SDK/fixtures/export register only, no new kernel
architecture. After required scoped/paired results, record normal hashes and
local commit. Accidental executable mode on this doc corrected to644.

## Production versus private evidence

- Production HEAD7211c2a; source baseline51d4100. Owned PCM/audio source
  uncommitted. Production paired201/242 (83.1%), missing41, Cobalt acceptance0/5.
- Production full58842 terminal0 PASS14:43:40, normal BIOS+UEFI restored.
  Paired execution40404 terminal0 PASS14:48:20 BIOS+UEFI; final hashes unchanged.
  Logs build/audio-batch-{pair,full,final-uefi}-service-recovery.log.
- Private copied image /tmp/myos-media-policy-cB3bQ1 adds ten exports. Actual
  pinned Cobalt+genuine Mesa provider serial14:35 reports missing31/provider0,
  i.e.211/242 (87.2%) binding. Entire paired adversarial scope58654 PASS14:35:37.
  This does NOT execute Cobalt and is not a production/phase completion count.

## Eight media policies — BIOS+UEFI native PASS

Actual SHA-checked pinned API18 media.h/drm.h declarations; configuration header
must precede media.h because drm.h requires SB_C_FORCE_INLINE. Initial compile
failure retained in private-media-build.log, fixed header order only.
SbMediaCanPlayMimeAndKeySystem reports NotSupported: no demux/decoder/player.
SbMediaCanChangeType reports false: no active player transitions.
Audio/video GC budgets derive from actual SbSystemGetTotalCPUMemory, RAM/64
capped4MiB and RAM/16 capped16MiB, respectively. Negative dimensions reject;
documented unknown zeros and INT_MAX dimensions do not overflow. Allocation
unit/initial capacity0 use documented direct malloc, pool-on-demand true,
64-bit GC duration170s. No false codec, DRM or usable-memory promise.
Native64/256MiB, four launches/profile, four workers64 checks, exact heap/pages
and unchanged readonly asset disks PASS14:21:07. Joint message/media image
BIOS+UEFI PASS14:32:01. Logs private-media-native{,-uefi}.log and
private-media-joint{,-uefi}.log. Eight exports only, not media playback.

## Two socket exports — BIOS+UEFI native PASS

sendto uses existing connected TCP/AF_UNIX stream send path with one pinned
descriptor. Explicit destination rejected EISCONN; no UDP routing claim.
recvmmsg matches musl x64 msghdr56/mmsghdr64, returns per-message lengths,
partial completed count, nonblocking EAGAIN, WAITFORONE, shared monotonic caller
timeout and earlier SO_RCVTIMEO. One original-generation pin spans the whole
vector; internal receives retain that original reference, never reacquire a
reused fd. Sequence sampled before receive avoids lost wakeups. Errors reject
unsupported flags/counts/invalid timespec without fake success.
Native BIOS+UEFI four relaunches and exact resources PASS14:32:01, including
scatter/peek, vector overflow/error checks, close/reuse while waiting, timeout,
WAITFORONE leaving later result fields untouched, shorter socket timeout.
Logs private-message-native{,-uefi}.log, message-56nmbi2r/message-3xg8s1gt.
No UDP/ancillary/MSG_WAITALL/full POSIX/conformance claim.

## Exact next work

1. Baseline58842/40404, private58654 and production77672 complete; no live owner.
2. After required full/audio pair-execution scope PASS and normal BIOS+UEFI
   restoration, create the audio local checkpoint. Promote tested private
   media/message sources+headers+fixtures, update import register and actual
   paired assertions from measured evidence, then affected regressions.
3. Remaining31: accept/bind/listen3; file11; media audio-output/config2;
   player13 and decode-target2. Player preferred-mode contract requires best
   effort valid output; do not fabricate player handles/frames or claim
   empty getters satisfy functioning media. Compressed codecs remain33.3.
4. Actual native Cobalt HTML/JS/DOM/timers/input/verified concurrent HTTPS,
   >=60s and close/relaunch/resource acceptance still0/5; phase remains OPEN.

Windows work mirrors: platform-media-policy.c, elf-media-policy.c,
build-media-policy.sh, exports-media-policy.py, test_media_policy.py,
platform-message-batch.inc, elf-message-batch.c, test_message_batch.py,
test_elf_pair_media.py. Repro scripts run_media_policy_retry.sh,
run_message_batch.sh, check_media_pair.sh; logged through tools/run_logged.py.
Private build contains copied mutable sources, not symlinked production code.
