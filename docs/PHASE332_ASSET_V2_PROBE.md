# Phase 33.2 private hierarchical resource/runtime packaging

2026-10-05 +07. PRIVATE implementation; production remains b121452.
No new production imports or Cobalt execution claim. Never push or start33.3.

## Design

Asset v2: 48-byte header, up to1024 sorted256-byte records, names239 bytes,
implicit directories, binary file lookup, validated nonoverlapping extents.
Separate hidden runtime extent, build-generated raw/packed sizes and SHA256;
unchanged MRT1 decoder/CRC, bounded temporary allocations, RX after validation.
Generated manifest lets fixture packager attach the exact runtime automatically.
No arbitrary replacement runtime, no raised kernel cap, no removed test coverage.
Asset-v1 remains readable and explicit v1 writer encoding remains byte-identical.
External runtime apps require a matching v2 bundle; v1 shell compatibility is
not proof of external-runtime execution from a v1 disk.

Kernel ATA discovery recognizes both versions; readonly write guard unchanged.
Immutable mount catalogue allocation is volume-owned, not per-app leaked memory.
New SYSTEM_ASSET_ENTRY7 validates request24/output256 including every page;
long names reach pinned musl dirent280 without legacy metadata truncation.
Relative openat/fstatat pin immutable directory path while taking its snapshot;
absolute path ignores dirfd, invalid/closed fd EBADF, nondirectory ENOTDIR.
Existing syscall absolute path bound127 bytes remains; this is not full POSIX
filesystem semantics or proof all actual Cobalt resource names fit.

## Evidence

Host ASan/UBSan PASS10:23:34 /tmp/myos-asset-v2-host-n3b1hoem:
actual kernel parser, namespace/runtime/large read,1024 entries, OOM recovery,
invalid magic/count/names/offsets/overflow/runtime overlap/nonadjacent ancestor
collision; writer exclusive-create/invalid paths and original v1 bytes.

Private /tmp/myos-asset-v2-image-Nhvb5U:
BIOS/UEFI namespace native PASS10:12:15; joint runtime identity/message/interface
PASS10:16:49. This predates long-name syscall/relative path changes.
Private /tmp/myos-asset-v2-image-sHPEXl:
BIOS/UEFI long-name/pointer ABI PASS10:23:08; added relative-open check FAILED
10:27:28 status27. Cause: openat only accepted AT_FDCWD, not resource corruption.
Failure retained; do not present earlier weaker fixture as relative-open PASS.

Latest image /tmp/myos-asset-v2-image-Ts69oq, session56426 terminal exit0
10:34:42, BIOS and UEFI PASS:
test kernel237728/raw runtime36064/packed29212; cap262144 unchanged.
BIOS PASS by10:34:13: four native launches, exact resource restoration,
77-character name enumerate/open relative/fstatat, absolute invalid-dirfd,
parent traversal, closed/nondirectory errors,65537-byte large I/O, readonly,
output crossing protected page rejected atomically. Native v1 shell/read/hash
compatibility PASS both profiles. Logs private-native.log and
private-native-uefi.log. No live image owner after terminal exit.
Latest joint scope session36452 terminal exit0 10:37:57: BIOS/UEFI all five
runtime identity cases, message four relaunches, interface no-NIC/DHCP/carrier/
errors/exact resources PASS on Ts69oq after relative-path fix. Logs
private-joint-{runtime_asset,message,interface}-{bios,uefi}.log.
Adapted test_runtime_pack_v2.py preserves host decoder ASan/UBSan boundaries,
mapped exact bytes/RX, relaunch/resources and private corruption. External
profile mutates only private v2 runtime extent (both BIOS/UEFI); embedded mode
retains private BIOS-image path. Session50615 terminal exit0 10:39:22, BIOS/UEFI
host ASan/UBSan boundaries +exact bytes/RX/corruption/relaunch/resources PASS.
Logs private-pack-v2-{bios,uefi}.log. No live private image owner now.

## Integration still required

Sources are Windows workspace work/ mirrors; authoritative repo unmodified.
build_asset_v2_probe.sh creates private source copy including drivers/fs/net/
memory (do not symlink mutable directories). Native fixture elf-asset-v2.c and
test_asset_v2.py; parser asset-v2.c/.h; runtime platform-runtime-batch.c;
packaging make_asset_disk_v2.py/exports-runtime-asset.py/build-runtime-asset.sh.
Other files: syscall-interface.c/system-call-interface.h, ata-asset-v2.c,
ramfs-asset-v2.c, process-runtime-asset.c/runtime-asset-blob.asm, message/interface
includes and optimal MRT1 packer. See PHASE332_RUNTIME_ASSET_PROBE.md for prior
identity tests and PHASE331_REMAINING_BATCH.md for remaining51 contracts.

Integrate scoped fixtures into full runner without losing CRC/exact bytes/RX/
rollback coverage. All private latest checks terminal; production still unchanged.
Then promote coherent tested batch, mandatory full regression, normal BIOS/UEFI
restore, measured actual binding count, local commit and state update.
Remaining46 after five real exports are promoted still require contracts;
all242 symbols do not close phases. Actual Cobalt acceptance remains0/5.
