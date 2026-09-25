# Phase 24 — USB image and hardware preparation

The owner has no spare physical USB device. On 2026-09-25, the owner approved
developing Phases 26–32 in QEMU first, then completing the remaining Phase 24
hardware preparation and Phase 25 physical acceptance. Existing phase numbers
and the already validated USB image are retained. See [ROADMAP.md](ROADMAP.md).
The **USB-image/QEMU part of Phase 24 is implemented**. Selecting a real target
and implementing its required device drivers remain pending; the full
hardware-preparation gate is not claimed complete.

`build/myos-usb.img` is a 64 MiB MBR image with BIOS EDD bootstrap sectors
before LBA 2048 and a 32 MiB FAT16 EFI System Partition (type EF) at LBA 2048.
The partition contains `EFI/BOOT/BOOTX64.EFI`. The same disk boots through
SeaBIOS and OVMF when connected as **emulated USB mass storage**, without a
floppy or IDE boot image. The kernel and boot assets are embedded in both boot
paths. Tests verify actual USB boots, all three apps, partition bounds, FAT
consistency and EFI file readback. The OS image stays unchanged after use.

```bash
cd ~/myos
python3 tools/release.py build
python3 tools/run_image.py --firmware bios --media usb
python3 tools/run_image.py --firmware uefi --media usb
```

QEMU acceptance uses 128 MiB, emulated USB storage for firmware boot and
**PS/2 keyboard/mouse** for kernel interaction. There is no kernel xHCI/EHCI/
UHCI host controller, USB HID or USB-storage driver yet. Booting from USB
through firmware does not provide USB keyboard/mouse or USB persistence after
the kernel takes over. An optional separate IDE MyOS data disk works; the boot
USB itself is not the save disk.

Before physical testing, record machine/board model, firmware mode, available
GOP mode, RAM, keyboard/mouse connection, USB controller and storage controller
IDs. Select the required drivers from that evidence. No physical-device write
command or automatic device-selection tool is supplied at this stage. A later
write requires the owner to select the exact expendable device, verify its
capacity/identity and confirm erasure. Phase 25 remains deferred until real
boot/display/input/app/persistence evidence is available.
