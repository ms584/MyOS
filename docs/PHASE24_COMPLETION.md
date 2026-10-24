# Phase24 completion — selected ThinkBook offline hardware preparation

Update: Phase25 Shell known-file cross-boot proof nowPASS (generation14,2026-10-24); physicalmedia readback nowFAIL in boot-prefix (marker/tail match), unresolved cause; other PASS evidence retained. See PHASE25_USB_READBACK_TH.md. The earlier remaining-check audit below records the closure checkpoint.

2026-10-24: **COMPLETE for the selected profile. Phase25 is still incomplete.**
Source/image: `6fc22f5ed84be859a8381de057b498c55b6b812c`.
Image `build/phase24-usb-bot-clock-2026-10-24/images/myos-usb-data.img`:
67,108,864bytes; SHA256 `d66310b71d0ecb737941bf5a6dfb22448cf728d0211456de923a06d0effa85aa`.
Manifest/checksums beside the frozen image; source/docs/evidence inGit, build artifacts local.
This closure changes documentation only; executable and image remain unchanged.

## Gate audit

The original Phase24 definition (a7a0137:docs/PHASE24.md/ROADMAP.md) left target
selection, required drivers and hardware preparation open after virtual USB boot.
The current handoff also required actual HID/AMD/Samsung/GOP/loader evidence.
Those driver gaps are now closed on the selected target; Phase25-specific remaining
media/readback/content checks are retained below, rather than marked PASS.

| Preparation requirement | Evidence and limit |
|---|---|
| Target/board/firmware/CPU/RAM/controller/device identities | ThinkBook15G3ACL21A4/LNVNB161216,Ryzen5500U,owner8GB,BIOSGQCN45WW(V1.31),UEFI/SecureBootOFF; owner inventory in wip/phase24/2026-10-15/ and2026-10-16/. Build host Dell is separate |
| CPU/RAM/UEFI/GOP path | Physical800x600 kernel/Shell/Desktop,PMM/paging/heap/PIC/PIT/Ring3 checks in kernel-shell-evidence.json. GOP/low-memory ownership guards execute before successful handoff. Raw framebuffer base/pitch not supplied; no claim every GOP mode or all8GB allocation works |
| Required input/controller drivers | AMD1022:1639 startup reconnect fixed; actual30fa:0301 HID initialized and1849reports; owner movement andclick/drag/release confirmation. Built-inPS2FUJ7401 Shell typing works; I2C touchpad not selected |
| Required storage driver | Samsung090c:1000 BOT512 prepared16MiB data; actualsyncgen11,owner power-cycle/USB reinsertion,bootrestoredRAMFSgen11,Settings andSnakeBest10 same asbefore. No systemNVMe write path |
| Desktop andthree apps | Shell/Desktop/Settings/Snake photographs; Calculator12+34=46 andpointer controls PASS byowner report. Calculator result is not shown inlatestphoto; provenance explicit |
| USB image/build/size/checksum/source | Frozen64MiB raw MBR BIOS/UEFI image;ESP32MiB/custom data16MiB,manifest andSHA256SUMS; byte-identical rebuilds inbot-clock-evidence.json. Physical BIOS target boot not claimed (selected UEFI); BIOS remains QEMU validated |
| Necessary tests/regression | NativeASan/UBSan reset/startup/bulk-clock/BOT negative controls; BIOS/UEFI USB mouse/rings/Ring3; USB2/USB3 BOTsave/flush/reboot/recovery/bounds; FAT/EFI/layout/repro PASS. Fullsystem/highGOP earlier results reused for unchanged architecture; source/log hashes inbot-clock-evidence.json |
| Compatibility/limits/owner USB procedure/Phase25 handoff | PHASE24_HANDOFF_TH.md,PHASE24_USB_BOT_RETEST_TH.md,PHASE25_HARDWARE_ACCEPTANCE.md. No agent physical write/format/repartition/firmware change; owner performed media writes/boots |

Evidence chain in `docs/wip/phase24/2026-10-24/`:
`bot-physical-evidence.json` → `bot-clock-evidence.json` →
`samsung-sync-evidence.json` → `samsung-reboot-evidence.json` →
`desktop-controls-evidence.json`. Original failures and frozen images remain intact.

## Limits and Phase25 remaining

Selected offline profile only. No I2C touchpad/NVMe/RTL8168/QCA61x4A/HDA driver;
no physical network/audio/YouTube acceptance inferred from QEMU Phase32/33.
Allocator supports its documented1GiB physical range; not all installed8GB.
Root-attached boot HID/BOT only; no hubs/UAS/runtime hotplug/wheel/general HID.
RAMFSv2 has64persistent nodes and65536bytes/file; does not use full32GB USB.
Settings/Calculator/Snake are kernel-resident; Ring3 probes are separate.
No shutdown/reboot command; firmware/power actions remain owner controlled.

Phase25 remains incomplete pending its explicit Shell known-file content readback
across boot and physical image/media readback evidence. Do not reflash the existing
USB for persistence checks. After legitimate saves, compare immutable boot regions
or retain initial write-validation evidence; a full-device checksum is expected to
differ from the pristine image because the data partition changed. Never guess a
physical disk number or format/repartition to obtain a matching checksum.

Operator reports are valid provenance, identified separately from visible photos.
This document does not turn unprovided readback or arbitrary-file results into PASS.
