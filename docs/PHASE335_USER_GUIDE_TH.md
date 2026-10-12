# คู่มือทดลอง MyOS — YouTube และ Rickroll

อัปเดต 12 ตุลาคม 2026: Phase32,33.5 และ33 ตรวจรับแล้วบน MyOS/QEMU ตามรายงาน
PHASE335_ACCEPTANCE.md คุณยืนยันเส้นทางค้นหา เลือก เล่นพร้อมเสียง และยอมรับ
A/V sync ไม่เกิน500 ms; ค่าสูงสุดสามรอบ204.618/140.037/87.339 ms
ช่วงโหลด Browser อาจใช้หลายนาที ยังมี rebuffer/เฟรมตกและ UI ตอบสนองช้า
เกณฑ์500 ms เป็นการสัมพันธ์ภาพ/เสียง ไม่ใช่เวลารอโหลดหรือ latency ของเมาส์

## 1. เปิดจาก Windows

เปิด Windows Terminal เลือก PowerShell แล้วรัน:

```powershell
wsl.exe -d Ubuntu
```

เมื่อเห็น prompt Ubuntu ให้รันทีละบรรทัด:

```bash
cd /home/xytron404/myos
python3 tools/run_logged.py -- python3 tools/run_phase335_manual.py build/phase335-pointer-boundary-manual --qmp --sdl-diagnostic --sdl-show-host-cursor --network-evidence
```

จะเปิด QEMU พร้อมเสียง และสร้าง manual-session-* ใน build/phase335-pointer-boundary-manual
Terminal แสดงตำแหน่ง serial log ให้ปล่อย Terminal เปิดไว้
guest ใช้ RAM 1024 MiB และบูตด้วย snapshot ไม่มีการตรวจรับโดยอัตโนมัติ
ถ้าขึ้นว่ามี QEMU ทำงานอยู่ ให้ปิด VM เดิมก่อน ห้ามเปิดซ้อนกัน
หากขึ้น PulseAudio timeout หรือ WSL เปิดไม่ได้ ให้หยุดและส่งข้อความผิดพลาด
อย่าลบ ext4.vhdx หรือ shutdown ขณะงานทดสอบทำงาน

## 2. เตรียม MyOS และเปิด Browser

คลิก QEMU เมื่อเห็น myos> แล้วพิมพ์ทีละคำสั่ง:

```text
net up
mkdir /dev/shm
mouse absolute
mem
elf cobalt cobalt provider https://www.youtube.com/watch?v=dQw4w9WgXcQ
```

ทุกขั้น network/TLS/JavaScript/demux/decode/render/audio ทำใน MyOS
Windows/WSL ใช้เพียง build, virtual hardware และบันทึกหลักฐาน
ตั้ง Quality เป็น 144p หากยังเป็น Auto รอภาพเคลื่อนไหวและเสียงจริง
หน้าเว็บหรือวงกลมหมุนยังไม่ถือว่าเล่นได้
เมาส์ควรตามตรงก่อนและหลังคลิก ไม่ต้องสลับ Ctrl+Alt+G ซ้ำ
Ctrl+Alt+G ใช้ปล่อยการจับเมาส์เมื่อกลับ Windows

สำหรับ search/selection ใช้คำสั่งนี้แทน direct URL:

```text
elf cobalt cobalt provider https://www.youtube.com/
```

คลิก Search พิมพ์ Rick Astley Never Gonna Give You Up กด Enter และเลือก Rickroll
ตรวจ URL/video ID ว่าเป็น dQw4w9WgXcQ; คลิปอื่น โฆษณาและ thumbnail ไม่ผ่าน
Phase32 ได้ตรวจ search → เลือก → เล่นแยกต่างหากและคุณยืนยันแล้ว

## 3. ตรวจภาพ เสียง และ controls

1. ดูและฟัง Rickroll จริงอย่างน้อย 30 วินาทีใน session เดียวกัน
2. กด K: ทั้งภาพและเสียงต้องหยุด กด K อีกครั้ง: กลับมาเล่นสัมพันธ์กัน
3. กด L: ไปข้างหน้า 10 วินาที ตรวจเวลาและท่อนเพลงจริง
4. กด J: ย้อนหลัง 10 วินาที ตรวจภาพ/ท่อนใหม่ ไม่มีภาพหรือเสียงเก่าค้าง
5. กด Esc กลับ shell แล้วรัน mem ต้องตอบสนอง
6. รัน desktop เปิด Demo A ด้วยปุ่ม 1 แล้วกด Esc กลับ shell
7. เปิดวิดีโอใหม่ด้วยคำสั่ง Browser เดิม ต้องเล่นได้ ไม่มี crash
8. เปิด/ปิดซ้ำอีกอย่างน้อย 2 ครั้ง ตรวจความลื่นและการคืน RAM
9. ปิด QEMU แล้วรันข้อ 1 ใหม่เพื่อบูตสด ทำ playback/controls ซ้ำ

หากค้าง กระตุก ภาพดำ เสียงค้าง หรือเปิดไม่สำเร็จ ให้รายงานพร้อมเวลาที่เกิด
อย่าประกาศผ่านเพียงเพราะ serial log มี PASS

## 4. หลักฐานและผลที่ต้องรายงาน

เก็บ session ต้นฉบับ พร้อม serial log, QEMU command และ build hashes
network.pcap มีข้อมูลเครือข่ายและ HTTPS เข้ารหัส ให้เก็บส่วนตัว
ไม่เผยแพร่ raw log หรือ signed media URL
รายงานวันที่ URL/build, ระยะเวลาที่ฟังจริง, pause/resume/seek, เปิดซ้ำและบูตใหม่
รวมอาการสะดุดและเวลารอ Browser/วิดีโอ

ตัวบันทึกเครื่องมือเก็บ native PNG/WAV/timeline จาก session เดียวกัน
WAV อาจไม่รวมเวลาที่อุปกรณ์หยุดส่งเสียง ต้องใช้ timeline ต้นฉบับจับคู่เวลา
PCM ไม่ยืนยันว่าเป็นเพลงถูกต้องหรือเสียงสะอาด ต้องตรวจการฟังด้วย
เครื่องมือจะไม่อ้างว่าคุณตรวจรับหากยังไม่มีผลที่คุณยืนยัน

## 5. ดูหลักฐานรอบตรวจรับ

ใน Ubuntu:

```bash
cd /home/xytron404/myos
cat docs/wip/phase335/checkpoint22/pointer-boundary/final-acceptance.json
```

ใน File Explorer ใช้ตำแหน่งนี้เพื่อดู recording ต้นฉบับ:

```text
\\wsl.localhost\Ubuntu\home\xytron404\myos\build\phase335-pointer-boundary-manual\manual-session-uwzd1yy5\native-recording-mkl4wcyt
```

มีภาพPNG,เสียงaudio.wav,timelineในevidence.json และวิดีโอรีวิว
launch-1-screen-audio.avi รอบ70วินาที:pause40s,resume45s,seekหน้า55s/หลัง62s
AVI จับเวลาโดยประมาณ;ค่าคลาดเคลื่อนจริงอ่านจาก native clock ไม่ใช่ AVI
ต้นฉบับ recording/PCAP อยู่ในเครื่อง ไม่ส่งไฟล์ก้อนใหญ่หรือ signedURL ขึ้นGitHub

หากชุด build ไม่อยู่ในเครื่องอื่น ต้องเตรียม pinned Cobalt/assets/provider ตาม
เอกสาร native runtime ก่อน;คำสั่งนี้อ้างชุดที่ตรวจรับในเครื่องนี้ ไม่ใช่การ
ดาวน์โหลด dependencies โดยอัตโนมัติ การตรวจรับ QEMU ยังไม่ยืนยันบูต USB จริง
