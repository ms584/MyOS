# Phase25 — ขั้นสุดท้าย: ตรวจอ่าน Samsung โดยไม่เขียน USB

2026-10-24: Shell file testผ่านแล้ว: `/home/phase25.txt` เป็น
`ThinkBook-Phase25-PASS` ก่อน/หลังบูต, sync/restored/disk generation14ตรงกัน.
[หลักฐาน](wip/phase25/2026-10-24/shell-file-evidence.json).
ยังไม่ปิดPhase25จนกว่าจะตรวจและแก้ข้อแตกต่างในphysical readbackได้ตามเกณฑ์.

## ผลล่าสุด: ยังไม่ผ่าน

รายงานเจ้าของเวลา2026-10-24T20:15:13+07:00 ยืนยัน physical:true และตัวระบุSamsungDisk1ตรง.
boot-prefix ไม่ตรง: expected `65047f87ed20d372753176127e08977cd9d5dd0a57d618f46eb7c36e221457a7`,
actual `086eb42035aec728f98c1b00af5ccb0b9673fcdf0f4c3bc24e7cf0954885b90f`.
data-marker และimmutable-tailตรงทั้งคู่. ยังไม่ทราบตำแหน่งbyteที่ต่างหรือสาเหตุ;
ไม่สรุปว่าUSBเสีย และไม่ใช้ผลsave/restoreมาแทนการตรวจนี้.
[รายงานต้นฉบับ](wip/phase25/2026-10-24/physical-usb-readback.json) และ
[provenance](wip/phase25/2026-10-24/physical-usb-readback-provenance.json).
ขั้นถัดไป: อ่านเปรียบเทียบMBR/ไฟล์ESP/FAT metadata/paddingให้แคบลงกับอิมเมจเดิม.
ไม่ต้องรันverifierเดิมซ้ำหรือflash/formatใหม่; รักษาข้อมูลgeneration14ไว้. เครื่องมือแยกตำแหน่งด้านล่างพร้อมทดสอบแล้ว.

## ขั้นตอนอ่านครั้งแรกที่เจ้าของทำแล้ว (อ้างอิง)

ภาพ Get-Disk ล่าสุดยืนยัน **Disk1 Samsung Flash Drive DUO USB32078036992B,
512B,IsBootFalse/IsSystemFalse**. Disk0เป็นNVMeระบบ ไม่ใช่สื่อทดสอบ.
ไม่ต้องflash/formatใหม่; ใช้Samsungที่เก็บgeneration14อยู่แล้ว.

1. ดาวน์โหลด [verify_phase25_usb_readback.ps1](https://github.com/ms584/MyOS/blob/main/tools/verify_phase25_usb_readback.ps1)
   ด้วยปุ่ม Download raw file แล้วบันทึกไว้ใน **Downloads** โดยใช้ชื่อไฟล์เดิม
   `verify_phase25_usb_readback.ps1` (ไม่ใช่.txt หรือหน้าHTML).
2. ใช้หน้าต่าง **Windows PowerShell Administrator** ที่แสดงในภาพล่าสุด.
   ถ้าได้ถอดเสียบ/เพิ่มดิสก์หลังภาพนั้น ให้ตรวจ Get-Diskใหม่ก่อน; สคริปต์ก็ตรวจidentityซ้ำ.
3. รันคำสั่งบรรทัดเดียวนี้ (Disk1มาจากภาพเจ้าของ ไม่ได้เดา):

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$env:USERPROFILE\Downloads\verify_phase25_usb_readback.ps1" -DiskNumber 1 | Tee-Object -FilePath "$env:USERPROFILE\Downloads\phase25-usb-readback.json"
```

4. ส่งไฟล์ `Downloads\phase25-usb-readback.json` กลับมา หรือภาพผลครบ.
   ต้องมี `physical: true`, `result: PASS`, ทั้งสามregions `match: true`.
   ถ้าFAILหรือerror ให้ส่งผล ไม่format/repartition/flashซ้ำ.

ExecutionPolicyBypassใช้กับprocessนี้เท่านั้น ไม่ตั้งpolicyถาวร.
สคริปต์เปิดUSBด้วยGENERIC_READ/OPEN_EXISTINGและFileAccess.Readเท่านั้น;
Tee-ObjectบันทึกรายงานในDownloadsของWindows ไม่เขียนrawUSB.
ไม่มีSet-Disk/format/partition/lock/dismount/firmware actionหรือการเลือกดิสก์อัตโนมัติ.
ต้องตรวจชื่อ/USB/ความจุ/sector/MBRและIsBoot/IsSystemทั้งก่อนและหลังอ่าน.
หากไม่ตรงจะหยุด ไม่ข้ามการตรวจหรือเลือกSSDแทน.

## ขอบเขตการเทียบ

อ้างอิงอิมเมจsource `6fc22f5ed84be859a8381de057b498c55b6b812c`,67108864B,
SHA256 `d66310b71d0ecb737941bf5a6dfb22448cf728d0211456de923a06d0effa85aa`.
Digestฝังในสคริปต์ จึงไม่ต้องดาวน์โหลดอิมเมจ64MiBมาเทียบอีกครั้ง.

- boot-prefix: offset0,length35651584B (34MiB),รวมMBR/BIOS/ESP/gap.
- data-marker: offset35651584,length512B; markerนี้MyOSไม่เขียนทับ.
- immutable-tail: offset52428800,length14680064B (14MiB).
- เว้นข้อมูลsnapshotที่เปลี่ยนได้: offset35652096,length16776704B.
  พื้นที่หลังอิมเมจ64MiBจนถึงปลายUSB32GBไม่ได้ตรวจ.

Full-image checksumหลังsaveย่อมต่างจากอิมเมจเปล่า; การตรวจนี้คงไฟล์และgeneration14ไว้.
ไม่ใช่การพิสูจน์ทุกsectorหรือความทนทานเมื่อไฟดับระหว่างเขียน.

## ผลทดสอบเครื่องมือ

Windows PowerShell5.1รันโค้ดจริงกับไฟล์fixture: pristine/modified-saveผ่าน;
เปลี่ยนboot/marker/tailตรวจพบ; ตรวจhashอิสระและไฟล์ต้นฉบับไม่เปลี่ยน.
Get-Diskguardทดสอบด้วยmetadataจำลอง: number/brand/bus/size/sector/MBR/boot/systemผิดถูกปฏิเสธ.
ไม่มีnumber/numberติดลบ/devicepathในfixture/ไฟล์สั้น/EOFถูกปฏิเสธ.
เปิดstreamผ่านread-onlyCreateFileWแบบเดียวกันในfixtureและphysicalmodeเพื่อรองรับPS5.1.
หลักฐาน [readback-tool-tests.json](wip/phase25/2026-10-24/readback-tool-tests.json).
**Fixture PASSไม่แทนphysical PASS**; agentยังไม่ได้เปิดrawdiskจริง.

## ตรวจตำแหน่งที่ต่าง: ขั้นที่เจ้าของทำต่อ

ดาวน์โหลด [diagnose_phase25_usb_prefix.ps1](https://github.com/ms584/MyOS/blob/main/tools/diagnose_phase25_usb_prefix.ps1)
ด้วย Download raw file ลง Downloads บน ThinkBook. ใช้ Samsung เดิมและ Windows PowerShell Administrator.
Disk1มาจากภาพเจ้าของ; หากถอดเสียบหรือเพิ่มดิสก์ให้ตรวจ Get-Disk ใหม่ก่อน.
สคริปต์ตรวจชื่อ/USB/ความจุ/sector/MBR/non-system ทั้งก่อนและหลังอ่านเหมือน verifier เดิม.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$env:USERPROFILE\Downloads\diagnose_phase25_usb_prefix.ps1" -DiskNumber 1 | Tee-Object -FilePath "$env:USERPROFILE\Downloads\phase25-prefix-diagnostic.json"
```

ส่ง Downloads\phase25-prefix-diagnostic.json กลับมา. MISMATCH เป็นผลวินิจฉัยที่คาดได้จากรายงานก่อนหน้า;
ไม่ใช่คำสั่งซ่อมหรือการยืนยันว่าUSBเสีย. ไม่แฟลช/format/แก้ข้อมูลUSBเพื่อให้checksumผ่าน.
อ่านเพียง34MiBแรก แบ่ง544ช่วงขนาด64KiB เทียบhashกับอิมเมจต้นฉบับเดิม.
รายงานเก็บoffset/hashทุกช่วงที่ต่าง และเนื้อหาbase64เฉพาะ16ช่วงแรก สูงสุด1MiB
(รายงานJSONประมาณ1.5MiBเมื่อUTF-8 หรือ3MiBเมื่อPowerShellบันทึกUTF-16); ใช้เทียบbyteกับต้นฉบับเพื่อแยกMBR/ESPไฟล์บูต/FATmetadata/padding.
ไม่ได้อ่านส่วนsnapshot MyOS. หากcapture_truncated=true ต้องวินิจฉัยส่วนที่เหลือเพิ่มเติม;
ไม่ถือว่าเนื้อหาส่วนที่ไม่ได้เก็บผ่าน. MATCH ก็ไม่ปิดPhase25โดยอัตโนมัติ ต้องประเมินร่วมกับผลก่อนหน้า.
ไม่มีการเปิดดิสก์เขียน; Tee-ObjectเขียนเฉพาะรายงานลงDownloadsบนWindows.

สร้างสคริปต์ซ้ำในrepoจริงด้วย `python3 tools/run_logged.py -- python3 tools/generate_phase25_prefix_diagnostic.py`.
GeneratorตรวจSHA256อิมเมจเดิมก่อนฝัง544digests และใช้guard/native read-onlyopenเดียวกับverifier.
WindowsPowerShell5.1ทดสอบไฟล์fixtureจริง: pristine/save-area/เปลี่ยนMBR+ESP+ท้ายprefix/
เปลี่ยน17ช่วงตรวจพบครบและเก็บเนื้อหาไม่เกิน16ช่วง; hash/payloadตรวจอิสระ,ไฟล์ไม่เปลี่ยน.
หลักฐาน [prefix-diagnostic-tests.json](wip/phase25/2026-10-24/prefix-diagnostic-tests.json).
Physical diagnosisยังNOT RUN. MyOS executable/imageไม่เปลี่ยน; reuseผลQEMU/buildเดิมตามขอบเขต.
