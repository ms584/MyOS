2026-10-23 — ThinkBook Desktop/Snake visible; USB mouse motion FAIL, RAM-only; USB diagnostics checkpoint; Phase24 INCOMPLETE.
Repository /home/xytron404/myos; main; code/image source32f7b6873e719f20275b7b5284ab99edd82d537c.
Owner authorizes scoped commits/push; final docs/evidence commit follows code.

Target ThinkBook15 G3 ACL21A4/LNVNB161216; Ryzen5500U; owner8GB RAM.
Windows6256402432B usable; BIOS GQCN45WW(V1.31); UEFI/SecureBootOFF reported.
PS2 keyboardFUJ7401; mouse30fa:0301 advertises HID Boot Mouse03/01/02;
actual MyOS SET_PROTOCOL/input NOT RUN. AMD xHCI1022:1639 (two reported).
Samsung DUO MUF-32CB/APC090c:1000,32078036992B,512B sectors,SCSI/BOT.
System NVMe144d:a809 is NOT scratch; I2C touchpad/network/HDA not selected/supported.

Owner first attempt failed old loader framebuffer range; preserved physical-boot-evidence.json.
Retry photo now shows800x600 console,kernel/Shell,PMM/graphics/PIC/PIT/Ring3 boot PASS.
Two AMD xHCI controllers found, but no USB device/init success; RAM-only files.
Desktop/Snake rendering and Score5 visible; owner mouse motion FAIL. Typed-text/full app controls pending.
Current physical evidence: docs/wip/phase24/2026-10-23/desktop-mouse-evidence.json.

Implemented high GOP supervisor UC/NX alias at0x7e00000000 before ExitBootServices;
shared PML4[0] under Ring3 CR3, normal VM cannot overwrite it. Physical ABI unchanged.
CPU physical-width/overflow/span/pitch checks and visible GOP diagnostics added.
BIOS low framebuffer,800x600 BGR requirement,low kernel/stack/DMA ownership unchanged.
Existing xHCI/BOT512 prepared-data path,288KiB cap,write bounds and limits unchanged.

PASS: native ASan/UBSan framebuffer bounds/mapping/probe; old-probe negative control.
PASS: synthetic high QEMU RAM framebuffer,old-loader rejection,new console/gfx pixel
readback,three apps,supervisor/NX/UC flags and shared process CR3; not physical GPU evidence.
PASS: full default tests/test_system.py; normal image restored.
PASS: packaged BIOS/UEFI/exact USB,USB mouse/Ring3,BOT USB3/USB2 save/reboot/recovery,
UEFI DMA/graphics/PCM ownership,independent FAT/EFI/layout/output preservation.
PASS: BIOS/UEFI/USB/data byte-identical rebuilds; old frozen image unchanged;
original build/system-tests restored; new results in build/phase24-gop-2026-10-23/.
Historical incomplete harness logs retained as NOT PASS; final logs authoritative.

Selected NEW image: build/phase24-gop-2026-10-23/images/myos-usb-data.img
67108864B; SHA256 e0da9a5f2ff03e8031ab9cc1ed4655281c7b580cfde4c3a09cb58b3d62b66917.
Manifest/SHA256SUMS beside image; tracked gop-evidence.json,gop-tests.log,gop-SHA256SUMS
under docs/wip/phase24/2026-10-23/. Old phase24-usb-input image60c8a229 remains historical.
Retest guide: docs/PHASE24_GOP_RETEST_TH.md; compatibility: PHASE24_HANDOFF_TH.md.
Phase32/33 owner acceptance/manual evidence preserved; complete online audit not repeated.

Added read-only usb [0|1] snapshots/init+enumeration stages; USB behavior unchanged.
PASS focused native diagnostics/ports, BIOS+UEFI build, USB mouse+BOT regression.
Prior full system/high-GOP tests reused: no architecture/mapping/storage behavior change.
Remaining: actual typing/pointer/full three-app checks; USB device enumeration and Samsung
read/write/mandatory flush/reboot restore. Kernel/Shell boot confirmed; full acceptance incomplete.
Next: prepare diagnostic image; owner usb / usb 0 / usb 1 / disk on new image.
Guide PHASE24_USB_DIAGNOSTICS_TH.md: diagnose ports/stages before driver behavior changes.
USB has prepared16MiB custom data; RAMFS64nodes/4096B perfile, not all32GB.
Persistence blocked until USB backend available; see PHASE25_HARDWARE_ACCEPTANCE.md.
No physical USB/disk write,format,repartition or firmware change by agent.
