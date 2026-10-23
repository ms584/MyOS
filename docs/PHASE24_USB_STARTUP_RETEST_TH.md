# Phase24 — ลองอิมเมจแก้ USB reconnect บน ThinkBook

วันที่2026-10-23; Phase24/25 ยังไม่ครบ. แก้การตรวจพบ USB หลัง reset ตามภาพจริง;
ยังไม่ใช่ผลยืนยันว่าเมาส์/Samsung บนเครื่องจริงผ่านแล้ว.

## สาเหตุที่มีหลักฐาน

[ภาพ usb/usb0/usb1](wip/phase24/2026-10-23/usb-startup-physical-evidence.json)
แสดง controller ทั้งสอง init9/running1, ทุกพอร์ตs0, command/transfer CC0,
reports0. พอร์ตที่เห็นอุปกรณ์ก่อน takeover เช่น H0P3/H1P6 กลับไม่มี CCS ตอนscan
แต่ตอนnowมี CCS อีกครั้ง. ไดรเวอร์เดิมอ่านครั้งเดียวทันทีหลัง HCRST แล้วข้ามพอร์ต
จึงไม่เคยส่ง EnableSlot/AddressDevice/SET_PROTOCOL ให้อุปกรณ์ที่กลับมาช้า.
ยังระบุไม่ได้ว่าพอร์ตใดเป็น mouse VID30fa:0301 หรือ Samsung090c:1000 จนกว่าจะ enumerate.

ตรงกับ [Intel xHCI1.22 sections4.19.3/4.19.4](https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/extensible-host-controler-interface-usb-xhci.pdf):
root port เริ่ม Disconnected หลัง reset แล้วรายงาน CCS เมื่อพบการเชื่อมต่อ.
USBSTS0x10 เป็น port change; controller running ไม่ใช่ผลว่าเริ่มอุปกรณ์สำเร็จ.

แก้ให้สังเกต root ports ทุกพอร์ตร่วมกันในช่วง2วินาทีต่อcontroller,
ต้องมี CCS ต่อเนื่อง100ms จึงใช้ reset/enumeration เดิม. พอร์ตว่างไม่ทำให้รอทีละ2วินาที.
มี iteration cap หาก PIT ไม่เดิน; ไม่ force power/reset firmware ไม่เพิ่ม hubs หรือ
runtime hotplug. ThinkBook2controllers เพิ่มช่วง discovery รวมประมาณ4วินาที.
`usb`/`usb 0`/`usb 1` ยังคงเป็น read-only.

## อิมเมจใหม่และการสร้างซ้ำ

รายละเอียด source commit,ขนาด,checksum และคำสั่ง build จะอยู่ใน
[wip/phase24/2026-10-23/usb-startup-evidence.json](wip/phase24/2026-10-23/usb-startup-evidence.json)
และ manifest ข้างอิมเมจ. อิมเมจ diagnostic เดิม07809cd8... ยังไม่มีการแก้นี้.

อิมเมจเป็น raw MBR64MiB พร้อม BIOS/UEFI,ESP32MiB และ custom data16MiB.
RAMFS ยัง64nodes/4096bytes ต่อไฟล์ ไม่ใช่การใช้ความจุ32GB เต็ม.
พื้นที่ข้อมูลจะใช้งานถาวรได้ก็ต่อเมื่อ `disk` ยืนยัน backend และทดสอบ save/flush/reboot.
ไม่ format/repartition เพิ่ม ไม่ใช้ SSD ระบบเป็น scratch.

## เจ้าของลองบน ThinkBook เมื่อพร้อม

1. สำรองข้อมูล Samsung ก่อน: raw flash อิมเมจใหม่จะทับข้อมูลเดิม.
   เลือก Samsung DUO32GB ด้วยตนเอง ไม่เดาหมายเลขดิสก์และไม่เลือก SSD256GB.
   ขั้นตอนเขียน raw image/validation/F12 ตาม [คู่มือบูต](PHASE24_GOP_RETEST_TH.md).
   เอเจนต์ไม่ได้เขียน USB จริงหรือเปลี่ยน firmware settings.
2. ปิดเครื่อง เสียบ Samsung และเมาส์เข้าพอร์ตเครื่องโดยตรงก่อนเปิดเครื่อง ไม่ผ่านhub.
   เลือก UEFI USB ใน F12. หลัง kernel เริ่ม รอช่วง discovery ทั้งสอง controller.
   ไม่ถอดเสียบใหม่ระหว่างการทดสอบ เพราะไม่รองรับ runtime hotplug enumeration.
3. มองหาบรรทัด `USB device ...` และ `USB: xHCI HID boot mouse initialized`.
   หากไม่มี ให้เก็บ boot log และผลคำสั่งด้านล่าง; ไม่เดาเปลี่ยน firmware/reset/power.
4. พิมพ์ทีละบรรทัด:

```text
usb
usb 0
usb 1
mouse
disk
desktop
```

5. ใน Desktop ขยับเมาส์ซ้าย/ขวา/ขึ้น/ลง ลองคลิก Settings/Calculator/Snake.
   ถ้าค้าง ให้กดEsc กลับShell เก็บภาพ `usb 0`, `usb 1`, `mouse` หลังขยับแล้ว.
   s19หมายถึงmouse endpointเริ่มแล้ว; reports ต้องเพิ่มเมื่อส่ง report ไม่ใช่แค่มีcursor.
   s20ใหม่หมายถึงตอนscanเห็นconnected แต่ไม่ผ่าน100ms stableภายในstartup window.
   ความหมายstageอื่นและ PORTSC ดู [คู่มือ diagnostics](PHASE24_USB_DIAGNOSTICS_TH.md).
   `init`/`running` เป็นค่ารายงาน ไม่ใช่คำสั่ง Shell.
6. หาก `disk` ยังRAM-only ห้ามถือว่าบันทึกถาวรผ่าน. หากได้USB backend ค่อยทดสอบ
   file/Settings/Snake save, mandatory flush และรีบูตrestore ตาม
   [Phase25 checklist](PHASE25_HARDWARE_ACCEPTANCE.md). ยังไม่ปิดPhase24/25ก่อนผลจริงครบ.

การทดสอบซอฟต์แวร์ไม่แทน AMD/Samsung จริง. Native regression จำลอง delayed attach,
bounce/disconnect,late/empty ports,stalled PIT,32ports,tick wrap; negative control
ใช้เงื่อนไขเดิมแล้วพลาดอุปกรณ์. QEMU fixture ต่อเมาส์150msหลังR/S startup
ก่อนเข้าShell และตรวจ movement/buttons ทั้งBIOS/UEFI (IDE boot แยก fixtureนี้).
ผล/รายการ regression ที่ผ่านจริงจะอยู่ใน usb-startup-evidence.json.
