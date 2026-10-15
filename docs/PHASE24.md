# Phase 24 — USB image and hardware preparation

## Current checkpoint — 2026-10-15

**Incomplete.** The owner selected **Lenovo ThinkBook 15 G3 ACL, Type 21A4**:
Ryzen 5 5500U, 8 GB RAM, 256 GB SSD, QCA61x4A Wi-Fi, AMD Radeon Graphics
(owner reports 2 GB VRAM), built-in keyboard and a separate mouse.
Exact controller IDs, keyboard/mouse transports, firmware/GOP information and
USB medium identity/capacity remain unknown. The agent's Dell G5 5500 build
host is a different computer; its inventory is not target evidence.

The image/QEMU preparation is complete at this checkpoint. Required
target input/storage drivers remain unresolved; this does **not** complete
the full Phase 24 gate in [ROADMAP.md](ROADMAP.md). Phase 25 has not started.
Phase 32/33 remain accepted; their complete audit was not repeated.

See [target compatibility, USB guide and Phase 25 plan](PHASE24_HANDOFF_TH.md)
and [machine-readable evidence](wip/phase24/2026-10-15/evidence.json).

## Prepared image

- Frozen local file: `build/phase24-prep/myos-usb.img`.
- Raw whole-disk image, **67,108,864 bytes (64 MiB)**; MBR with BIOS EDD
  bootstrap before LBA 2048 and an active type EF partition at LBA 2048
  (65,536 sectors / 32 MiB FAT16 ESP). Logical sector size: 512 bytes.
- UEFI removable-media path: `EFI/BOOT/BOOTX64.EFI`; unsigned x86-64 loader.
- SHA-256: `3d23215359e35d8a65a5de61bf4fd52bdfcf84c5b81e31fcccc5717d49cfbe41`.
- Image source: `cd71e6e7ddb3728ebba3456c1c91936ab15cab44`.
  The commit containing this checkpoint changes documentation/inventory only.
- Local manifest/checksums: `build/phase24-prep/manifest.json`, `SHA256SUMS`.
  [Tracked checksums](wip/phase24/2026-10-15/SHA256SUMS) and evidence permit
  verification if ignored build outputs are unavailable after cloning.

This is the packaged desktop boot profile (Settings, Calculator, Snake,
Shell). It does not bundle the separate Cobalt/provider/media asset disk used
by the accepted QEMU online-playback profile. No physical online/audio claim
is made.

## Build and evidence

```bash
cd /home/xytron404/myos
python3 tools/run_logged.py -- nice -n 10 python3 tools/release.py build
python3 tools/run_logged.py -- nice -n 10 python3 tests/test_release_tools.py
```

The [handoff guide](PHASE24_HANDOFF_TH.md#build-ที่ทำซ้ำได้) gives the exact
isolated-output firmware test and second-build checksum procedure.
Full local logs are in `build/phase24-prep/`; concise original test logs are
tracked in `docs/wip/phase24/2026-10-15/`.

2026-10-15 checks passed:

- Normal build; independent FAT check, EFI file readback and MBR layout.
- Existing `tests/test_firmware.py`: UEFI handoff, three apps, separate
  emulated IDE save/reboot, exact BIOS and UEFI USB boots, PS/2 desktop input,
  RAM-only USB session and unchanged boot image.
- Second normal build: BIOS/UEFI/USB images byte-identical.
- All five normal artifact hashes match the accepted 2026-10-12 regression
  identity; that unchanged full regression is reused. No architecture change.
- Inventory helper: PowerShell syntax/schema/runtime smoke on the **Dell
  host only**. An exact local copy avoids UNC RemoteSigned execution failure
  without changing execution policy.

The firmware fixture retains its existing 128 MiB profile and runs one VM
at a time. It is not a new minimum-RAM claim or the 1024 MiB online profile.
Old `build/release-validation.json` is not refreshed or presented as valid;
no complete new release ZIP/package acceptance is claimed.

## Open gate

UEFI currently needs 800x600 BGR GOP below 4 GiB, allocatable kernel pages at
0x10000–0x7ffff, an owned stack below 4 GiB, low ISA DMA allocation and a
bounded memory map. Kernel startup requires an answering i8042/PS/2 keyboard;
a missing controller/ACK can panic. Native USB host/HID/storage, I2C-HID,
AHCI/NVMe, QCA61x4A Wi-Fi, RTL8111-family Ethernet and HD Audio drivers are
absent. Firmware USB boot support ends at ExitBootServices and cannot supply
kernel USB input/storage. Secure Boot is unsupported.

Next: collect the ThinkBook inventory and USB/mouse identity, select and
implement only its necessary input/storage drivers, then rerun affected tests
and close Phase 24 against the unchanged roadmap. Real boot/display/input/app/
supported-persistence evidence belongs to Phase 25. No physical device was
written, formatted or repartitioned; no firmware settings were changed.
