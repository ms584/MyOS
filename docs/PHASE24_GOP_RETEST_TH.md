# Phase 24 — ลองบูต ThinkBook อีกครั้งหลังแก้ framebuffer

**อัปเดตผลจริง:** ภาพลองใหม่ยืนยัน800x600 console/kernel/Shell และ boot checks ผ่านแล้ว; ยัง RAM-only ไม่มีหลักฐาน USB HID/BOT สำเร็จ. [หลักฐานล่าสุด](wip/phase24/2026-10-23/kernel-shell-evidence.json). ทดสอบ typing/mouse/desktop/สามแอปต่อได้ ไม่ต้อง reflash ซ้ำตอนนี้. ข้อความรอลองใหม่/FAIL ก่อน kernel ด้านล่างเป็น checkpoint ก่อนภาพใหม่นี้.

วันที่2026-10-23; Phase24 ยังไม่ครบ; อิมเมจใหม่เข้า kernel/Shell บน ThinkBook แล้ว; input/apps/USB persistence ยังไม่ครบ
สถานะซอฟต์แวร์: full system + high-GOP/BIOS/UEFI/USB/DMA/layout/repro PASS; เครื่องจริงเข้า Shell แล้ว; ทดสอบส่วนที่เหลือต่อ
ผล/คำสั่ง/log hashes: [gop-evidence.json](wip/phase24/2026-10-23/gop-evidence.json) และ [สรุปผล](wip/phase24/2026-10-23/gop-tests.log)

## เหตุการณ์บนเครื่องจริง

เจ้าของส่งภาพจาก ThinkBook21A4: firmware พบ Samsung Flash Drive DUO และเรียก
MyOS UEFI loader ได้ แต่หยุดที่ `unsupported framebuffer range (Unsupported)`
ก่อน kernel. ข้อความ `EFI USB Device ... boot failed` เป็นผลตามหลัง loader คืน error
ไม่ใช่หลักฐานว่าไฟล์ BOOTX64.EFI หายหรือ USB เสีย
ภาพ/แหล่งที่มา/SHA256: [physical-boot-evidence.json](wip/phase24/2026-10-23/physical-boot-evidence.json)

เงื่อนไขเดิมรวม pitch,base,ขนาดและขอบเขต4GiB จึงยังระบุไม่ได้จากภาพว่า
ค่าจริงใดไม่ผ่าน ตามเส้นทางของ loader ที่เลือก ข้อความนี้อยู่หลังเลือก/SetMode
800x600 BGR สำเร็จแล้ว แต่ภาพไม่แสดงค่า GOP จริงหรือ checksum ที่อ่านกลับจาก USB

## อิมเมจสำหรับลองใหม่

- source commit: `32f7b6873e719f20275b7b5284ab99edd82d537c`
- ไฟล์: `/home/xytron404/myos/build/phase24-gop-2026-10-23/images/myos-usb-data.img`
- raw whole-disk MBR image; BIOS+UEFI;64MiB=67,108,864 bytes
- SHA256: `e0da9a5f2ff03e8031ab9cc1ed4655281c7b580cfde4c3a09cb58b3d62b66917`
- ESP32MiB + MyOS snapshot data16MiB; ไม่ใช่ ISO/installer หรือไดรฟ์ Windows

เปิด File Explorer บนเครื่องที่มี repo แล้ววางพาธ:

```text
\\wsl.localhost\Ubuntu\home\xytron404\myos\build\phase24-gop-2026-10-23\images
```

คัดลอก `myos-usb-data.img` ไป Downloads. ตรวจไฟล์ที่คัดลอกใน PowerShell:

```powershell
Get-FileHash -Algorithm SHA256 "$env:USERPROFILE\Downloads\myos-usb-data.img"
```

อย่าเลือกอิมเมจเดิมใน `build/phase24-usb-input/images/` เพื่อทดสอบการแก้นี้
ไฟล์เดิมเก็บไว้เป็นหลักฐานของความล้มเหลวและผล QEMU เดิม ไม่เขียนทับ

## เตรียม USB และบูต — ขั้นตอนเจ้าของทำเมื่อพร้อม

การเขียนอิมเมจทับข้อมูลเดิมบน Samsung DUO32GB รวมพาร์ทิชันและข้อมูลที่เคยบันทึก
สำรองข้อมูลก่อนและตรวจว่าพร้อมลบข้อมูลบนสื่อนี้. ไม่มีการเขียน USB/SSD หรือ
เปลี่ยน firmware โดยเอเจนต์; คู่มือนี้ไม่อนุญาตให้เดาหมายเลขดิสก์

1. ใช้โปรแกรมเขียน raw .img เช่น [Etcher](https://etcher.balena.io/): เลือก
   Flash from file เป็นไฟล์ใหม่, Select target เป็น Samsung DUO32GB ที่ตรวจเอง,
   ไม่เลือก SSD256GB. กด Flash เมื่อพร้อมทับข้อมูลและรอ validation สำเร็จ
2. ไม่ format ก่อน/หลัง, ไม่ขยายพาร์ทิชัน. หาก Windows ขอ format ให้ Cancel.
   ตรวจ readback ก่อนบูตครั้งแรก; หลัง MyOS save พื้นที่ data จะเปลี่ยนตามปกติ
3. ปิด ThinkBook เสียบ Samsung และเมาส์เข้าพอร์ตเครื่องโดยตรงก่อนเปิดเครื่อง
   ไม่ใช้ hub และไม่ถอด/เสียบใหม่ขณะทดสอบ เพราะยังไม่มี hotplug enumeration
4. เปิดเครื่องแล้วกด F12 หรือ Fn+F12 ซ้ำที่โลโก้ Lenovo เลือก USB แบบ UEFI
   ใช้เมนูชั่วคราว. ถ้าไม่พบ USB หรือ firmware บล็อก ให้เก็บภาพ ไม่เปลี่ยน settings แบบเดา
5. ถ่ายภาพ/วิดีโอตั้งแต่ loader โดยเฉพาะสองบรรทัด `MyOS UEFI GOP: base=... size=...`
   และ `pitch=... physical-bits=...` พร้อมข้อความ error ถ้ามี
6. ถ้าถึง Shell ให้พิมพ์ `mem`, `uptime`, `mouse`, `disk` ทีละบรรทัดและเก็บผล
   จากนั้น `desktop` เพื่อลองคีย์บอร์ด/เมาส์และสามแอป. หยุดที่ปัญหาแรกและรายงาน
7. การ save/reboot/restore ใช้เฉพาะ data partition ของ Samsung ที่เลือก:
   หาก `disk` แสดง RAM only ให้ลง BLOCKED ไม่ถือว่าบันทึกถาวรผ่าน
   รายการตรวจรับเต็มอยู่ใน [Phase25](PHASE25_HARDWARE_ACCEPTANCE.md)

หากติด `kernel low-memory reservation`, `loader allocation`,จอดำ หรือ input ค้าง
ให้ส่งภาพและขั้นตอนล่าสุด ไม่แก้ด้วยการเขียน SSD/เปลี่ยน SATA mode/flash BIOS
รุ่นนี้ยังไม่มี shutdown/reboot: ก่อนบังคับปิดให้ `sync` รอผลสำเร็จและ prompt
การกด Power ค้างยังเป็น forced shutdown มีความเสี่ยงข้อมูล ไม่รับรอง physical flush
จนกว่าจะทดสอบบน Samsung ผ่าน. หลังเครื่องดับถอด USB แล้วเปิดเพื่อกลับ Windows

## สิ่งที่แก้และขอบเขต

เพิ่ม bounded framebuffer mapping เหนือ4GiB ผ่าน supervisor UC/NX window
`0x7e00000000` ก่อน ExitBootServices; แชร์ผ่าน PML4[0] ภายใต้ process CR3
ตรวจ physical-address width ของ CPU,overflow,ขนาด,pitch; BIOS low framebuffer คงเส้นทางเดิม
หากผิดพลาดจะแสดงค่า base,size,pitch และแยกสาเหตุแทนข้อความรวมเดิม
ยังต้อง800x600 BGR,pitch3200–16384 และจอง low kernel/stack/DMA ตามเดิม

การทดสอบ high mapping ใช้ GOP base ที่ฉีดใน loader เฉพาะชุดทดสอบ ไปยัง QEMU
DIMM RAM เหนือ4GiB ไม่ใช่ผล GPUจริง. Native bounds/ASan/UBSan และ negative control
ของโค้ดเดิมผ่าน; QEMU high console/GUI pixels/สามแอป/Ring3 ผ่าน
การระบุว่า ThinkBook ใช้งานได้ต้องมีผลลองใหม่จากเจ้าของ

อ้างอิง: [Lenovo temporary boot menu](https://download.lenovo.com/pccbbs/pubs/thinkbook_14_15_gen3/html_en/EN/change_device_boot_order.html),
[UEFI GOP FrameBufferBase/Size](https://uefi.org/specs/UEFI/2.10/12_Protocols_Console_Support.html)
