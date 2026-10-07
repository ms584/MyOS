# Phase 33.2: filesystem volume and audio enumeration WIP

Updated 2026-10-07 12:20 +07. Private implementation, not main promotion.
Production remains registered212 / paired214 of242 with28 missing imports.
Three real implementations below are NOT an assertion that Cobalt works.

## Implemented and scoped native evidence

- `statvfs`: pinned musl LP64 layout112 bytes, fsid at64, flags72, namemax80.
  Custom syscall58 checks path and whole writable output before copying.
  Mutable RAMFS:4096-byte units; blocks/free/available0 because it has no
  filesystem-wide block allocation/quota. Real64 inode slots/free counts;
  open/unlinked nodes stay counted. fsid1,ST_NOSUID,namemax31; reserved zero.
  Assets explicitly ENOTSUP; missing path ENOENT; output unchanged on failure.
  /dev/urandom runtime facade queries its real /dev backing volume.
  This is not disk capacity or available RAM, and not a quota implementation.
- `SbMediaGetAudioOutputCount` / `SbMediaGetAudioConfiguration`: exact pinned
 24-byte ABI; query actual SYS_SYSTEM/AUDIO/PCM_QUERY, no device acquisition.
  One analog PCM output iff native device exists; actual mono channel count1,
  unknown latency0. Invalid index/null/no-device fails without touching output.
  Native SB16 baseline mono11025Hz/8bit; NOT stereo, encoded codec or player.
- RAMFS ASAN/UBSAN volume scope fn-F7eTAc terminal0 at12:02:46.
- statvfs native BIOS statvfs-limm2qvl and UEFI statvfs-ctv7i1y6,
  terminal0 at12:09:01. Pinned ABI/layout, four launches/profile, workers,
  bad pointers/cross-page output permissions/reserved args, pinned unlink
  counts and exact resource restoration. See cd-QJYMwG/statvfs-native*.log.
- Audio native BIOS audio-output-gkwp2uy9 and UEFI audio-output-xrr1attj;
  audio-output.status TERMINAL_EXIT=0 verified12:16:51. With/without actual
  SB16, worker/relaunch/guards/exact resources. See audio-output-*.log.

## Real Cobalt blocker, NOT fixture success

Pinned core/provider +135 resources; no engine patch or assertion bypass.
Filename fix now permits actual Base64 cache and spaced config filenames.
After volume adapter, startup-diagnostic-q67aaxb0 reaches CrBrowserMain but
fails because /dev/shm is absent. Private guest-only mkdir in follow-up
startup-diagnostic-zzz_smbk (terminal0 at12:19:23) reveals real requirements:

- ftruncate /dev/shm temporary file to262144 returns EFBIG(-27): RAMFS64KiB cap.
- another4-byte file succeeds truncation, then mmap MAP_SHARED|PROT_RW
  returns ENOTSUP(-95): current immutable-asset mapping does not support it.
- Zero HTTP requests; no HTML/DOM/input/HTTPS/endurance acceptance.
- Heap125184->133376; new persistent RAMFS files versus leak not isolated.

Diagnostic runtime has25 unsupported call abort probes and traces. Those
are NEVER completed APIs or production code. Nondiagnostic runtime snapshot
contains only real statvfs adapter atop file-map WIP; no probes/traces.

## Next prerequisites / promotion

Frozen combined full-regression tree fm-prC903 excludes this new batch.
Combined baseline full terminal0 12:42:35. Clean-runtime native scopes for
these3APIs terminal0 12:22:50 (BIOS/UEFI), no diagnostic probes. This batch
still needs its own architectural full regression before promotion. Before adding
mutable mappings, implement real shared backing/lifetime/coherence, failure
rollback, unlink/close/map references and explicit truncate semantics. Merely
raising FS_FILE_MAX or copying file bytes is not MAP_SHARED support.
Remaining import groups after validated promotion: FS10, player/decode15.
No Phase33.3, no push, no phase closure based on counts or fixtures.
