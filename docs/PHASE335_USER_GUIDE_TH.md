อัปเดต 10 ต.ค. 2026 18:20: จับ callchain รอบเอนจินหยุดได้แล้วในเส้นทางสร้าง tooltip; fixture native ยืนยันข้อจำกัดหน้าต่างเล็ก/หน้าต่างซ้อน แต่ยังไม่ได้แก้ production หรือยืนยันเสียงจากคุณ คู่มือยังเป็นฉบับทดลอง และ Phase33.5/33 ยังไม่ผ่าน

อัปเดต 10 ต.ค. 2026 17:21: คู่มือนี้ยังเป็นฉบับทดลอง ไม่ใช่คู่มือของ Phase ที่ตรวจรับแล้ว รอบวินิจฉัยหกครั้งไม่เกิด abort แต่ตารางไฟล์เต็มจริง ปัญหา abort ใน build ปกติยังไม่ได้แก้ ห้ามใช้ผลนี้ยืนยันว่าเสถียรหรือผ่าน Phase 33.5

# คู่มือทดลอง MyOS — YouTube และ Rickroll

**ฉบับร่างก่อนตรวจรับ — Phase33.5/33 ยังไม่เสร็จ**
ล่าสุดพบ Cobalt abort ในการเปิดซ้ำครั้งที่3 ของชุด di3oxi_6;
กำลังวินิจฉัยและต้องแก้/ทดสอบใหม่ก่อนออกคู่มือฉบับตรวจรับสุดท้าย
คุณลองเปิดแล้วพบ error ก่อนบูต ยังไม่มีผลรับรองภาพ/เสียง/controls จากคุณ
ดูผลจริงล่าสุดใน PHASE335_ACCEPTANCE.md และ PROJECT_STATE.md

## คำสั่งตั้งแต่เปิด PowerShell จนเข้า MyOS

1. เปิด **PowerShell** หรือ **Windows Terminal** ใน Windows แล้วพิมพ์:

```powershell
wsl.exe -d Ubuntu
```

2. เมื่อเห็น prompt ของ Ubuntu เช่น `xytron404@...:~$` ให้พิมพ์ทีละบรรทัด:

```sh
cd /home/xytron404/myos
python3 tools/run_logged.py -- python3 tools/run_phase335_manual.py build/phase334-public-g_wbd0r3
```

3. รอหน้าต่าง **QEMU / MyOS** เปิด แล้วคลิกในหน้าต่างนั้น คำสั่งต่อไปนี้พิมพ์ที่ `myos>` ภายใน MyOS ทีละบรรทัด:

```text
net up
mkdir /dev/shm
mem
elf cobalt cobalt provider https://www.youtube.com/watch?v=dQw4w9WgXcQ
```

หลังพิมพ์คำสั่ง Browser ให้รอโหลดเอนจินและหน้าเว็บโดยไม่พิมพ์คำสั่งซ้ำระหว่างรอ

4. รอ YouTube โหลด เลือกวิดีโอ Rick Astley ที่ถูกต้อง แล้วเลือก Settings → Quality → 144p ภายใน MyOS ดูและฟังเพลงอย่างน้อย30วินาที จากนั้นทดสอบ `K` pause/resume, `L` seek หน้า10วินาที และ `J` seek หลัง10วินาทีตามรายการด้านล่าง

5. เมื่อต้องการกลับ shell กด `Esc` ภายใน MyOS แล้วพิมพ์:

```text
mem
desktop
```

เปิดหน้าต่างบน desktop ด้วยเมาส์ กด `Q` กลับ shell แล้วเปิด Rickroll ใหม่ด้วยคำสั่ง `elf cobalt ...` ด้านบน เมื่อต้องการบูตใหม่ให้ปิดหน้าต่าง QEMU แล้วรันคำสั่งขั้นที่2 ใน Ubuntu อีกครั้ง

**แก้ error `Block node is read-only`:** ตัวเปิดใช้ temporary snapshot overlay ให้ QEMU IDE ทำงานได้โดยไม่แก้ไฟล์ boot/assets ต้นฉบับ ต้องใช้ตัวเปิดฉบับแก้แล้ว ไม่ใช้คำสั่งเก่าที่มี `readonly=on` บน IDE hard disk

หากต้องการตรวจเฉพาะการบูตก่อนเปิดหน้าต่าง ใช้คำสั่งนี้ใน Ubuntu:

```sh
python3 tools/run_logged.py -- python3 tools/run_phase335_manual.py build/phase334-public-g_wbd0r3 --check-boot
```

ผล `PASS passive normal MyOS boot` ตรวจเพียงถึง shell ยังไม่ยืนยันภาพเพลง/เสียง/controls หรือ Phase ผ่าน ตัวเปิดแต่ละครั้งจะพิมพ์ตำแหน่ง serial log ของ session ใหม่ และไม่เขียนทับหลักฐานรอบก่อน

## เตรียมระบบ
ใช้ WSL Ubuntu กับ repository `/home/xytron404/myos` และ WSLg ที่รองรับ
หน้าต่าง GTK/PulseAudio ให้ Windows เปิดเสียงและเลือก output device ที่ใช้งานจริง
ใช้ผู้ใช้ปกติที่เข้าถึง `/dev/kvm` ได้ ไม่ใช้ root
ต้องไม่มี QEMU ชุดทดสอบอื่นกำลังทำงาน และไม่ rebuild image ระหว่างทดลอง
ทุกคำสั่ง development ใช้ `python3 tools/run_logged.py --` ตาม AGENTS.md

Normal boot ที่ตรวจล่าสุดเป็น QEMU10.2.1/KVM/CPUhost/RAM1024MiB,
RTL8139 user network,SB16,800×600; provider VP9/Opus แบบ native underlay
Cobalt27.lts.3/Evergreen7.3.2/Starboard18,Mesa25.0.7,libvpx1.15.2,Opus1.5.2
รายละเอียด SHA256 อยู่ในรายงาน ไม่ให้ใช้ provider เก่าที่พบ sync failures

## เปิดหน้าต่าง MyOS สำหรับตรวจรับด้วยตนเอง
ชุด g_wbd0r3 เป็น snapshot normal boot และ native application assets ที่ใช้
ทดลองครั้งก่อน;ไม่มี movie fixture สำหรับแทน YouTube เครื่องมือจะตรวจ hash
boot/assets/runtime/core ก่อนเปิด VM ทุกครั้ง การเปิดนี้ยังไม่บันทึกว่า Phase ผ่าน

```sh
cd /home/xytron404/myos
python3 tools/run_logged.py -- python3 tools/run_phase335_manual.py build/phase334-public-g_wbd0r3
```

คำสั่งนี้เปิด QEMU GTK และเสียง PulseAudio เท่านั้น ไม่เปิด host browser/player
ขั้นตอน fetch,TLS,JavaScript,demux,decode,render และ audio อยู่ใน MyOS
หลังแก้ปัญหาเปิดซ้ำ จะอัปเดต bundle/provider ที่แนะนำในคู่มือฉบับสุดท้าย

## เปิด Browser ภายใน MyOS และไปยัง Rickroll
ที่ `myos>` พิมพ์ทีละบรรทัด:

```text
net up
mkdir /dev/shm
mem
elf cobalt cobalt provider https://www.youtube.com/watch?v=dQw4w9WgXcQ
```

นี่เป็น Browser Cobalt แบบ Ring3 ของ MyOS ส่วนไอคอน Browser รุ่นข้อความ
บน desktop เป็นอีก application และยังเล่น YouTube แบบนี้ไม่ได้
รอหน้าโหลด หากเป็นโฆษณาให้รอหรือกด Skip ad ใน YouTube ด้วยเมาส์ MyOS
อย่านับเวลาของโฆษณาหรือวิดีโออื่นเป็น Rickroll
ตรวจชื่อ Rick Astley – Never Gonna Give You Up และ VideoID `dQw4w9WgXcQ`
ใน Stats for nerds เลือก Settings→Quality→144p และดูค่าที่เลือกจริง
ชุดที่ตรวจพบ source256×144@25,VP9(278)/Opus(251),WebM/SABR,
ภาพ native presentation160×90 ขยายในหน้า800×600 มี dropped frames ที่รายงานไว้
ตำแหน่งเมนูเปลี่ยนได้ อย่าอาศัย click coordinates ที่บันทึกไว้โดยไม่ดูหน้า

## ตรวจภาพ เสียง และ controls จาก session เดียวกัน
1. ดูและฟัง Rickroll อย่างน้อย30วินาที บันทึกเวลาเริ่ม/สิ้นสุด
   ต้องเห็นภาพเคลื่อนไหวและได้ยินเพลง ไม่ใช้ screenshot/title/RMS แทนการฟัง
2. กด `K` เพื่อ pause รอ10วินาที ตรวจทั้งภาพและเสียงหยุด
3. กด `K` อีกครั้ง ตรวจภาพและเสียงเล่นต่อสัมพันธ์กัน
4. กด `L` เพื่อ seek ไปข้างหน้า10วินาที ตรวจ timeline เปลี่ยนและ scene/เสียงใหม่
5. กด `J` เพื่อย้อนหลัง10วินาที ตรวจตำแหน่งเปลี่ยนจริง ไม่มีภาพหรือเสียงเก่าค้าง
   ถ้ามี buffering ให้บันทึกช่วงเวลาและตรวจว่ากลับมาเล่นจากตำแหน่งใหม่ได้
6. คลิกโลโก้ YouTube เพื่อออกจาก player แล้วตรวจเสียงหยุด กด `Esc` กลับ shell
7. พิมพ์ `mem`,จากนั้น `desktop` เปิดหน้าต่างด้วยเมาส์ และกด `Q` กลับ shell
   เปิดวิดีโออีกครั้งด้วยคำสั่ง Browser ด้านบน
8. ทำเปิด–ปิดซ้ำหลายครั้ง ตรวจว่าทุกรอบเล่นได้ ไม่มี abort/freeze
   จด pages/heap/file-cache retention จาก log ไม่ถือว่า heap เพิ่มเป็นผ่านโดยอัตโนมัติ
9. ปิดหน้าต่าง QEMU แล้วรันคำสั่ง manual อีกครั้งเพื่อบูตใหม่
   ทำซ้ำการเล่น≥30วินาที,pause/resume,seek หน้า–หลัง,ปิด/เปิดและ desktop
   อย่าใช้ save-state หรือใส่คลิปที่บันทึกไว้เป็น input

## หลัง offline หรือเกิดปัญหา
ถ้า YouTube แสดง offline ให้กด Retry ใน MyOS เมื่อเครือข่ายกลับมา
ตรวจชื่อและ VideoID ใหม่ก่อนนับ Rickroll:รอบ recovery ก่อนหน้าเล่นโฆษณา
Spandau Ballet จึงไม่ใช่หลักฐาน Rickroll หลัง recovery
หากเกิด FATAL/USER SIGNAL,ค้าง,เสียงไม่หยุด,seekผิด หรือภาพ/เสียงไม่ตรง
บันทึกเวลา/อาการ/serial log และถือว่ารอบนั้นไม่ผ่าน ไม่ล้าง cache เพื่อซ่อนปัญหา
ไม่แก้ระบบเวลา,ไม่ใช้ host decoder/transcoder และไม่ใช้ MYV1 แทนวิดีโอจริง

## ส่งผลตรวจรับและตรวจหลักฐาน
ส่งวันที่,commit/build/provider,videoID,เวลาของ pause/resume/seek,
ผลเพลงและความสัมพันธ์ภาพเสียง,ผลเปิดซ้ำ/บูตใหม่ และปัญหาที่พบ
การรับคำสั่งทำงานของคุณไม่เท่ากับการรับรองว่าการทดลองผ่าน
Phase33.5/33 ปิดได้เมื่อทุกข้อใน PHASE33.md ผ่านจริง รวม technical stability

หลักฐานต้นฉบับอยู่ `build/phase334-public-*`;สำเนาปลอดข้อมูลลับและ SHA256
อยู่ `docs/wip/phase335/` Raw PCAP/log/assets เก็บ private ใน ignored build
อย่าเผย signed URLs,cookies,headers หรือ keys
วิดีโอ screen+PCM AVI เป็นการจัดเวลาจาก WAV-byte polling แบบประมาณ
PNG/WAV/timeline ต้นฉบับและ native hardware clock เป็นหลักฐานอ้างอิง
เสียงที่บันทึกเป็น native mono11025Hz U8;เสียงสดจาก WSLg/Pulse ยังรอคุณยืนยัน

ไม่มีการ push อัตโนมัติ คุณเป็นผู้ push ทุกครั้ง

## แบบฟอร์มจดผลเมื่อพร้อมทดลอง

ใช้แบบฟอร์มนี้หลังระบุ build ที่ทดสอบแล้ว ไม่ใส่คำว่า “ผ่าน” ไว้ล่วงหน้า

| รายการ | ผลที่คุณเห็น/ได้ยิน และเวลา |
|---|---|
| วันที่และ commit/build/provider | |
| Video ID และชื่อวิดีโอ | |
| ภาพเคลื่อนไหวพร้อมเพลง ≥30 วินาที | |
| Pause: ภาพและเสียงหยุด | |
| Resume: ภาพและเสียงเล่นต่อสัมพันธ์กัน | |
| Seek ไปข้างหน้า: ตำแหน่งใหม่ ไม่มีภาพ/เสียงเก่าค้าง | |
| Seek ย้อนหลัง: ตำแหน่งใหม่ ไม่มีภาพ/เสียงเก่าค้าง | |
| ปิด player แล้ว desktop/shell ใช้งานได้ | |
| เปิดใหม่ทุกครั้งเล่นได้ | |
| บูตใหม่แล้วทำซ้ำครบ | |
| เสียงสดจากหน้าต่าง MyOS | |
| ปัญหาหรือ buffering ที่พบ | |

ผลที่ยังตรวจไม่ได้ให้เขียนว่า “ยังไม่ได้ตรวจ” แทนการอนุมานจาก log
