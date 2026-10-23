2026-10-23 — Phase24 INCOMPLETE; USB startup reconnect race corrected, physical retest pending.
Real repo /home/xytron404/myos; main; owner authorizes scoped commits/push.
Corrected executable/image source ec0e499e34ac964106981ea4202c0a2132c14c43; docs evidence commit follows.

Target ThinkBook15 G3 ACL21A4/LNVNB161216; Ryzen5500U; owner8GB RAM.
BIOS GQCN45WW(V1.31); UEFI/SecureBootOFF; built-in PS2 FUJ7401 keyboard.
USB mouse30fa:0301 boot03/01/02; two AMD xHCI1022:1639.
Samsung DUO MUF-32CB/APC090c:1000,32078036992B,512B,SCSI/BOT.
System NVMe144d:a809 NOT scratch; I2C touchpad/network/HDA unsupported/outside path.

Physical prior boot:800x600 kernel/Shell/Desktop/Snake visible; Score5/RAM-only;
owner mouse movement FAIL. New photo confirms typed usb/usb0/usb1 commands work.
Both hosts init9/running1, all stages0/scanCCS0, command/transfer CC0; later nowCCS1.
Established: one-shot scan skipped devices reconnecting after HCRST; no enumeration attempt.
Port-to-VID/PID mapping still unknown. PS2 init/cursor is not USB mouse input proof.
Evidence docs/wip/phase24/2026-10-23/usb-startup-physical-evidence.json + photo/hash.

Correction: shared2s startup discovery per controller;100ms stable CCS before existing
reset/enumeration. Finite iteration cap if PIT stalls; stage20 indicates not stable.
Read-only usb diagnostics retained; no new power/firmware/hub/runtime hotplug behavior.
No MMIO mapping/DMA/ABI/VM/storage-format architecture changes.
PASS native ASan/UBSan delayed attach/bounce/detach/late/empty/32ports/tick wrap/PIT stall;
same delayed-device assertion rejects old one-shot scan; read-only diagnostics/port reset.
PASS actual QEMU mouse attached150ms after startup, motion/buttons BIOS+UEFI;
USB HID/ring/Ring3, USB2/USB3 BOT coexistence/save/flush/reboot/recovery/write bounds.
PASS independent FAT/EFI/layout/output preservation and byte-identical image/kernel/runtime rebuilds.
Full default system/high-GOP tests reused from32f7b68: architecture/mapping unchanged.
Logs build/phase24-usb-startup-2026-10-23/; original build/system-tests restored.
Tracked usb-startup-evidence.json,usb-startup-tests.log,usb-startup-SHA256SUMS in evidence dir.

Selected NEW corrected: build/phase24-usb-startup-2026-10-23/images/myos-usb-data.img
67108864B raw MBR BIOS/UEFI; ESP32MiB/custom data16MiB; manifest/SHA256SUMS beside image.
SHA256 0837d4e1572fccf62832313909e543119f601c652c8b1ff1660546f754e7ba1f.
Old frozen GOP e0da9a5f... and diagnostic07809cd8... images/evidence unchanged.
RAMFS64nodes/4096B perfile, not full32GB. Actual persistence still unaccepted.
Guide docs/PHASE24_USB_STARTUP_RETEST_TH.md; compatibility PHASE24_HANDOFF_TH.md.
Phase32/33 owner acceptance preserved; unrelated audit not repeated.

Next: owner backup/flash corrected image, boot with direct Samsung+mouse already attached;
check USB device/HID messages, pointer movement/buttons, usb0/usb1/mouse/disk if still failing.
Remaining: actual corrected-image mouse input, full three-app controls, Samsung READ/WRITE/
mandatory flush/reboot restore. Phase24/25 NOT COMPLETE; PHASE25_HARDWARE_ACCEPTANCE.md.
No physical USB/disk write,format,repartition or firmware change by agent.
