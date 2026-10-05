# Phase 33.2 private hierarchical resource/runtime packaging

CURRENT 11:51 +07: full75305 exit0 11:49:12 ALL PASS, normal BIOS+UEFI restored.
Paired lifecycle6076 exit0 11:50:09 BIOS+UEFI PASS, source-matched c3nywg.
See latest integrated checkpoint at end for hashes; older LIVE notes superseded.

CURRENT 11:10 +07: 87856 terminal1 11:05:54, old malformed-volume test wrote
v1 offset48 into v2 name padding. Corrected test now exercises offset overflow
for BOTH formats; assertions unchanged. Normal BIOS restored by failing suite.
Full retry75305 LIVE started11:08:50: build/resource-batch-full-retry.log,
then final normal UEFI/hashes. Do not mutate live source/image. Earlier87856
pending notes below are historical. Pair-gvst4fao PASS and196/24281.0% binding
observed; actual engine still0/5. PCM separate/private: PHASE332_PCM_WIP.md.

2026-10-05 10:48 +07. Latest batch INSTALLED/uncommitted after28cdf9b.
Production build+host PASS; full pipeline87856 pending. No Cobalt execution
claim. Never push or start33.3. Historical private proofs below remain scoped.

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

## Current integration and remaining work

Sources are Windows workspace work/ mirrors; installed with logged
integrate_asset_v2_batch.sh (28 files, preserves tracked executable modes).
Latest private c3nywg native BIOS/UEFI traversal regression79676 terminal0
10:43:49; file/.. cannot bypass ENOTDIR, missing/.. cannot bypass ENOENT.
Production44994 terminal0 10:45:36 build237728/raw36224/packed29307 and permanent
tests/test_asset_format.py actual kernel ASan/UBSan+independent frozen v1 wire
bytes PASS. New four native fixtures +host parser wired into ALL/apps suite;
adapted runtime packing retains existing host and native coverage.
Pipeline87856 started10:46:59: actual pair then ALL full then final normal UEFI
hashes, pending. No source/image mutation while live. Pair_execution test still
has old missing51 assertion; adjust after observed pair count and rerun scope.
build_asset_v2_probe.sh creates private source copy including drivers/fs/net/
memory (do not symlink mutable directories). Native fixture elf-asset-v2.c and
test_asset_v2.py; parser asset-v2.c/.h; runtime platform-runtime-batch.c;
packaging make_asset_disk_v2.py/exports-runtime-asset.py/build-runtime-asset.sh.
Other files: syscall-interface.c/system-call-interface.h, ata-asset-v2.c,
ramfs-asset-v2.c, process-runtime-asset.c/runtime-asset-blob.asm, message/interface
includes and optimal MRT1 packer. See PHASE332_RUNTIME_ASSET_PROBE.md for prior
identity tests and PHASE331_REMAINING_BATCH.md for remaining51 contracts.

Finish pipeline87856, required pair execution/profile scopes, normal BIOS/UEFI
restore, measured actual binding count, local source commit and state update.
Remaining46 after five real exports are promoted still require contracts;
all242 symbols do not close phases. Actual Cobalt acceptance remains0/5.

# Latest integrated checkpoint — 2026-10-05 11:50 +07

Full retry75305 exit0 11:49:12 ALL architectural PASS, including new v1/v2
malformed volume fixture, host ASan/UBSan parser, native resource identity,
message/interface and existing signals/DNS/TLS/thread/fault/low RAM/VGA scopes.
No assertions removed. Original87856 failure was stale fixture offset48 vs240.
Normal BIOS+UEFI restored. Kernel229560, runtime36224, packed29307 bytes.
Paired lifecycle6076 exit0 11:50:09 source-matched BIOS+UEFI PASS, not Cobalt.
Private logs private-resource-pair-execution.log/-uefi.log in c3nywg, output
pair-exec-sewlmxgn and pair-exec-kprxw40l. Actual worker EGL/GLES pixels/TLS,
reverse fini/fault/SABI-before-init/cancellation/relaunch/exact resources.
Logs build/resource-batch-full-retry.log and build/resource-batch-final-uefi.log.
SHA256 normal build:

```text
715b23f674298ae9dad14388afdf9a008e4c8a86e8ba30cde38c7f7b0d319eee kernel.bin
58bc8ac305c220442217eacec56b2468b40d8f10045bdfac4cfaf62ea269cdba platform-runtime.bin
e941344449e412588e5b48e7a8521313dba79b691b897eaeed977296f9ef4064 platform-runtime.mrt
d404e027ec3a575539cd5e5affc10a9243521a8334a6c9ea5fea70988fe778fd platform-runtime.manifest.json
fbce04ec33895e0586eff9febe2959d5915a3af3f618824842ba6be2510c0322 myos-current.img
fd55f24666f3afef92a6475bf3f2a3d7bdf36e4af08018ba516b245226e9ce16 myos-uefi.img
```

Earlier LIVE/full-pending notes above are historical. No main image owner now.
Imports194/242 registered/196/242 paired,46 missing; actual Cobalt0/5.
Private five sink imports NOT promoted. No33.3/push/phase closure.
