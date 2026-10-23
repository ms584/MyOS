# Phase24 — ตรวจ USB บน ThinkBook และขอบเขตการใช้พื้นที่

**อัปเดต USB:** ภาพพอร์ตจริงยืนยันว่า one-shot scan พลาด reconnect หลัง reset; เพิ่ม bounded2s discovery/100ms stable CCS แล้ว แต่เมาส์/Samsung จริงยังต้องลองใหม่. ใช้ [คู่มืออิมเมจแก้ startup](PHASE24_USB_STARTUP_RETEST_TH.md); อิมเมจ diagnostic เดิมเป็นประวัติ.

วันที่2026-10-23; Phase24/25 ยังไม่ครบ. ภาพจริงยืนยัน console/Desktop/Snake;
เจ้าของรายงานเมาส์ขยับไม่ได้ และหน้าจอแสดง RAM-only. ดู
[หลักฐานภาพ/คำรายงาน](wip/phase24/2026-10-23/desktop-mouse-evidence.json).
พบตัวควบคุม AMD สองตัว ไม่ได้แปลว่าเริ่มเมาส์หรือ Samsung สำเร็จ.
ยังไม่ทราบ root cause; อิมเมจรอบนี้เพิ่มการสังเกต ไม่เปลี่ยน reset/การเริ่มอุปกรณ์.

## พื้นที่ USB ใช้ได้แค่ไหน

อิมเมจ `myos-usb-data.img` เป็น raw whole-disk MBR64MiB พร้อม BIOS/UEFI,
ESP FAT16 32MiB และ custom MyOS snapshot partition16MiB typeDA.
ไม่ใช่ FAT filesystem สำหรับไฟล์ผู้ใช้ และไม่ได้ใช้ Samsung32GB เต็มความจุ.
RAMFS ยังคง64nodes รวมroot, สูงสุด4096bytes ต่อไฟล์; ดู PHASE11/13.
การเพิ่มขนาดพาร์ทิชันอย่างเดียวไม่เพิ่มขีดจำกัด RAMFS.

สามารถใช้ส่วน data ที่เตรียมไว้เมื่อ MyOS มองเห็น backend USB BOT แล้ว.
ปัจจุบันเครื่องจริงขึ้น RAM-only: ห้ามถือว่าไฟล์จะอยู่หลังปิดเครื่อง.
ยังไม่ format/repartition/ขยายพื้นที่หรือเขียน SSD; การใช้32GB เต็มเป็นงานเพิ่มเติม.
เมื่อ `disk` ยืนยัน backend/disk แล้วจึงทดสอบ save, mandatory flush และ reboot
ตาม PHASE25_HARDWARE_ACCEPTANCE.md. ไม่ใช้ Windows format เมื่อระบบถาม.

## อิมเมจและการบูต

อิมเมจ diagnostic และ checksum/commit บันทึกใน
[wip/phase24/2026-10-23/usb-diagnostics-evidence.json](wip/phase24/2026-10-23/usb-diagnostics-evidence.json).
อิมเมจ: `build/phase24-usb-diag-2026-10-23/images/myos-usb-data.img`
Windows: `\\wsl.localhost\Ubuntu\home\xytron404\myos\build\phase24-usb-diag-2026-10-23\images\myos-usb-data.img`
ขนาด67,108,864bytes; source commit `3482f0e6ed564e5ee27ae6af0cf82794f37f304f`.
SHA256: `07809cd86d99e0022e7e9eecd4d155ab2d8db3218b055452dd92ad62ab5a1b0f`.
ผล native diagnostics/port regression, BIOS/UEFI USB mouse และ USB2/USB3 BOT
save/flush/reboot/recovery/bounds PASS; independent FAT/EFI/layout และ byte-identical
rebuild PASS. ไม่ใช่ผล ThinkBook retest. Manifest/SHA256SUMS อยู่ข้างอิมเมจ.

ใช้ **myos-usb-data.img** จากโฟลเดอร์ diagnostic ที่หลักฐานระบุ;
ไม่ใช้ myos-usb.img ซึ่งไม่มี data partition และไม่ใช้ไฟล์ GOP เก่าแทน.
ขั้นตอน raw flash/เลือก Samsungด้วยตัวเอง/ห้ามเลือกSSD/F12 อยู่ใน
[คู่มือบูต](PHASE24_GOP_RETEST_TH.md). การ flash ใหม่ทับข้อมูลเดิมบน USB;
สำรองก่อนทำ. เอเจนต์เตรียมเฉพาะไฟล์ ไม่ดำเนินการเขียน USB จริง.

ก่อนเปิดเครื่อง เสียบ Samsung และเมาส์เข้าพอร์ตเครื่องโดยตรง ไม่ผ่าน hub.
ไม่ถอดเสียบใหม่หลัง kernel เริ่ม เพราะยังไม่มี hotplug enumeration.

## เจ้าของเก็บผลจาก Shell ในอิมเมจ diagnostic

กด Esc ออกจาก Desktop แล้วพิมพ์ทีละบรรทัด:

```text
usb
usb 0
usb 1
disk
```

ถ่ายภาพผลแต่ละคำสั่ง โดยเฉพาะแถว P1,P2,... พร้อมระบุพอร์ตที่ใช้และว่ามี hub หรือไม่.
คำสั่ง `usb` อ่าน counters/สถานะที่บันทึกไว้; `usb 0`/`usb 1` เพิ่ม register reads
ของ controller และ root ports. ไม่ reset, ไม่สั่ง power, ไม่ acknowledge event,
ไม่เริ่มอุปกรณ์ใหม่ ไม่เขียนดิสก์และไม่เปลี่ยน firmware settings.
คำสั่งนี้มีเฉพาะอิมเมจ diagnostic ใหม่; อิมเมจ GOP เดิมจะไม่รู้จัก `usb`.

- `init`: ขั้นตอนล่าสุด1PCI,2capabilities,3ownership,4halt,5reset,6DMA/rings,
  7start,8scan,9scanจบหรือถูกหยุด; ต้องอ่านร่วมกับ `running`.
- `pre`: PORTSC ก่อน takeover/reset; `scan`: ตอนวนพอร์ตหลัง start;
  `now`: register ขณะพิมพ์คำสั่ง. เป็นค่าhex, bit0connected,bit1enabled,
  bit4reset,bit9power,bits10–13speed,bits5–8linkstate.
  pre/scanเป็น snapshot ไม่ใช่สภาพปัจจุบัน; scan0อาจหมายถึงยังไม่ได้วนพอร์ตนั้น.
- `s`: ขั้นตอนล่าสุดของพอร์ต:0ยังไม่พบconnectedตอนscan/ยังไม่scan,
  1เริ่มreset,2resetไม่สำเร็จ,3ตรวจspeed/slot,4EnableSlot,5จัดสรรcontext,
  6AddressDevice,7descriptor8,8EvaluateContext,9descriptor18,
  10configuration9,11configurationเต็ม,12parseinterface,
  13SetConfiguration,14เตรียมBOT,15เรียกstorage attach,
  16SetProtocol boot mouse,17mouse buffers,18ConfigureEndpoint,19mouseพร้อมรับ.
  ค่าแสดง **ขั้นตอนที่พยายามล่าสุด** ไม่ใช่ทุกขั้นตอนสำเร็จ;
  s15ไม่ได้รับรอง storage สำเร็จ ต้องตรวจ `disk` ด้วย.
- `last command/transfer CC` คือ completion code ล่าสุดต่อcontroller อาจเปลี่ยน
  จาก polling/runtime ไม่ใช่ error ของพอร์ตทุกแถว. reports/wraps/disconnects เป็น counters รวม.

หยุดที่ข้อมูลเหล่านี้ก่อนลองเปลี่ยน firmware/reset/power แบบเดาสุ่ม.
Settings/Calculator ยังไม่มีหลักฐานจริง; Snake Score5/RAM-only เป็นเพียงผลที่เห็นในภาพ.
ยังไม่ปิด Phase24/25 จนกว่าเมาส์และ Samsung save/flush/reboot ผ่านเกณฑ์จริง.

## Build ที่ทำซ้ำได้ (ไฟล์ใน repo เท่านั้น)

ที่ commit ใน manifest และ working tree สะอาด ใช้ WSL Ubuntu ใน repoจริง:

```sh
cd /home/xytron404/myos
python3 tools/run_logged.py -- python3 tools/package_bios.py
python3 tools/run_logged.py -- python3 tools/build_uefi.py
python3 tools/run_logged.py -- python3 tools/package_usb.py
python3 tools/run_logged.py -- python3 tools/package_usb.py --with-data --output build/myos-usb-diagnostic-rebuild.img
sha256sum build/myos-usb-diagnostic-rebuild.img
```

ไฟล์ output ต้องยังไม่มี; เครื่องมือปฏิเสธการเขียนทับ save image เดิม.
เทียบ checksum กับอิมเมจ frozen ในหลักฐาน ไม่ใช้ git commit ของ docs follow-up
แทน source_commit ของอิมเมจ. ไม่มีหมายเลข physical disk ในคำสั่งเหล่านี้.
