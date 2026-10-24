# Phase25 completion — selected ThinkBook offline profile

2026-10-24: **COMPLETE for the selected offline profile.**
Target: ThinkBook15 G3 ACL Type21A4, Ryzen5500U, owner8GB; UEFI/SecureBootOFF;
built-in PS/2 keyboard, USB30fa:0301 mouse, SamsungDUO090c:1000 selected scratch medium.
Image source6fc22f5ed84be859a8381de057b498c55b6b812c,64MiB rawMBR BIOS/UEFI;
selected actual boot is UEFI. Image SHA256 d66310b71d0ecb737941bf5a6dfb22448cf728d0211456de923a06d0effa85aa.

| Gate | Result / evidence |
|---|---|
| Firmware/loader/kernel/console/Desktop | PASS owner physical photos; Phase24 hardware preparation record |
| Built-in keyboard and external mouse movement/click/drag/release | PASS physical typing/HIDreports and owner confirmations; desktop-controls-evidence.json |
| Settings/Calculator/Snake/Shell | PASS selected checks; Calculator12+34=46 by owner report, not pictured; kernel-resident apps |
| USB save/flush/power-cycle restore | PASS gen11 Settings/SnakeBest10 retained; explicit phase25.txt=ThinkBook-Phase25-PASS restoredgen14 |
| Selected medium identity and boot content readback | PASS content audit below; original whole-prefix hash mismatch retained and explained |
| Limits/failures/recovery documented | PASS selected-profile documentation; power-loss-during-write hardware recovery NOT RUN |

## Readback discrepancy resolved by exact content audit

Owner first physical readback: marker and tail match; entire34MiB boot-prefix differs.
Owner second diagnostic captures all4 changed64KiB chunks, untruncated.
Offline reconstruction reproduces **exactly** the independent first physical prefix digest
086eb42035aec728f98c1b00af5ccb0b9673fcdf0f4c3bc24e7cf0954885b90f.
All291 changed bytes are confined to MBRdisk signature bytes440..443 and additions for
System Volume Information: three root entries (two LFN+directory), previously unused
clusters166/167/168, and their allocations in both FAT16 copies. Children WPSettings.dat
and IndexerVolumeGuid validated. Windows origin is inferred from these names/content
and the owner Windows context. This is a resolved overbroad immutable-region assumption
in the original verifier, not evidence of MyOS boot corruption.

Existing boot code, MBRpartition entries, BPB, EFI directory/file contents and pre-existing
FAT chains are byte-identical to the pinned image. Every other prefix byte is compared;
only the specifically decoded additions are excluded. No blanket FAT/ESP exclusion.
The original raw-prefix FAIL remains factual; raw byte identity after host mounting is
not claimed. Mutable MyOS snapshots are separately verified by physical cross-boot
known-file and app-value tests; never reconstruct them as if physically read here.
No USB repair/reflash/format/physical write by agent was required.

Authoritative [audit](wip/phase25/2026-10-24/prefix-content-audit.json),
[owner diagnostic](wip/phase25/2026-10-24/physical-prefix-diagnostic.json),
[first readback](wip/phase25/2026-10-24/physical-usb-readback.json),
[Shell evidence](wip/phase25/2026-10-24/shell-file-evidence.json).
Reproduce from repo root:

```sh
python3 tools/run_logged.py -- python3 tools/audit_phase25_prefix_report.py docs/wip/phase25/2026-10-24/physical-prefix-diagnostic.json docs/wip/phase25/2026-10-24/physical-usb-readback.json
```

Audit PASS; tampered payload/expectedhash/truncatedcapture/fixture-as-physical rejected;
log prefix-content-audit.log. Host tools only; previous unchanged MyOS build/native/QEMU
results reused. No architecture/runtime/image change, no repeated full-system run.

## Retained limits

No I2Ctouchpad/NVMe/RTL8168/QCA61x4A/HDA drivers, physical network/audio/YouTube acceptance,
full8GB memory support, full32GB USB use, hubs/UAS/hotplug/wheel/generalHID or hardwareBIOS
acceptance. Allocator1GiB range; data16MiB; RAMFSv2 64nodes/65536B perfile. No shutdown/reboot
shell command. Existing Phase32/33 QEMU acceptance remains a different profile.
No further owner commands required for this selected Phase25 closure.
