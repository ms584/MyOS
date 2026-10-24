# Phase 24 — ThinkBook 21A4: ความเข้ากันได้ / USB / ส่งต่อ Phase 25

**ปิด Phase24 — 2026-10-24:** การเตรียม ThinkBookชุดที่เลือกครบแล้ว: boot/display/input,required HID/BOT drivers,อิมเมจ/manifest/checksum/build/tests/คู่มือพร้อม. เจ้าของยืนยัน Calculatorและclick/drag/releaseปกติ; ผลCalculatorเป็นowner report ไม่ใช่ภาพผลคำนวณ. ดู [audit/ขอบเขต](PHASE24_COMPLETION.md). Phase25ยังขาดexplicitShell-file/media readback; checkpointINCOMPLETE/FAILด้านล่างเป็นประวัติ.

**ผลหลังปิดเปิดเครื่อง2026-10-24:** syncgen11 → เจ้าของปิดเปิด/เสียบUSBใหม่ → bootrestored RAMFSgen11 และdiskgen11. Settings/Best10Snakeตรงกับก่อนปิดตามเจ้าของยืนยัน. [หลักฐาน](wip/phase24/2026-10-24/samsung-reboot-evidence.json). ยังต้องCalculator/full controls/explicitShell file readback; ใช้อิมเมจเดิมต่อ. Checkpointก่อนหน้านี้ด้านล่างเป็นประวัติ.

**ผลจริงล่าสุด2026-10-24:** Samsungเปิดพื้นที่ข้อมูล USB BOT32768sectorsได้ และ `sync` รายงานSaved generation11; เมาส์เคลื่อนไหวผ่านแล้ว. ยังขาดknown-file/Settings/Snakeข้ามการบูตและfull controls. [หลักฐาน](wip/phase24/2026-10-24/samsung-sync-evidence.json). ใช้USBเดิมต่อ ไม่flashซ้ำระหว่างsave/restore; ข้อความfailure/retestก่อนหน้านี้เป็นประวัติ.

**อัปเดต2026-10-24:** เจ้าของยืนยันเมาส์เคลื่อนไหวได้; HID30fa:0301/report1849มีภาพจริง. Samsungถูกพบแต่BOT transportล้มเหลว/RAM-only. แก้CPU-iteration timeoutให้ใช้xHCI hardware clock พร้อมfailure phase; ยังต้องลองSamsungใหม่ตาม [คู่มือ BOT](PHASE24_USB_BOT_RETEST_TH.md).

**อัปเดต USB:** ภาพพอร์ตจริงยืนยันว่า one-shot scan พลาด reconnect หลัง reset; เพิ่ม bounded2s discovery/100ms stable CCS แล้ว แต่เมาส์/Samsung จริงยังต้องลองใหม่. ใช้ [คู่มืออิมเมจแก้ startup](PHASE24_USB_STARTUP_RETEST_TH.md); อิมเมจ diagnostic เดิมเป็นประวัติ.

**ผลจริงล่าสุด:** Desktop และ Snake แสดงบน ThinkBook แล้ว แต่เจ้าของรายงานเมาส์ขยับไม่ได้; ยัง RAM-only. [หลักฐาน](wip/phase24/2026-10-23/desktop-mouse-evidence.json). Phase24/25 ยังไม่ครบ; ใช้ [คู่มือตรวจ USB](PHASE24_USB_DIAGNOSTICS_TH.md) เพื่อเก็บพอร์ต/ขั้นตอนที่ติดก่อนแก้ driver. ข้อความ checkpoint ก่อนหน้านี้ด้านล่างเป็นประวัติ.


วันที่2026-10-23 — **Phase24 ยังไม่ครบ; ลองใหม่เข้า kernel/Shell แล้ว แต่ยัง RAM-only**
ภาพก่อนหน้าแสดง `unsupported framebuffer range` จาก MyOS loader และ firmware แจ้ง boot failed; เป็นประวัติก่อนลองใหม่
ดู [หลักฐานจริง](wip/phase24/2026-10-23/physical-boot-evidence.json) และ
[คู่มือลองอิมเมจแก้ GOP](PHASE24_GOP_RETEST_TH.md). ค่า base/pitch/size จริงยังไม่ทราบ
เพิ่ม high-framebuffer mapping/diagnostics แล้ว; full system/firmware/USB/DMA/layout/repro PASS
ผลอิมเมจปัจจุบันดู gop-evidence.json; driver checkpoint2026-10-15 เก็บเป็นประวัติ ไม่แทนผลจริง
มีรายงาน Windows จาก ThinkBook จริงก่อน/หลังเสียบเมาส์และขณะเสียบ USB
เก็บ [รายงานหลังเสียบเมาส์](wip/phase24/2026-10-15/thinkbook-inventory.json)
และ [ที่มา/ข้อมูลที่ตรวจแล้ว](wip/phase24/2026-10-15/inventory-evidence.json)
แยกจาก Dell G5 5500 ที่ใช้ build

เครื่อง: ThinkBook 15 G3 ACL Type 21A4 / board LNVNB161216,
Ryzen 5 5500U 64-bit/NX, BIOS GQCN45WW(V1.31), UEFI, Secure Boot ปิด
เจ้าของระบุ RAM ติดตั้ง 8 GB แต่ Windows TotalPhysicalMemory รายงาน
6,256,402,432 bytes (5.827 GiB); ไม่ใช้ค่านี้แทนความจุโมดูลหรือเดาว่า
ส่วนต่างทั้งหมดเป็น VRAM เพราะ firmware อาจจอง RAM ได้
Lenovo ระบุ Radeon เป็น integrated/shared memory; เจ้าของรายงาน VRAM 2 GB
แหล่งตีความฟิลด์: [Microsoft firmware enum](https://learn.microsoft.com/en-us/windows/win32/api/winnt/ne-winnt-firmware_type),
[TotalPhysicalMemory](https://learn.microsoft.com/en-us/windows/win32/cimwin32prov/win32-computersystem#totalphysicalmemory)

Settings, Calculator และ Snake ยังทำงานใน kernel mode;
การทดสอบ Ring3 ใช้แอปทดสอบแยก เช่น /bin/loop ไม่ใช่การย้ายสามแอปไป user mode

## ตารางความเข้ากันได้

“รองรับ” ต้องอ่านร่วมกับขอบเขตหลักฐาน; physical acceptance ยังไม่ครบ
รายงาน Windows พิสูจน์ชนิดอุปกรณ์/การตั้งค่าที่รายงาน ไม่พิสูจน์ driver handoff

| ส่วน | สถานะ | หลักฐานและสิ่งที่ยังขาด |
|---|---|---|
| อิมเมจ USB BIOS/UEFI | รองรับ UEFI kernel/Shell จริง; BIOS เฉพาะ QEMU | ภาพเจ้าของแสดง kernel/Shell/Desktop; exact checksum/readback ยังไม่มีหลักฐาน |
| CPU Ryzen 5 5500U | รองรับในขอบเขต boot checks ที่เห็น | kernel64-bit, PIC/PIT, preemption และ Ring3 checks ผ่านในภาพจริง; ไม่ใช่การรับรอง CPU ทุก feature |
| RAM | รองรับในขอบเขต boot checks ที่เห็น | PMM/paging/heap และ above256/512MiB checks ผ่าน; physical allocator จำกัด1GiB ไม่ใช้ RAM ทั้ง8GB; ไม่ใช่ full-memory stress |
| UEFI / BIOS | รองรับ UEFI handoff จริง; BIOS ยังไม่ยืนยัน | GQCN45WW(V1.31), UEFI inventory; ภาพแสดง kernel/Shell หลัง loader; CSM/EDD/VBE บนเครื่องจริงยังไม่ทดสอบ |
| Secure Boot | รองรับเงื่อนไขการตั้งค่าที่รายงาน | UEFISecureBootEnabled=0 จึงไม่ต้องเปลี่ยน setting เพื่อเตรียม unsigned loader; signed trust chain ยังไม่รองรับ; ไม่ใช่ผลบูตจริง |
| Graphics / จอ | รองรับ console/Desktop/Snake ที่เห็น | ภาพจริง800x600x32 และ desktop/Snake; base/pitch/size ไม่ได้เก็บ; loader FAIL เดิมเป็นประวัติ |
| คีย์บอร์ดในตัว | ยังไม่ยืนยันครบ | PS/2 FUJ7401/init ตรง driver; เห็น Desktop/Snake และพิมพ์ usb/usb0/usb1 ได้จริง; full controls/checklist ยังไม่ครบ |
| USB controller | พบ AMD จริง; device enumeration ยังไม่ยืนยัน | พิมพ์1022:1639 สองตัว; ไม่มี device attach success; ภาพscanCCS0→nowCCS1/stage0 ยืนยันพลาด reconnect; แก้ startup discovery แล้ว รอลองใหม่; QEMU ไม่แทน AMD acceptance |
| เมาส์ USB ที่เลือก | รองรับ movement จริง; click/drag/release ผ่านตามเจ้าของ | เจ้าของยืนยันขยับได้หลังstartup fix; ภาพHID30fa:0301 initialized และ1849reports; เจ้าของยืนยันclick/drag/releaseปกติ2026-10-24 |
| ทัชแพดในตัว | ต้องพัฒนา หากจะใช้ | Synaptics SYNA2BA6 เป็น I2C HID, controller AMDI0010/GPIO AMDI0030; ไม่ใช้ PS/2 mouse driver และไม่ได้เลือกเป็น input path รอบนี้ |
| USB keyboard / Bluetooth mouse | ต้องพัฒนา หากจะใช้ | รอบนี้เลือก PS/2 keyboard + USB mouse; ไม่เพิ่มเส้นทางเหล่านี้ |
| USB boot storage ก่อน kernel | รองรับเส้นทาง firmware ที่เห็น | Samsung DUO MUF-32CB/APC090c:1000,512B/BOT; loader/kernel/Shell จริงถึงแล้ว; physical readback ยังไม่ให้ |
| USB storage/save หลัง kernel | รองรับselected snapshot/save/sync/power-cycle restoreจริง | Samsung090c:1000 prepared32768sectors,syncgen11 → bootrestoredgen11หลังเจ้าของปิดเปิด/เสียบใหม่; Settings/Best10ตรงกันตามเจ้าของ; explicitShell file readbackยังไม่ให้; UASไม่รองรับ |
| SSD ระบบ | ต้องพัฒนา NVMe หากจะใช้ แต่ไม่เลือกเป็น scratch | Samsung MZALQ256HBJD-00BL2, NVMe144d:a809,256060514304B/GPT; ATA PIO เดิมใช้ไม่ได้; ไม่พัฒนา write path ให้ SSD ระบบในรอบนี้ |
| Ethernet | ต้องพัฒนา | PCI10ec:8168/SUBSYS390417aa; RTL8139 driver เดิมจับ10ec:8139 จึงใช้แทนไม่ได้; นอก offline desktop acceptance |
| Wi-Fi | ต้องพัฒนา | QCA61x4A168c:003e/SUBSYS082717aa; ไม่มี native driver/firmware transport/Wi-Fi stack |
| Audio | ต้องพัฒนา | รายงาน codec HDA10ec:0257 และ AMD1002:aa01; ใช้ ID จริงแทนการเดาชื่อ codec จาก platform; SB16 ISA driver ใช้แทน HDA ไม่ได้ |

อ้างอิง platform เท่านั้น:
[Lenovo PSREF](https://psref.lenovo.com/syspool/Sys/PDF/ThinkBook/ThinkBook_15_G3_ACL/ThinkBook_15_G3_ACL_Spec.html),
[AMD 5500U](https://www.amd.com/en/support/downloads/drivers.html/processors/ryzen/ryzen-5000-series/amd-ryzen-5-5500u.html)
รุ่นสื่อยืนยันจากภาพเจ้าของและ
[Samsung catalog: MUF-32CB/APC](https://images.samsung.com/is/content/samsung/p5/sec/business/insights/catalog-download/2018/04/GB-20180425-2.pdf)
ความจุบนกล่องไม่แทนผล byte capacity/sector ของอุปกรณ์ที่เสียบจริง

## เก็บข้อมูลที่ยังขาดบน ThinkBook

คีย์บอร์ด/เมาส์/controller IDs และ UEFI/Secure Boot ได้รับแล้ว ไม่ต้องส่งซ้ำ
ได้รับ samsung-usb-info.json แล้ว: USB090c:1000, class08/subclass06/protocol50
(SCSI transparent/Bulk-Only Transport),32078036992 bytes, logical/physical512B
หลักฐานอยู่ใน docs/wip/phase24/2026-10-15/ พร้อม SHA256 ของไฟล์ต้นฉบับ
ได้รับ [รายงานโปรโตคอลเมาส์](wip/phase24/2026-10-16/thinkbook-mouse-protocol.json) เพิ่มแล้ว
CompatibleID ยืนยันว่าประกาศ HID Boot Mouse (03/01/02); [SHA256/ขอบเขตหลักฐาน](wip/phase24/2026-10-16/mouse-evidence.json)
ยังต้องพิสูจน์ SET_PROTOCOL และ input ภายใต้ MyOS จริง ไม่ต้องรันรายงานนี้ซ้ำตอนนี้
collector รุ่น schema2 เพิ่มฟิลด์เหล่านี้และ RAM module capacities โดยไม่เก็บ
serial number, MAC, UUID, instance ID หรือหมายเลขดิสก์
ไฟล์รายงาน schema1 ที่เจ้าของส่งมาก่อนหน้ายังเก็บเป็นหลักฐานเดิม

ส่วนต่อไปเป็นวิธีเก็บซ้ำเมื่อเปลี่ยนอุปกรณ์ ไม่ต้องรันซ้ำตอนนี้
นำ `tools/collect_phase24_hardware.ps1` รุ่นล่าสุดไปไว้ในโฟลเดอร์ local บน
ThinkBook เสียบเมาส์และ Samsung DUO แล้วเปิด PowerShell ในโฟลเดอร์นั้น
คำสั่งบรรทัดเดียวนี้ใช้ policy เฉพาะ process ไม่เปลี่ยน policy ถาวร:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\collect_phase24_hardware.ps1 | Set-Content -Encoding utf8 .\thinkbook-phase24.json
```

หากยังถูกนโยบายเครื่องบล็อก ให้ส่ง error ไม่แก้นโยบายถาวร
[Microsoft ระบุว่า -ExecutionPolicy ไม่เปลี่ยน registry policy](https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.core/about/about_powershell_exe?view=powershell-5.1#-executionpolicy-executionpolicy)
สคริปต์อ่านข้อมูลเท่านั้น ไม่ format/เขียน raw image/เปลี่ยน firmware
รายงาน Windows ระบุ GOP modes และพิสูจน์ IRQ/input handoff ไม่ได้

## เส้นทางบูตและงานที่เลือก

UEFI: firmware อ่าน MBR/ESP → EFI/BOOT/BOOTX64.EFI → ตรวจ NX/GOP →
จอง kernel/stack/page tables/map/DMA → ExitBootServices → PS/2 keyboard →
xHCI takeover/enumeration → USB HID boot mouse + SCSI/BOT data backend → desktop
loader ยังต้องจอง0x10000–0x7ffff, stack/loaderต่ำกว่า4GiB,64KiB ISA DMA
ต่ำกว่า16MiB และ memory map≤64KiB/256descriptors
เงื่อนไขเหล่านี้ยังต้องตรวจบน ThinkBook; Secure Boot ปิดไม่ได้พิสูจน์ว่าจองได้

ไดรเวอร์ xHCI ใช้ dedicated supervisor UC/NX MMIO window เพื่ออ่าน BAR
เหนือ4GiBได้ (พบจริงใน OVMF/QEMU ที่0xc000000000)
mapping ใช้ร่วมกับ kernel entry ของ Ring3 roots; ไม่เปิด device MMIO ให้ user
command/control waits และ event processing มีขอบเขต; DMA buffers ถือครอง
ตลอดอายุ controller เพื่อไม่ให้ hardware เขียนลงหน้าที่คืนให้ allocator แล้ว
poll ที่ PIT100Hz; จำกัด2controllers/8slotsต่อcontroller/32ports/16scratchpads
และ descriptor≤1024B; การรับข้อมูลไม่ allocate เพิ่มต่อ mouse report

รองรับเฉพาะ USB low/full/high-speed HID boot mouse ที่ต่อ root port โดยตรง
ไม่ใช่ generic HID report parser; ไม่มี hubs, hotplug enumeration, keyboard,
Bluetooth, suspend/resume หรือ power management
ใช้ basic3buttons/X/Y; wheel/extra buttons ไม่รวม
กรณีไม่พบ/ไม่รู้จัก/ถอดเมาส์ต้องคง Shell/PS2 keyboard และปล่อยปุ่มที่กดค้าง
การเสียบใหม่หลังถอดยังไม่ enumerate ใหม่ ต้อง reboot
runtime ownership handoff ไม่ใช่การเปลี่ยน firmware settings ถาวร
ไม่มี physical test หรือ disk write เกิดขึ้นในการพัฒนานี้

คง PS/2 keyboard เดิม; ไม่เพิ่ม I2C touchpad หรือ NVMe สำหรับ SSD ระบบ
storage แยกชั้น drivers/usb-storage.c (BOT/SCSI), drivers/block.c (backend)
และ drivers/storage.c (snapshot formatเดิมของ PHASE11–13)
รับเฉพาะ root-attached USB full/high/SuperSpeed, class08/subclass06/protocol50,
LUN0, READ CAPACITY(10) ที่รายงาน512B และ SYNCHRONIZE CACHE(10) สำเร็จ
จำกัดหนึ่ง prepared data medium; ไม่ใช่ FAT mount, GPT, UAS, hubs หรือ general disk driver
CBW/CSW/tag/residue/status และคำขอ512Bมีการตรวจ; transfer error หยุดใช้
controller/เก็บ DMA buffers ไว้ ต้อง reboot ไม่มีการเดาว่า flush สำเร็จ
polling มี iteration cap แม้ filesystem ปิด IRQ; ไม่เปิด IRQ กลาง snapshot atomicity
saveเป็น synchronous และอาจทำให้ UI หน่วง; ยังไม่มี latency/performance/power-loss guarantee บน Samsung จริง

เขียนได้เฉพาะ MBR layout ที่เตรียมไว้: ESP typeEF LBA2048/65536 และ
data typeDA LBA69632/32768 sectorsพร้อม MYOS-DATA-v2 marker และไม่มี
partitionอื่น; ต้องอยู่ในขอบเขตความจุจริง ไม่มี write path ไป MBR/ESP/data marker
data ใช้ dual-bank/checksum/recoveryเดิม; markerและlayoutไม่ใช่กลไกยืนยัน
เจ้าของหรือป้องกันการปลอมแปลง จึงห้ามต่อสื่ออื่นที่มี layout นี้โดยไม่ตั้งใจ
unknown/overlap/extra-partition/sector4096B คง RAM-only
ATA data ที่ทำเครื่องหมายไว้ยังมีลำดับก่อน USB; อิมเมจทดสอบจริงไม่ต้องต่อ ATA
ผล QEMU ไม่ปิด actual transport/flush/HID/GOP gate ของ ThinkBook
Network/audio ยังไม่รองรับ ไม่ใช้ผล Phase32/33 ใน QEMU เป็นหลักฐานเครื่องจริง

## Build ที่ทำซ้ำได้

source commit/checksum/สถานะผลทดสอบอิมเมจใหม่ดู
[gop-evidence.json](wip/phase24/2026-10-23/gop-evidence.json) และ
[คู่มือลอง GOP ใหม่](PHASE24_GOP_RETEST_TH.md); driver-evidence.json วันที่2026-10-15 เป็น checkpoint เดิม
ใช้ checkout ที่ code ตรง commit นั้นและ dependency เดิม; ไม่ build Cobalt ใหม่
อย่า checkout/reset ทับงานที่ยังไม่ commit; ใช้โฟลเดอร์ output ใหม่สำหรับแต่ละรอบ
คำสั่งต่อไปนี้เขียนเฉพาะไฟล์ใน build ไม่เปิด physical block device:

```bash
cd /home/xytron404/myos
git status --short
git rev-parse HEAD
mkdir -p build/phase24-rebuild
python3 tools/run_logged.py -- nice -n 10 python3 tools/release.py build > build/phase24-rebuild/build.log 2>&1
python3 tools/run_logged.py -- python3 tools/package_usb.py --with-data --output build/phase24-rebuild/myos-usb-data.img
python3 tools/run_logged.py -- python3 tests/test_release_tools.py > build/phase24-rebuild/layout.log 2>&1
```

หยุดเมื่อคำสั่ง exit ไม่ใช่0และหาสาเหตุก่อนทำขั้นต่อไป
--with-data/--output ปฏิเสธไฟล์เดิม; ห้ามลบ save image เพื่อให้ build ผ่าน
ค่าเริ่มต้น build/myos-usb.img ยังเป็น boot/RAM-only ที่ไม่แก้ไขตัวเอง
variant myos-usb-data.img เพิ่ม custom snapshot partition16MiBในอิมเมจ
64MiBเดิม ไม่ใช่การขยาย RAMFSหรือการ repartition USB จริง
packaging cap288KiBตรงกับ stage2 limit576sectorsเดิม; linkerยังบังคับ
kernel+BSSต่ำกว่า0x80000 ไม่ได้ขยาย low-memory reservation

ตรวจ byte-identical rebuild ทั้ง bootและdata variant โดยใช้ outputคนละชื่อ:

```bash
sha256sum build/myos-bios.img build/myos-uefi.img build/myos-usb.img > build/phase24-rebuild/before.sha256
python3 tools/run_logged.py -- nice -n 10 python3 tools/release.py build > build/phase24-rebuild/rebuild.log 2>&1
sha256sum -c build/phase24-rebuild/before.sha256
python3 tools/run_logged.py -- python3 tools/package_usb.py --with-data --output build/phase24-rebuild/myos-usb-data-second.img
cmp build/phase24-rebuild/myos-usb-data.img build/phase24-rebuild/myos-usb-data-second.img
```

อิมเมจใหม่ที่เลือกเตรียมสำหรับ Samsung DUO:
build/phase24-gop-2026-10-23/images/myos-usb-data.img,
raw whole-disk MBR BIOS/UEFI **67,108,864 bytes (64MiB)**,
ESP FAT16 32MiBและ custom data16MiB; EFI/BOOT/BOOTX64.EFI
Source commit: `32f7b6873e719f20275b7b5284ab99edd82d537c`
SHA-256: `e0da9a5f2ff03e8031ab9cc1ed4655281c7b580cfde4c3a09cb58b3d62b66917`
checksum/source commit/toolchain/ผลที่ผ่านต้องดู manifest ใหม่ข้างต้น
ตรวจไฟล์ที่ freeze แล้ว:

```bash
(cd /home/xytron404/myos/build/phase24-gop-2026-10-23/images && sha256sum -c SHA256SUMS)
```

build/phase24-prep/myos-usb.img และ evidence.json/SHA256SUMS เดิมเป็น
checkpoint cd71e6e/da85dd1 ก่อนเพิ่ม driver เก็บไว้เป็นประวัติ ห้ามสับสนกับอิมเมจใหม่
ไม่ใช่ ISO, ไม่ใช่ไฟล์ลากใส่ USB และไม่ใช่ installer
ไม่รวม external Cobalt/YouTube assets; ไม่มี native Wi-Fi/Ethernet/HDA
ไม่ใช้ release-validation.json เก่ามาอ้างว่า release package ปัจจุบันผ่านครบ

tests/test_usb_mouse.py และ tests/test_usb_storage.py ใช้ VM128MiB,
tests/test_usb_bot_protocol.py ทดสอบ production code กับ hostile transport
พร้อม ASan/UBSan; tests/test_xhci_ports.py ตรวจ asynchronous reset/timeout/
failed-host release โดยเปลี่ยนเฉพาะ MMIO adapter และจับ old-WPR defect ได้
full tests/test_system.py เป็น architectural gate แยก
ผลใหม่เก็บใน gop-evidence.json และ build/phase24-gop-2026-10-23/; ผลเก่าอยู่ driver-evidence.json
ไม่ตีความ VM128MiB เป็น minimum RAM ของเครื่องจริงหรือ online profile1024MiB

ทำซ้ำ focused checks โดยแยก output ใหม่ก่อน import fixtures
(รันใน WSL หลัง build BIOS/UEFI/USB แล้ว และหยุดหาก output เดิมมีอยู่):

```bash
python3 tools/run_logged.py -- nice -n 10 python3 - > build/phase24-rebuild/focused.log 2>&1 <<'PY'
import sys
from pathlib import Path
sys.path.insert(0,"tests")
import test_system
test_system.OUT=Path("build/phase24-checks").resolve()
test_system.OUT.mkdir(parents=True,exist_ok=False)
import test_usb_mouse,test_usb_storage,test_firmware
test_usb_mouse.exercise()
test_usb_storage.exercise()
test_usb_storage.exercise_usb2()
test_firmware.main()
PY
python3 tools/run_logged.py -- python3 tests/test_framebuffer_range.py
python3 tools/run_logged.py -- python3 tests/test_framebuffer_uefi.py
python3 tools/run_logged.py -- python3 tests/test_usb_bot_protocol.py
python3 tools/run_logged.py -- python3 tests/test_xhci_ports.py
```

architectural gate คือ python3 tests/test_system.py แบบ defaultเต็มชุด
รอบพัฒนานี้ใช้ wrapper เก็บ build/system-tests เดิมแยกไว้และคืนใน finally;
ตรวจ PROJECT_STATE ก่อนรันซ้ำ เพื่อรักษาหลักฐานเดิมและไม่เปิด VM ซ้อน
ผลเฉพาะ focused checks ไม่แทน full regression และไม่แทนเครื่องจริง

## เตรียม USB และเลือก boot device — สำหรับเจ้าของตรวจทานภายหลัง

**เอเจนต์ยังไม่เขียน USB จริง** เจ้าของเขียนและลองบูตเองแล้ว พบ loader range FAIL
การเขียนอิมเมจใหม่เป็นขั้นตอนเจ้าของทำเมื่อพร้อมทับข้อมูล. เลือก Samsung DUO MUF-32CB/APC
USB090c:1000,32078036992 bytes,512B sectors ตามรายงานแล้ว
ต้องตรวจว่าเป็นสื่อที่เจ้าของเลือกและยอมลบข้อมูล ขนาดอย่างน้อย 67,108,864
bytes, logical sector 512 bytes, และตรวจ source/checksum ก่อน
raw whole-device write จะเขียนทับ partition table/ข้อมูลเดิม
ไม่ format/repartition เพิ่ม และไม่เลือก SSD 256 GB หรือดิสก์ WSL แทน USB
หลังเขียนในงานที่อนุญาต Windows อาจถามให้ format custom data partition:
ให้ยกเลิก อย่ารับคำสั่ง format และอย่าขยายพาร์ทิชันเพื่อใช้พื้นที่32GBที่เหลือ

คำสั่งอ่านข้อมูลบน Linux ที่เห็น USB จริง (ไม่ใช่หลักฐานว่า WSL เห็น USB แล้ว):

```bash
lsblk -o NAME,PATH,SIZE,MODEL,TRAN,LOG-SEC,MOUNTPOINTS
ls -l /dev/disk/by-id/usb-*
```

ต้องตรวจชื่อ/รุ่น/ขนาด/whole-device symlink กับ USB ที่ถืออยู่และ unmount
ทุก partition ของสื่อนั้นด้วยตนเองก่อนการเขียนในงานที่อนุญาต
WSL block device อาจเป็น virtual disk ของระบบ ห้ามเดา /dev/sdX
หรือ Windows PhysicalDrive จากลำดับ และไม่ attach disk ให้อัตโนมัติ

ตัวอย่างนี้เป็น **comment สำหรับ review เท่านั้น ไม่ได้รัน**;
placeholder ยังไม่ใช่อุปกรณ์ที่อนุญาต:

```bash
# USB_WHOLE_DEVICE='/dev/disk/by-id/<OWNER_VERIFIED_WHOLE_USB_DEVICE>'
# sudo dd if=/home/xytron404/myos/build/phase24-gop-2026-10-23/images/myos-usb-data.img of="$USB_WHOLE_DEVICE" bs=4M conv=fsync status=progress
# sudo cmp -n 67108864 /home/xytron404/myos/build/phase24-gop-2026-10-23/images/myos-usb-data.img "$USB_WHOLE_DEVICE"
```

หลังใช้ MyOS บันทึกข้อมูล พื้นที่ data จะเปลี่ยนตามปกติ จึงตรวจ whole-image
readback ก่อนบูตครั้งแรก ไม่ใช้ checksumต้นฉบับตัดสิน USBที่มี save แล้วว่าเสีย

หลังเขียนและ readback ผ่านในงานที่อนุญาตแล้ว ปิดเครื่อง ThinkBook เสียบ USB
ที่เลือกไว้ เปิดเครื่องและกด **F12 หรือ Fn+F12** ซ้ำเมื่อขึ้นโลโก้ Lenovo
เลือก entry ของ USB แบบ UEFI ตามชื่อสื่อ ไม่เลือก SSD/Windows Boot Manager
ถ้าไม่มี entry ให้บันทึกภาพและชื่ออุปกรณ์ ไม่เปลี่ยน firmware แบบเดาสุ่ม
อ้างอิง [Lenovo คู่มือรุ่นนี้](https://download.lenovo.com/pccbbs/pubs/thinkbook_14_15_gen3/html_en/EN/change_device_boot_order.html)
ใช้เมนูชั่วคราว ไม่ต้องเปลี่ยน boot order ถาวร

Secure Boot เปิดอยู่จะบล็อก loader ที่ไม่ signed: หยุดและให้เจ้าของตัดสินใจ
เตรียมวิธีกู้คืนระบบเดิมก่อนเปลี่ยน firmware ด้วยตนเอง; ไม่มีคำสั่งปิด
Secure Boot, ลบ keys, เปลี่ยน SATA mode หรือ flash firmware ในงานนี้
บูตจาก firmware สำเร็จไม่ยืนยันว่าเมาส์/คีย์บอร์ด USB จะใช้ได้หลัง handoff

## แผนส่งต่อ Phase 25 และผลที่ต้องเก็บ

เริ่มได้หลังปิดช่องว่าง Phase 24 และเจ้าของอนุญาต physical write/boot แยก
ใช้ [แม่แบบผลจริง](PHASE25_HARDWARE_ACCEPTANCE.md) โดยเก็บ:

1. inventory ของ ThinkBook, USB identity ที่ตรวจด้วยตนเอง, image checksum,
   source/preparation commits และ firmware settings ก่อนทดสอบ
2. ผล USB readback/boot entry; รูป loader error หรือ desktop/shell หลัง handoff;
   จด GOP mode และ input transport จริง
3. ทดสอบคีย์บอร์ด/เมาส์หลังเข้า kernel: พิมพ์คำสั่ง เลื่อน/คลิก/ลากหน้าต่าง
   และการตอบสนองข้ามแอป ไม่ใช้การกด F12 ใน firmware เป็น input acceptance
4. เปิด Settings, Calculator (7+8=15), Snake (เคลื่อนที่/หยุด/เริ่มใหม่),
   ปิดหน้าต่างแล้วกลับ Shell; เก็บภาพ/เวลาจริงและอาการผิดพลาด
5. persistence ต้องใช้อุปกรณ์ scratch ที่รองรับและอนุญาตแล้วเท่านั้น
   ใช้ myos-usb-data.img กับ Samsung ที่เลือก ทดสอบ save/reboot/restore
   ไฟล์ Settings และ Snake; หาก enumerate/flush/restore ไม่ผ่านให้ลง
   BLOCKED ไม่ตีความ RAM-only เป็น persistence PASS หรือปิด full gate แทน
6. บันทึก network/audio เป็น unsupported/untested ตามตาราง เว้นแต่มี native
   driver และผลจริงเพิ่ม; การทดสอบนี้ไม่รับรอง YouTube บน ThinkBook
7. ถ้า panic, input ค้าง, จอดำ หรือ firmware ปฏิเสธ ให้หยุด เก็บหลักฐาน
   แล้วกลับระบบเดิมโดยถอด USB หลังปิดเครื่อง; ไม่พยายามแก้ด้วยการเขียน SSD

เก็บผลทุกข้อเป็น PASS/FAIL/BLOCKED/NOT RUN พร้อมหลักฐานและข้อจำกัด
มีผลบูตจริง FAIL ก่อน kernel แล้ว; kernel/input/apps/persistence ยังไม่ทดสอบจริง ห้ามเลื่อนเป็นผ่านจาก QEMU

ลองบูตจริงแล้ว FAIL ก่อน kernel; input/apps/persistence ยัง NOT RUN; ไม่ถือว่า Phase25 ผ่าน
รายการเปิดก่อนปิดPhase24เดิมได้รับหลักฐานจริงแล้ว: HIDinitialized/reports/input,AMDhandoff,
Samsungcommand/save/flush/power-cycle restore และGOP/kernel successfulhandoff.
ดูPHASE24_COMPLETION.md; ไม่อนุมานraw framebuffer addressหรือfull8GB support.
Phase25ยังไม่ปิดเพราะShell-file/media readbackไม่ได้ให้; เกณฑ์เดิมไม่ลดลง.

อ้างอิง driver protocol:
[Intel xHCI1.2b](https://cdrdv2-public.intel.com/625472/625472_xHCI_Rev1_2b.pdf),
[USB HID1.12 AppendixB](https://www.usb.org/sites/default/files/hid1_12.pdf),
[USB MSC Bulk-Only1.0](https://www.usb.org/sites/default/files/usbmassbulk_10.pdf)
