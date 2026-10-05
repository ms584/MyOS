# Phase33.2 private build-matched runtime asset probe

2026-10-05 09:47 +07. PRIVATE ONLY, not installed in production or full-tested.
Purpose: remaining51 imports cannot fit resident kernel (only130 bytes remain
in the private message prototype). Do not increase256KiB cap/remove coverage.

## Measured design and native evidence

PROCESS_RUNTIME_EXTERNAL=1 removes packed runtime bytes from resident blob
(keeps one-byte label marker). Existing fixed Ring3 export addresses/raw-size
ABI remain. Generated header pins packed length and SHA256 of this exact build;
kernel reads readonly v1 asset `myos-runtime.mrt`, checks size/bounds and SHA256,
then unchanged MRT1 decoder checks raw length/CRC before any executable mapping.
Private temporary packed/raw allocations freed on failures/success. Existing
caller owns rollback if later runtime/TLS page mapping fails.
Missing/wrong identity reports diagnostic and rejects staging; no fallback to
arbitrary asset code. SHA256 pins build compatibility, not a secure-boot claim.
No asset disk writes. External mode0 retains original embedded path.

Private NcZ69d PROCESS_TESTS1 kernel233566 (vs262014 with same two message
exports), raw35168/packed28447; BSS194416. Cap unchanged262144.
Build72771 exit0 09:34:40. Native41829: BIOS all variants and message PASS;
UEFI failed09:38:25 because build_uefi.py rebuilt EMBEDDED default profile.
This failed setup is not external-runtime UEFI proof. Corrected by explicitly
passing PROCESS_RUNTIME_EXTERNAL=1 to UEFI builder (which otherwise resets only
TEST_CASE/PROCESS_TESTS/TEXT_ONLY). Native53739 exit0 09:41:16:
UEFI good/missing/wrong-length/wrong-build-valid-CRC/bad-CRC all PASS, three
relaunches each, exact mapped-byte hash and RX write fault, page recovery,
readonly image/disk hashes unchanged. Valid same-length foreign payload has a
correct recalculated CRC, so its rejection exercises compiled SHA256 identity.
Native UEFI kernel229464; no test-only code in UEFI builder by design.
Artifacts /tmp/myos-runtime-asset-image-NcZ69d/build/runtime-asset-{l01nskej,zia774lx}.
Logs private-native*.log,
private-message*.log; BIOS message-z5h8d0k3/UEFI message-djktkpwd PASS.

## Interface batch built on this budget (also PRIVATE)

Read pinned musl ifaddrs.h from Cobalt3befd332486952f6603ae3baf2a8713878d6e200
via logged curl09:38:48; x64 ifaddrs56 bytes/ifa_addr offset24.
SYSTEM_INTERFACE adds32-byte owned-copy NIC snapshot: actual RTL8139 presence,
MAC, admin/carrier flags, DHCP address/netmask/broadcast. Reserved bytes zero.
Validate reserved arg and every writable user page BEFORE copying; no guest
pointer kernel dereference, no fabricated loopback/IPv6/device.
Private getifaddrs builds AF_PACKET record and AF_INET only with real DHCP;
linked allocation cleanup/OOM output atomicity; freeifaddrs(NULL) legal.
if_indextoname uses real index1/eth0 only if present; invalid ENXIO preserves
output, null output EFAULT. No claim of IPv6/loopback support.
Build92523 exit0 09:43:14, private /tmp/myos-runtime-asset-image-rkSiv3:
test kernel233630/raw35648/packed28850. Native10049 exit0 09:45:11:
BIOS interface-oywroka5 + UEFI interface-gv9rwrh0 PASS no NIC, before/after
DHCP, QMP real link down/up, raw null/RX output/reserved rejection, name canary,
40 enumerate/free per launch, two launches/state and exact resources.
Not exported in production/import status; production still51missing, Cobalt0/5.

## Integration requirements before promotion

- Production full7164 must be terminal/checkpointed first; no shared edits live.
- Preserve normal BIOS/UEFI, explicit external profile propagation/release
  manifest, disk packaging/versioned hierarchy and generated exports identity.
  Every native application asset disk in external mode needs matching runtime;
  test fixtures must not silently use embedded mode as above.
- Extend OOM at packed/raw/map/TLS stages, malformed/oversized/truncated volumes,
  older/mismatched build, low RAM and genuine paired renderer/launcher/cancel.
- Rework runtime packaging regression for external mode: corrupt asset, not
  imaginary embedded bytes. Preserve all old exact/RX/bounds/rollback coverage.
- Scoped shared batch tests then mandatory full at integration, normal restore,
  docs/local commit. This proof alone is not production acceptance.
- Flat v1 assets still cannot package Cobalt resource hierarchy. No engine yet.

Windows work/process-runtime-asset.c, build-runtime-asset.sh,
exports-runtime-asset.py, runtime-asset-blob.asm, build_runtime_asset_probe.sh,
test_runtime_asset.py; interface sources platform-interface.inc,
system-call-interface.h, syscall-interface.c, net-interface.{c,h},
elf-interface.c/test_interface.py. COPY net when prototyping: never overwrite
a private-directory symlink that points back to the production net sources.

## Next Starboard contract fact, not implemented capability

Native audio_queue is unsigned8-bit mono11025Hz, queue64KiB/DMA two8KiB halves.
QEMU capture file2ch16-bit44100Hz is host backend conversion, NOT guest format.
SbAudioSink needs real float32->native conversion, owned device/worker callback
lifetime and hardware-consumed accounting (queue removal is not playback).
No user PCM syscall exists yet. Native PCM plumbing is33.2; codecs stay33.3.
