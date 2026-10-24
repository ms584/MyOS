# Phase 24 — ThinkBook USB image and hardware preparation

**ปิด Phase24 — 2026-10-24:** การเตรียม ThinkBookชุดที่เลือกครบแล้ว: boot/display/input,required HID/BOT drivers,อิมเมจ/manifest/checksum/build/tests/คู่มือพร้อม. เจ้าของยืนยัน Calculatorและclick/drag/releaseปกติ; ผลCalculatorเป็นowner report ไม่ใช่ภาพผลคำนวณ. ดู [audit/ขอบเขต](PHASE24_COMPLETION.md). Phase25ยังขาดexplicitShell-file/media readback; checkpointINCOMPLETE/FAILด้านล่างเป็นประวัติ.

**ผลหลังปิดเปิดเครื่อง2026-10-24:** syncgen11 → เจ้าของปิดเปิด/เสียบUSBใหม่ → bootrestored RAMFSgen11 และdiskgen11. Settings/Best10Snakeตรงกับก่อนปิดตามเจ้าของยืนยัน. [หลักฐาน](wip/phase24/2026-10-24/samsung-reboot-evidence.json). ยังต้องCalculator/full controls/explicitShell file readback; ใช้อิมเมจเดิมต่อ. Checkpointก่อนหน้านี้ด้านล่างเป็นประวัติ.

**ผลจริงล่าสุด2026-10-24:** Samsungเปิดพื้นที่ข้อมูล USB BOT32768sectorsได้ และ `sync` รายงานSaved generation11; เมาส์เคลื่อนไหวผ่านแล้ว. ยังขาดknown-file/Settings/Snakeข้ามการบูตและfull controls. [หลักฐาน](wip/phase24/2026-10-24/samsung-sync-evidence.json). ใช้USBเดิมต่อ ไม่flashซ้ำระหว่างsave/restore; ข้อความfailure/retestก่อนหน้านี้เป็นประวัติ.

**อัปเดต2026-10-24:** เจ้าของยืนยันเมาส์เคลื่อนไหวได้; HID30fa:0301/report1849มีภาพจริง. Samsungถูกพบแต่BOT transportล้มเหลว/RAM-only. แก้CPU-iteration timeoutให้ใช้xHCI hardware clock พร้อมfailure phase; ยังต้องลองSamsungใหม่ตาม [คู่มือ BOT](PHASE24_USB_BOT_RETEST_TH.md).

**อัปเดต USB:** ภาพพอร์ตจริงยืนยันว่า one-shot scan พลาด reconnect หลัง reset; เพิ่ม bounded2s discovery/100ms stable CCS แล้ว แต่เมาส์/Samsung จริงยังต้องลองใหม่. ใช้ [คู่มืออิมเมจแก้ startup](PHASE24_USB_STARTUP_RETEST_TH.md); อิมเมจ diagnostic เดิมเป็นประวัติ.

**ผลจริงล่าสุด:** Desktop และ Snake แสดงบน ThinkBook แล้ว แต่เจ้าของรายงานเมาส์ขยับไม่ได้; ยัง RAM-only. [หลักฐาน](wip/phase24/2026-10-23/desktop-mouse-evidence.json). Phase24/25 ยังไม่ครบ; ใช้ [คู่มือตรวจ USB](PHASE24_USB_DIAGNOSTICS_TH.md) เพื่อเก็บพอร์ต/ขั้นตอนที่ติดก่อนแก้ driver. ข้อความ checkpoint ก่อนหน้านี้ด้านล่างเป็นประวัติ.


## Current checkpoint — 2026-10-23

**Phase24 INCOMPLETE. Physical Desktop/Snake rendering observed; USB mouse movement FAIL and RAM-only. Full input/apps/persistence acceptance pending.**
Earlier physical USB/UEFI attempt failed before kernel; its analysis follows.
Owner photos show MyOS loader `unsupported framebuffer range`; firmware then
reports Samsung USB boot failed. The old combined check does not reveal actual
base/pitch/size; do not assert the target address is above4GiB without new values.
High-address handling was a concrete loader/kernel gap: bounded supervisor UC/NX
mapping now covers valid high physical GOP framebuffer and survives process CR3.
New diagnostics show base,size,pitch and CPU physical width before range failures.
Source/image/results: [gop-evidence.json](wip/phase24/2026-10-23/gop-evidence.json).
[Owner photos](wip/phase24/2026-10-23/physical-boot-evidence.json),
[Thai reflash/retest guide](PHASE24_GOP_RETEST_TH.md).
Native/synthetic-high focused checks and full default system regression PASS;
packaged BIOS/UEFI/USB input/storage, UEFI DMA ownership, FAT/layout and byte-identical
BIOS/UEFI/USB/data rebuilds PASS. Previous frozen image and original system-test
evidence preserved. These software checks preceded the latest physical retry; current kernel/Shell evidence is linked above.
No physical USB/disk write or firmware change by agent; owner attempted the boot.

## Previous validated QEMU checkpoint — 2026-10-16

**Phase24 INCOMPLETE; Phase25 physical acceptance NOT RUN.**
Target: Lenovo ThinkBook15 G3 ACL Type21A4 / board LNVNB161216,
Ryzen5 5500U, owner8GB RAM, Samsung256GB system NVMe.
Owner Windows inventory identifies UEFI/SecureBootOFF, PS/2 FUJ7401 keyboard,
USB30fa:0301 mouse, AMD1022:1639 xHCI, Radeon1002:164c,
QCA61x4A168c:003e and Samsung DUO MUF-32CB/APC USB090c:1000.
The USB report gives32078036992 bytes,512B logical/physical sectors and
class08/subclass06/protocol50 (SCSI transparent/Bulk-Only Transport).
The Dell build host is separate; its inventory does not establish target support.

See [compatibility matrix, USB guide and Phase25 plan](PHASE24_HANDOFF_TH.md),
[owner report provenance](wip/phase24/2026-10-15/inventory-evidence.json)
and [physical result template](PHASE25_HARDWARE_ACCEPTANCE.md).
The roadmap gate is unchanged; no QEMU result establishes actual AMD/Samsung,
mouse boot-protocol, i8042, GOP or firmware allocation compatibility.

## Implemented preparation

- Bounded polling xHCI1.x root ports: HID boot mouse with basic3buttons/X/Y;
  PS/2 keyboard retained. No generic HID parser, hubs, wheel or hotplug enumeration.
- Dedicated supervisor UC/NX MMIO window supports firmware BARs above4GiB,
  shared under Ring3 roots; DMA pages stay owned for controller lifetime.
- Separate SCSI/BOT512 module, LUN0, READ/WRITE(10), READ CAPACITY(10),
  mandatory SYNCHRONIZE CACHE(10), strict CBW/CSW validation and finite waits
  even while filesystem operations hold IRQs disabled.
- Optional prepared USB data partition only; write bounds exclude MBR,
  BIOS loader/kernel, FAT16 ESP and data marker. Unknown/malformed layouts stay
  RAM-only. Legacy marked ATA snapshots retain priority and format/recovery.
- No NVMe/system-SSD, I2C touchpad, RTL8168, QCA61x4A or HDA driver added.
  Network/audio are outside this offline desktop preparation.

The three desktop apps (Settings, Calculator, Snake) remain kernel-resident.
Ring3 input/regression coverage uses separate isolated test applications.

## Images and repeatable build

Driver preparation image/build/test checkpoint validated in QEMU.
Image source commit: `031d4c8421662e81ed501d4fd8515c2abc8825d0`.
Selected image SHA256: `60c8a229c14991bce4bb5e2c239dcc942000d618a823dbda010e2a2a12beb2d9`.
Selected writable preparation output:
build/phase24-usb-input/images/myos-usb-data.img.
It is a **67,108,864-byte (64MiB) raw whole-disk image**, not an ISO/installer:
MBR BIOS bootstrap before LBA2048; typeEF ESP2048/65536 sectors (32MiB FAT16),
EFI/BOOT/BOOTX64.EFI; optional typeDA data69632/32768 sectors (16MiB),
MYOS-DATA-v2 dual-bank custom snapshots. Remaining capacity is not repartitioned.

Default build/myos-usb.img retains its immutable boot/RAM-only layout.
The new --with-data flag creates a fresh file exclusively and refuses to replace
an existing save image. [Build instructions](PHASE24_HANDOFF_TH.md#build-ที่ทำซ้ำได้)
cover both variants, second-build comparison and reviewed USB-write placeholders.
Source commit, toolchain, sizes, hashes and final test results are in
[driver-evidence.json](wip/phase24/2026-10-15/driver-evidence.json) and
[driver-SHA256SUMS](wip/phase24/2026-10-15/driver-SHA256SUMS).
Old build/phase24-prep and evidence.json/SHA256SUMS remain the historical
cd71e6e/da85dd1 checkpoint and do not contain these drivers.

The normal kernel exceeds the old256KiB packaging guard. Packaging now uses
the existing288KiB/576-sector stage2 envelope; linker kernel+BSS<0x80000
and low-memory reservations are unchanged. Build/tests must satisfy both guards.

## Evidence and remaining gates

Focused input including active Ring3,270 real USB reports/ring wrap,stable
owned pages,detach release and unsupported fallback passes in QEMU.
Focused USB3/USB2 BOT save/reboot restores files,Settings and Snake; corrupt-bank
recovery and boot-region preservation pass. Foreign/overlapping/unmarked/
extra-partition/4096B fixtures remain unchanged; detach fails boundedly.
Native production BOT tests under ASan/UBSan reject write bounds and malformed
CSW/tag/residue/phase/transport results without further writes.
Native asynchronous-port regression confirms WPR reads0 while PR tracks progress,
bounded timeout and failed-controller button release; old-WPR negative control fails
as expected. USB Full/Low speed class gating corrected during pre-freeze review.
Full default architectural regression PASS with normal image restored;
packaged BIOS and UEFI firmware tests, layout/output-preservation checks and
byte-identical normal/BIOS/UEFI/USB/data rebuilds PASS. Logs: build/phase24-usb-input/.
Earlier interrupted partial-system.log is not a completed regression.
Original Phase32/33 acceptance/manual recordings are preserved, not re-audited.

Owner Windows CompatibleIDs now confirm mouse30fa:0301 advertises HID boot mouse
(class03/subclass01/protocol02): [report and limits](wip/phase24/2026-10-16/mouse-evidence.json).
This is not a raw descriptor or MyOS runtime result.
Still unconfirmed: actual mouse SET_PROTOCOL(boot)/input; AMD controller handoff;
Samsung READ/WRITE/flush behavior; GOP800x600 BGR/framebuffer below4GiB;
kernel allocation0x10000–0x7ffff, owned stack/loader below4GiB,
low ISA DMA and bounded firmware map. Real target display/input/apps/persistence
must have physical evidence in Phase25. Do not downgrade those gates.

No physical USB/disk was written, formatted or repartitioned, and no firmware
setting changed. Physical write/boot requires separate owner authorization.
The owner authorized scoped local commits and GitHub push for this preparation.
