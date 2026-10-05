# Phase 33.1/33.2 remaining-import integration batch

2026-10-05 09:49 +07. PRIVATE prototypes only; production signal full7164 exit0
09:46:43, normal BIOS/UEFI restored/hashed. No live production image owner.
PHASE332_RUNTIME_ASSET_PROBE.md: private external runtime + interface3 native
BIOS/UEFI PASS; no production counts/integration/full. Owner requests remaining51 in one
coherent batch, scoped verification together and full at integration.

## Authoritative progress, not closure

Production fixture189/242=78.1%, paired binding191/242=78.9%,51 strong missing.
Weak TLS separately tracked. Cobalt has not executed: real acceptance0/5.
Do not promote private symbols into these counts. No33.3, no push.

## Kernel budget and verified private progress

work/pack_runtime_opt.py retains existing MRT1 format/CRC and unchanged target
decoder. Dynamic programming chooses literal runs/match prefixes, deterministic
32-candidate match search. Host46915 exit0 09:20:26: all runtime mutations,
every truncation/canaries, 1..65536 literal/overlap/periodic, ASan/UBSan PASS.
Evidence /tmp/myos-opt-pack-proof-xs1a3k6p.
Private image76923 exit0 09:23:03, /tmp/myos-optimal-image-GILrP1:
PROCESS_TESTS=1 kernel261246 vs261726, 480 bytes saved, cap262144 unchanged.
Raw34304/packed27731. Native exact mapped bytes/RX faults/relaunch/resources
BIOS pack-jwi75vyn and UEFI pack-hxviiwda PASS; corrupt CRC rejected before
execution/relaunch/no leak under BIOS. Production not changed.

work/platform-message.inc + platform-runtime-batch.c + exports-batch.py:
PRIVATE sendmsg/recvmsg stream subset, pinned musl56-byte layout and field
widths (not host size_t assumptions), one descriptor pin/one I/O per vector.
All vectors validated before I/O, aggregate overflow, partial scatter/gather,
PEEK/DONTWAIT, no stale fd reacquisition, explicit unsupported ancillary/send
destination errors. No datagram/MSG_WAITALL/SCM_RIGHTS completeness claim.
Private83788 exit0 09:27:33, /tmp/myos-message-image-qkVXyn:
kernel262014/cap262144 leaves130 bytes, raw35168/packed28447.
Native BIOS message-dim1e65_ + UEFI message-u__ccv4t PASS four relaunches each,
exact resource recovery, scatter/peek/short read, musl name-length truncation,
overflow/error output, invalid flags/objects, close while waiting + fd reuse.
Fixture user/elf-message.c/test_message.py only in private image; no new
production export or status. TCP/datagram/intr/timeout races need more coverage.

## Complete remaining51 (no name-only success stubs)

- Network10: accept, bind, listen; getifaddrs, freeifaddrs, if_indextoname;
  sendto, sendmsg, recvmsg, recvmmsg. Reuse owned descriptor readiness/cancel;
  actual interface queries and datagram ownership; server/timeout/partial/races.
- Files11: chmod, fchmod, fchown; inotify_init1/add_watch/rm_watch;
  readlink, rename, symlink, statvfs, utimensat. Consult PHASE11–13 before edits;
  persistence/snapshot and pathname/lifetime/error semantics must remain real.
- Starboard30: five audio-sink, two decode-target, ten media capability/
  budget and thirteen player imports listed in PHASE331_IMPORTS.csv. Verify
  pinned headers/callback lifetimes and engine capability paths. Native PCM
  sink plumbing is33.2; compressed codecs/playback are33.3 and remain excluded.
  Advertise no unavailable codec/DRM/player mode; documented unsupported
  results only where pinned contract permits. No successful fake handles.

## Integration dependencies and next action

480-byte compression improvement alone cannot fit all51; actual test image
with only two new prototypes already leaves130 bytes. Need a durable runtime
packaging/layout solution, not cap increase or removal of test coverage.
Audit versioned readonly asset hierarchy/resources and build-matched runtime
packaging together; fixed exported addresses/expected bytes, atomic RX mapping,
missing/corrupt/version mismatch rejection and cleanup must be proven. Preserve
existing v1 volumes/normal boot and immutable image ownership. Private runtime
offload now native-proven: PHASE332_RUNTIME_ASSET_PROBE.md. Not installed or
integration/full-tested; do not assume engine readiness.
Then complete all groups, measure linked budget, shared scoped tests and full.
Actual core needs content/fonts/ICU/trust, measured RAM/thread/TLS behavior,
HTML+JS/DOM+real input+verified concurrent HTTPS, >=60s close/relaunch evidence.
