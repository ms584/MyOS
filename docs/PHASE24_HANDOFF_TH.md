# Phase 24 — ThinkBook 21A4: ความเข้ากันได้ / USB / ส่งต่อ Phase 25

วันที่ 2026-10-15 — **Phase 24 ยังไม่ครบ; Phase 25 ยังไม่ได้ทดสอบบนเครื่องจริง**
เครื่องเป้าหมายยืนยันจากภาพและคำตอบเจ้าของ ไม่ใช่ Dell G5 5500 ที่ใช้ build
ข้อมูลที่ยืนยันแล้ว: ThinkBook 15 G3 ACL Type 21A4, Ryzen 5 5500U, RAM 8 GB,
SSD 256 GB, QCA61x4A, Radeon Graphics (เจ้าของรายงาน VRAM 2 GB),
คีย์บอร์ดในตัว และเมาส์แยก
Lenovo ระบุ Radeon เป็น integrated/shared memory; ไม่ตีความตัวเลข 2 GB
ว่าเป็น VRAM แยกหรือเป็นหลักฐานว่า MyOS ใช้ GPU acceleration ได้

## ตารางความเข้ากันได้

“รองรับ” ในแถวอิมเมจหมายถึงผลตามขอบเขตที่ระบุเท่านั้น
ยังไม่มีแถวฮาร์ดแวร์ของ ThinkBook ที่ผ่านการทดสอบจริง

| ส่วน | สถานะ | หลักฐานและสิ่งที่ยังขาด |
|---|---|---|
| อิมเมจ USB BIOS/UEFI | รองรับ เฉพาะ QEMU | build + FAT/MBR/EFI readback + BIOS/UEFI USB boots ผ่าน; ดู evidence.json และ firmware.log |
| CPU Ryzen 5 5500U | ยังไม่ยืนยัน | เจ้าของยืนยันรุ่น; AMD ระบุระบบ 64-bit; MyOS ต้องใช้ x86-64/NX และ legacy PIC/PIT; ต้องยืนยัน NX/บูตบนเครื่องนี้ ไม่อนุมานจาก QEMU |
| RAM 8 GB | ยังไม่ยืนยัน | เจ้าของยืนยันขนาดติดตั้ง; memory/memory.c จัดการ physical pages สูงสุด 1 GiB; ไม่ได้รองรับ RAM ทั้ง 8 GB; ต้องดู usable low-memory map หลังจอง iGPU/firmware |
| UEFI / BIOS | ยังไม่ยืนยัน | boot/uefi.c และ boot/stage2.asm มีเส้นทางจริง; ต้องทราบเวอร์ชัน/โหมด firmware; ไม่สมมติว่ามี legacy CSM หรือยอมรับ MBR/FAT16 USB |
| Secure Boot | ต้องพัฒนา หากต้องคงเปิด | BOOTX64.EFI unsigned; ไม่มี signed trust chain; ถ้าเปิดอยู่ให้หยุดก่อนบูต ไม่แก้ setting อัตโนมัติ |
| Graphics / จอในตัว | ยังไม่ยืนยัน | Lenovo ระบุ integrated Radeon; MyOS ใช้ GOP framebuffer ไม่ต้องใช้ AMD acceleration สำหรับสามแอป; loader รับเฉพาะ 800x600 BGR, pitch 3200–16384, framebuffer จบต่ำกว่า 4 GiB; ไม่มี GOP dump ของเครื่อง |
| คีย์บอร์ดในตัว | ยังไม่ยืนยัน | drivers/keyboard.c ใช้ i8042 ports 0x60/0x64, IRQ1, scan-set translation; ไม่ได้อ่านจาก UEFI input หลัง ExitBootServices; ต้องเก็บ ACPI/device IDs และพิสูจน์ ACK/keypress; ไม่มี I2C-HID driver |
| เมาส์แยก | ยังไม่ยืนยัน | ยังไม่ทราบ USB/Bluetooth/อื่น; drivers/mouse.c ใช้ PS/2 IRQ12; vmmouse เป็นช่องทาง VM เท่านั้น ไม่พิสูจน์เมาส์จริง |
| USB mouse / keyboard | ต้องพัฒนา | ไม่มี xHCI/EHCI/UHCI host หรือ USB HID driver; เสียบพอร์ต USB 2-compatible ก็ไม่พิสูจน์ว่าจะมี PS/2 emulation หลัง firmware handoff |
| Bluetooth mouse | ต้องพัฒนา | ไม่มี Bluetooth transport/stack/HID; ต้องทราบอินเทอร์เฟซเมาส์ก่อนเลือกงาน |
| USB boot storage ก่อนเข้า kernel | ยังไม่ยืนยันบน ThinkBook | firmware โหลด EFI/embedded kernel ได้ใน QEMU; ต้องให้ firmware เครื่องจริงอ่าน USB/ESP นี้และตรวจ logical sector 512 bytes; ไม่รับรอง 4Kn |
| USB storage / save หลังเข้า kernel | ต้องพัฒนา | ไม่มี kernel USB mass-storage driver; boot USB ไม่ใช่ save disk; USB filesystem ไม่ใช่ MyOS snapshot store |
| SSD 256 GB / persistence | ต้องพัฒนาเมื่อใช้ NVMe/AHCI | รุ่น SSD/PCI ID/bus ยังขาด; Lenovo มี NVMe/SATA configurations; drivers/ata.c รองรับ primary legacy ATA PIO เท่านั้น ไม่ใช่ AHCI/NVMe; ห้ามใช้ SSD ระบบเป็น scratch disk |
| Ethernet | ต้องพัฒนา สำหรับ RTL8111H-CG ตาม platform | Lenovo PSREF ระบุ RTL8111H-CG; ต้องตรวจ PCI ID ของเครื่อง; drivers/rtl8139.c จับเฉพาะ 10ec:8139 ไม่ใช่ RTL8111; ไม่จำเป็นต่อสามแอป offline |
| Wi-Fi QCA61x4A | ต้องพัฒนา | เจ้าของยืนยันรุ่นและ Lenovo มี Windows driver สำหรับ platform นี้; MyOS ไม่มี driver/firmware transport/Wi-Fi stack; Windows driver ใช้แทน native MyOS driver ไม่ได้ |
| Audio | ต้องพัฒนา สำหรับ HD Audio ตาม platform | Lenovo ระบุ HD Audio ALC3287; ต้องยืนยัน controller/codec ของเครื่อง; drivers/sb16.c ใช้ ISA SB16 0x220 IRQ5 DMA1 ไม่รองรับ HDA; ไม่จำเป็นต่อ Phase 24 offline desktop |

อ้างอิงตรวจเมื่อ 2026-10-15:
[Lenovo PSREF](https://psref.lenovo.com/syspool/Sys/PDF/ThinkBook/ThinkBook_15_G3_ACL/ThinkBook_15_G3_ACL_Spec.html),
[AMD 5500U](https://www.amd.com/en/support/downloads/drivers.html/processors/ryzen/ryzen-5000-series/amd-ryzen-5-5500u.html),
[Lenovo QCA61x4A/platform driver listing](https://think.lenovo.com.cn/support/driver/driverdetail.aspx?DEditid=101259&driverID=undefined&treeid=3125457)
ข้อมูล platform ไม่ใช่ผล inventory เฉพาะเครื่อง และไม่ยืนยัน hardware support ของ MyOS

## เก็บข้อมูลที่ยังขาดบน ThinkBook

นำไฟล์ `tools/collect_phase24_hardware.ps1` ไปไว้ในโฟลเดอร์ local บน
**ThinkBook** ตรวจเนื้อหาแล้วรันจาก PowerShell โดยไม่ต้องเปลี่ยน execution policy:

```powershell
.\collect_phase24_hardware.ps1 | Set-Content -Encoding utf8 .\thinkbook-phase24.json
```

ถ้านโยบายเครื่องห้ามรัน script ให้หยุดและรายงานข้อจำกัด ไม่เปลี่ยนนโยบายอัตโนมัติ
การรันตรงจาก UNC อาจติด RemoteSigned; ใช้สำเนา local ที่ตรวจแล้ว
ไฟล์อ่านเฉพาะ model/board/CPU/NX/BIOS, boot mode/Secure Boot,
disk model/bus/size และ HardwareID ของอุปกรณ์ที่เกี่ยวข้อง
ไม่เก็บ serial number, MAC, UUID, instance ID หรือหมายเลขดิสก์
ถ้าอ่านส่วนใดไม่ได้จะรายงาน unknown; ไม่เปลี่ยน firmware หรือเขียน physical disk

ต้องเพิ่มจากเจ้าของ: USB ยี่ห้อ/รุ่น/ความจุ/ขนาด logical sector,
เมาส์ยี่ห้อ/รุ่นและ USB หรือ Bluetooth, exact MTM (ไม่ต้องส่ง serial),
และเลือกอุปกรณ์ persistence ที่ยอมให้ใช้ในอนาคต
สคริปต์ Windows ระบุ GOP modes และพิสูจน์ IRQ/input handoff ไม่ได้
ต้องเก็บ GOP/loader error จาก firmware ของ ThinkBook ภายหลัง ก่อนตัดสินใจแก้
loader หรือเลือก input/storage driver; ขณะนี้ไม่เพิ่ม driver จากชื่อรุ่นที่เดา

## เส้นทางบูตและขอบเขตการพัฒนา

UEFI: firmware อ่าน MBR/ESP → EFI/BOOT/BOOTX64.EFI → ตรวจ NX/GOP →
จอง kernel/stack/page tables/map/DMA → ExitBootServices → kernel → PS/2 init →
desktop สามแอป
loader ต้องจอง 0x10000–0x7ffff และ stack/loader ต่ำกว่า 4 GiB;
ISA DMA ต้องจอง 64 KiB ต่ำกว่า 16 MiB แม้ไม่มี SB16;
memory map buffer 64 KiB และสูงสุด 256 nonempty descriptors
เงื่อนไขเหล่านี้อาจบล็อกก่อน desktop แม้มี RAM ติดตั้งมากพอ

BIOS: firmware EDD โหลด boot sector/stage2/kernel → VBE → long mode →
kernel; ThinkBook มี CSM/EDD/VBE ครบหรือไม่ยังไม่ทราบ จึงเลือก UEFI เป็น
เส้นทางเตรียมหลัก แต่ยังไม่รับรองว่า boot ได้
GOP error จะแสดงก่อน handoff; log หลัง handoff ใช้ COM1 ซึ่งเครื่องนี้
ยังไม่ยืนยันว่ามีพอร์ตให้เก็บ จึงต้องเตรียมภาพหน้าจอเป็นหลักฐานด้วย

งานไดรเวอร์ที่ต้องตัดสินใจตาม inventory:

1. ยืนยันคีย์บอร์ดในตัวว่าเป็น i8042/PS/2; ถ้าไม่ใช่ ต้องมี driver ตรง transport
   เพราะ keyboard_init ใช้ check/panic เมื่อ controller/ACK ไม่ตอบ
2. เมาส์ USB ต้องมี host controller ที่ตรง PCI ID + enumeration + HID input;
   Bluetooth ต้องมีเส้นทางของตัวเอง; firmware legacy USB support ไม่แทน driver
3. กำหนด storage ที่อนุญาตก่อนเพิ่ม NVMe/AHCI หรือ USB mass storage/persistence
   และอ่าน PHASE11–13 ก่อนแก้ persistence; ไม่เปลี่ยน SSD ระบบเป็น data disk
4. Network/audio เก็บเป็นข้อจำกัดของการส่งต่อ offline desktop; หากต้องการ
   online/audio บน ThinkBook ต้องเตรียม native drivers เพิ่ม และผ่านเกณฑ์ใหม่
   ไม่ยืมผล Phase 32/33 ใน QEMU มาปิดช่องว่าง

checkpoint นี้ไม่เปลี่ยน kernel/boot/drivers และไม่ลดเกณฑ์ Phase 24
การเลือกเครื่องทำแล้ว แต่ required input/storage drivers ยังไม่พร้อม

## Build ที่ทำซ้ำได้

source ของอิมเมจคือ `cd71e6e7ddb3728ebba3456c1c91936ab15cab44`;
commit ที่มีคู่มือนี้เปลี่ยนเฉพาะเอกสารและเครื่องมือ inventory
เครื่องมือ/เวอร์ชัน/firmware hashes อยู่ใน
[wip/phase24/2026-10-15/evidence.json](wip/phase24/2026-10-15/evidence.json)
ใช้ dependency เดิม ไม่ build Cobalt ใหม่ ไม่ทำงานหลาย VM พร้อมกัน
คำสั่งนี้เขียนเฉพาะไฟล์ build; ไม่เขียน physical disk:

```bash
cd /home/xytron404/myos
git status --short
git rev-parse HEAD
mkdir -p build/phase24-rebuild
python3 tools/run_logged.py -- nice -n 10 python3 tools/release.py build > build/phase24-rebuild/build.log 2>&1
python3 tools/run_logged.py -- nice -n 10 python3 tests/test_release_tools.py > build/phase24-rebuild/layout.log 2>&1
python3 tools/run_logged.py -- nice -n 10 python3 - > build/phase24-rebuild/firmware.log 2>&1 <<'PY'
import sys
from pathlib import Path
sys.path.insert(0, "tests")
import test_system
test_system.OUT = Path("build/phase24-rebuild/firmware").resolve()
test_system.OUT.mkdir(parents=True, exist_ok=True)
import test_firmware
test_firmware.main()
PY
```

หยุดเมื่อคำสั่งใด exit ไม่ใช่ 0; อย่ารันคำสั่งขั้นถัดไปจนทราบสาเหตุ
fixture นี้ใช้โปรไฟล์ 128 MiB เดิมกับ PS/2 และ emulated USB; ไม่ใช่ online
profile 1024 MiB หรือการทดสอบเครื่องจริง
เมื่อผ่านแล้วตรวจการสร้างซ้ำ:

```bash
sha256sum build/myos-bios.img build/myos-uefi.img build/myos-usb.img > build/phase24-rebuild/before.sha256
python3 tools/run_logged.py -- nice -n 10 python3 tools/release.py build > build/phase24-rebuild/rebuild.log 2>&1
sha256sum -c build/phase24-rebuild/before.sha256
cp -- build/myos-usb.img build/phase24-rebuild/myos-usb.img
(cd build/phase24-rebuild && sha256sum -c ../../docs/wip/phase24/2026-10-15/SHA256SUMS)
```

อิมเมจ checkpoint พร้อมที่ `build/phase24-prep/myos-usb.img`:
raw whole-disk MBR BIOS/UEFI, **67,108,864 bytes (64 MiB)**,
ESP FAT16 32 MiB เริ่ม LBA 2048, EFI/BOOT/BOOTX64.EFI
SHA-256:
`3d23215359e35d8a65a5de61bf4fd52bdfcf84c5b81e31fcccc5717d49cfbe41`
ตรวจไฟล์ที่เตรียมแล้ว:

```bash
(cd /home/xytron404/myos/build/phase24-prep && sha256sum -c SHA256SUMS)
```

ไม่ใช่ ISO, ไม่ใช่ไฟล์สำหรับลากใส่ USB ที่ format แล้ว และไม่ใช่ installer
อิมเมจนี้ไม่รวม external assets ของ Cobalt/YouTube; ไม่ทำให้ native
Wi-Fi/audio/storage ของ ThinkBook ใช้งานได้
ไม่ใช้ release-validation.json เก่ามาอ้างว่า release package ปัจจุบันผ่านครบ

## เตรียม USB และเลือก boot device — สำหรับเจ้าของตรวจทานภายหลัง

**ยังไม่อนุญาตให้เขียน USB ในงานนี้** USB รุ่น/ความจุยังไม่ได้แจ้ง
ต้องตรวจว่าเป็นสื่อที่เจ้าของเลือกและยอมลบข้อมูล ขนาดอย่างน้อย 67,108,864
bytes, logical sector 512 bytes, และตรวจ source/checksum ก่อน
raw whole-device write จะเขียนทับ partition table/ข้อมูลเดิม
ไม่ format/repartition เพิ่ม และไม่เลือก SSD 256 GB หรือดิสก์ WSL แทน USB

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
# sudo dd if=/home/xytron404/myos/build/phase24-prep/myos-usb.img of="$USB_WHOLE_DEVICE" bs=4M conv=fsync status=progress
# sudo cmp -n 67108864 /home/xytron404/myos/build/phase24-prep/myos-usb.img "$USB_WHOLE_DEVICE"
```

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
   ทดสอบ save/reboot/restore Settings และ Snake; หากไม่มี driver/สื่อให้ลง
   BLOCKED ไม่ตีความ RAM-only เป็น persistence PASS หรือปิด full gate แทน
6. บันทึก network/audio เป็น unsupported/untested ตามตาราง เว้นแต่มี native
   driver และผลจริงเพิ่ม; การทดสอบนี้ไม่รับรอง YouTube บน ThinkBook
7. ถ้า panic, input ค้าง, จอดำ หรือ firmware ปฏิเสธ ให้หยุด เก็บหลักฐาน
   แล้วกลับระบบเดิมโดยถอด USB หลังปิดเครื่อง; ไม่พยายามแก้ด้วยการเขียน SSD

เก็บผลทุกข้อเป็น PASS/FAIL/BLOCKED/NOT RUN พร้อมหลักฐานและข้อจำกัด
ยังไม่มีผลจริงใน checkpoint นี้; ห้ามเลื่อนสถานะเป็นผ่านจากผล QEMU
