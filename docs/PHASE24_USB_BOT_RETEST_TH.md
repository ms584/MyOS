# Phase24 — Samsung BOT / bulk timeout ขณะปิด interrupt

วันที่2026-10-24; Phase24/25 ยังไม่ครบ. เจ้าของยืนยันเมาส์เคลื่อนไหวได้แล้ว;
ภาพล่าสุดเห็น30fa:0301 HID initialized และ1849reports. Samsung090c:1000 พบแล้ว
แต่ H1หยุดที่BOT attach(s15),disk RAM-only. ดู
[ภาพ/ที่มา/ข้อจำกัดการวินิจฉัย](wip/phase24/2026-10-24/bot-physical-evidence.json).

## สิ่งที่รู้และสิ่งที่ยังไม่รู้

ภาพแสดง last command/transfer CC1/1 แต่โค้ดเดิมไม่ล้างtransfer CCก่อนbulk TDใหม่.
ค่านี้อาจเป็นผลสำเร็จก่อนหน้า จึงไม่ใช่หลักฐานว่าTDที่ล้มเหลวสำเร็จหรือระบุphaseได้.
ข้อความ no supported prepared data partition เป็นข้อความรวมหลังprobeล้มเหลว;
ยังไม่มีหลักฐานว่าพาร์ทิชันเสีย ไม่แก้ด้วยformat/repartition.

พบข้อผิดพลาดใน wait loop: BOT/snapshot ปิดIFเพื่อatomicity ทำให้PIT ticksไม่เพิ่ม.
ตัวguard2ล้านรอบ CPU อาจจบก่อน1วินาทีและหยุดcontrollerทั้งที่deviceยังมีเวลาตอบ.
Native fixture จำลอง DMAตอบหลัง2.4ล้านpolls แต่ยังไม่ถึง1วินาที: โค้ดเดิมพลาด,
โค้ดใหม่รับได้. ผลนี้พิสูจน์ข้อบกพร่องของdeadline ไม่ใช่การรับรองว่าครอบคลุมทุกสาเหตุบนSamsung.

ใช้ [xHCI1.22 section5.5.1 MFINDEX](https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/extensible-host-controler-interface-usb-xhci.pdf)
ซึ่งเพิ่มทุก125usเมื่อR/S1 เป็นdeadline8000microframes(1วินาที)โดยไม่เปิดIF.
จัดการ14bit wrap; guard2ล้านรอบใช้เฉพาะเมื่อhardware clockไม่เดิน.
คงfail-closed,exact-length/CSW checks,DMA ownership,MBR/ESP/marker write bounds
และmandatory SYNCHRONIZE CACHE. ไม่เพิ่มreset recovery,hub,UASหรือdriverSSD.

## อิมเมจและbuild

ไฟล์: `build/phase24-usb-bot-clock-2026-10-24/images/myos-usb-data.img`
Windows:

```text
\\wsl.localhost\Ubuntu\home\xytron404\myos\build\phase24-usb-bot-clock-2026-10-24\images\myos-usb-data.img
```

ขนาด **67,108,864bytes (64MiB)**; source commit `6fc22f5ed84be859a8381de057b498c55b6b812c`.
SHA256: `d66310b71d0ecb737941bf5a6dfb22448cf728d0211456de923a06d0effa85aa`.
Manifest/SHA256SUMS อยู่ข้างอิมเมจ. Native hardware-clock/negative control+BOT phase,
startup/input BIOS/UEFI,USB2/USB3 persistence/coexistence/bounds และFAT/layout/repro PASS.
อิมเมจนี้ยังNOT RUNบนSamsungจริง; ภาพownerใช้startup imageก่อนหน้า.

สร้างซ้ำจากsource commitข้างต้นและworking treeไม่มีexecutable changes:

```sh
cd /home/xytron404/myos
python3 tools/run_logged.py -- python3 tools/package_bios.py
python3 tools/run_logged.py -- python3 tools/build_uefi.py
python3 tools/run_logged.py -- python3 tools/package_usb.py
python3 tools/run_logged.py -- python3 tools/package_usb.py --with-data --output build/myos-usb-bot-clock-rebuild.img
sha256sum build/myos-usb-bot-clock-rebuild.img
```

outputต้องยังไม่มี; เครื่องมือไม่ทับsave imageเดิม. คำสั่งสร้างเฉพาะไฟล์ในbuild/.

ไฟล์/commit/checksum/results บันทึกใน
[wip/phase24/2026-10-24/bot-clock-evidence.json](wip/phase24/2026-10-24/bot-clock-evidence.json).
อิมเมจraw MBR64MiB,ESP32MiB,custom data16MiB. RAMFSv2สูงสุด65536bytesต่อไฟล์
และ64persistent nodesตามinclude/fs.h;4KiBเป็นขีดจำกัดv1เดิม ไม่ใช่v2ปัจจุบัน.
ไม่ได้ใช้Samsung32GBเต็ม. Source/คู่มือ/หลักฐานอยู่Git; อิมเมจbuild/อยู่ในrepoจริงที่เครื่องbuild.

## เจ้าของทดสอบเมื่อพร้อม

1. สำรองข้อมูลUSBก่อนflash: การเขียนraw imageใหม่ทับข้อมูลเดิมบนSamsung.
   เลือกSamsungด้วยตัวเอง ไม่เลือกSSD/เดาหมายเลขดิสก์. ขั้นตอนvalidation/F12ดู
   [คู่มือบูต](PHASE24_GOP_RETEST_TH.md). เอเจนต์ไม่เขียนUSBจริง/format/เปลี่ยนfirmware.
2. เสียบSamsungและเมาส์ตรงเครื่องก่อนเปิด ไม่ผ่านhub/ไม่hotplugหลังkernelเริ่ม.
3. หลังถึงShellพิมพ์ `disk`. ถ้าเป็นUSB BOT data disk ให้เก็บภาพ แล้วค่อยทดสอบ:

```text
write /home/usb-check phase24-samsung
cat /home/usb-check
sync
disk
```

รอแต่ละคำสั่งคืนpromptและsyncสำเร็จ. เก็บผล backend/generation.
จากนั้นปิด/เปิดตามขั้นตอนเจ้าของในPhase25โดยใช้USBเดิมไม่flashซ้ำ แล้วตรวจ
`cat /home/usb-check` และSettings/Snake restore. ภาพก่อน/หลังจำเป็นต่อการยืนยันpersistence.
หากdiskยังRAM-only หยุดการทดสอบpersistenceและเก็บboot log.

4. ถ้ายังล้มเหลว ภาพควรมีบรรทัดใหม่:

```text
USB bulk failed: IN/OUT len=... reason=... CC=... residual=... microframes=...
USB BOT failure: opcode=... phase=... transport=...
```

- reason completion: matching TDตอบerror/short/residual; CCเป็นผลTDนั้น.
- reason timeout: hardware clockครบ8000microframesแล้วยังไม่มีmatching completion.
- reason clock-stalled: MFINDEXไม่เปลี่ยนครบguard; ต้องตรวจcontroller/clock.
- phase1=CBW,2=data,3=CSW; transport0=bulk failure,1=ได้bulkครบแต่CSWไม่ผ่านตรวจ.
- opcodeเช่น0x12=INQUIRY,0x00=TEST UNIT READY,0x25=READ CAPACITY,
  0x28=READ,0x2a=WRITE,0x35=SYNCHRONIZE CACHE.

เก็บ `usb 1` เพิ่มถ้าจำเป็น. ไม่ต้องพิมพ์init/runningเพราะเป็นค่ารายงานไม่ใช่คำสั่ง.
`USB BOT: prepared MyOS data partition` +diskเป็นUSBยังไม่แทนผลwrite/flush/rebootครบ.
เมาส์บนH0ยังต้องทำงานต่อขณะH1storage fail; อย่าทำให้mouse PASSกลายเป็นstorage PASS.

เกณฑ์เต็มอยู่ใน [Phase25 checklist](PHASE25_HARDWARE_ACCEPTANCE.md).
ยังไม่ปิดPhase24/25จนกว่าSamsungและแอปตามเกณฑ์เครื่องจริงครบ.
