2026-10-23 — Phase24 INCOMPLETE; ThinkBook Desktop/Snake visible, USB mouse FAIL, RAM-only.
Real repo /home/xytron404/myos; main; owner authorizes scoped commits/push.
Diagnostic executable/image source 3482f0e6ed564e5ee27ae6af0cf82794f37f304f; docs follow-up commit follows.

Target ThinkBook15 G3 ACL21A4/LNVNB161216; Ryzen5500U; owner8GB RAM.
Windows6256402432B usable; BIOS GQCN45WW(V1.31); UEFI/SecureBootOFF reported.
PS2 FUJ7401 keyboard; USB mouse30fa:0301 advertises HID boot03/01/02.
Two AMD xHCI1022:1639; actual device enumeration/input not established.
Samsung DUO MUF-32CB/APC090c:1000,32078036992B,512B sectors,SCSI/BOT.
System NVMe144d:a809 is NOT scratch. I2C touchpad/network/HDA unsupported/outside path.

Physical: prior loader range FAIL retained; retry800x600 kernel/Shell boot checks PASS.
Latest photos: Desktop/Snake rendered, Score5 RAM-only; owner reports no mouse motion.
No exact flashed checksum/readback or successful USB HID/BOT attach evidence.
Evidence docs/wip/phase24/2026-10-23/desktop-mouse-evidence.json.
Keyboard text/full Settings/Calculator/Snake control checks still incomplete.

Added read-only usb / usb 0 / usb 1: pre-reset/scan/current root-port registers,
init/enumeration stages and counters. No speculative reset/power/driver behavior changes.
Root cause UNKNOWN pending target diagnostics. PS2 init is not USB mouse success.
PASS native ASan/UBSan diagnostics incl two hosts, bounds and zero MMIO writes;
port reset/stop regression; BIOS/UEFI builds; actual QEMU HID events/ring wrap/Ring3;
USB2/USB3 BOT save/flush/reboot/recovery/write bounds/mouse coexistence;
independent FAT/EFI/layout/output preservation; byte-identical images/kernel/runtime.
Full default system/high-GOP checks reused from32f7b68: architecture/mapping unchanged.
New logs build/phase24-usb-diag-2026-10-23/; original build/system-tests restored.
Tracked usb-diagnostics-evidence.json, usb-diagnostics-tests.log, usb-diagnostics-SHA256SUMS.

Selected NEW diagnostic: build/phase24-usb-diag-2026-10-23/images/myos-usb-data.img
67108864B raw MBR BIOS/UEFI with prepared16MiB custom data; manifest beside image.
SHA256 07809cd86d99e0022e7e9eecd4d155ab2d8db3218b055452dd92ad62ab5a1b0f.
Old GOP image e0da9a5f... and original acceptance/failure evidence preserved.
RAMFS64nodes/4096B perfile; does not use all32GB. No persistence until disk backend ready.
Guide PHASE24_USB_DIAGNOSTICS_TH.md; compatibility PHASE24_HANDOFF_TH.md.
Phase32/33 owner acceptance preserved; no unrelated audit repeated.

Next: owner backs up USB, writes diagnostic image and boots with direct mouse+Samsung;
collect usb / usb 0 / usb 1 / disk; then diagnose before behavior changes.
Remaining: physical mouse motion/buttons, full app/typing checks, Samsung READ/WRITE/
mandatory flush/reboot restore; Phase24/25 NOT COMPLETE. See PHASE25_HARDWARE_ACCEPTANCE.md.
No physical USB/disk write,format,repartition or firmware change by agent.
