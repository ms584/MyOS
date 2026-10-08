# MyOS project state
Updated: 2026-10-08 16:40 +07. /home/xytron404/myos,main.
Production source79f4639; documentation checkpointf34228c. Owner pushes only.
Remote main08d90f7 observed14:21:53; tracking origin/main may be stale.
## Acceptance
- Goal ACTIVE; Phase33.1/33.2 OPEN,no33.3/no push; YouTube unproven.
- Native HTML/app.js/DOM42/timers,keyboard/click,controlled verified HTTPS,
  assets3 concurrent and wrong-host/expired/untrusted rejection observed.
  NOT public Internet/DNS proof. Production integration/full acceptance pending.
- Imports242/242,missing0;15 media unavailable+9 POSIX ENOSYS explicit.
  Counts/fixture PASS are NOT API completeness or phase percentages.
## Production / verified baseline
- PMM512MiB E820,upper-frame guards,direct8MiB SHM,mmap hints; separate64 SHM
  child slots,live128; persistent64/snapshot format unchanged. W^X/RELRO intact.
  No swap/demandpaging/COW. Owner disks untouched.
- Kernel64/runtime128 descriptors,reserve before create/truncate; process FS
  table and SB16 softwarequeue heap. Full28044 and affected BOTH firmware PASS.
- Resident accounting free+unique SHM backing and alias refs after success;
  failed-load checks strict. Named4KiB inode legitimately survives creator exit.
- Clean84234 full terminal0 14:44:36. Promotion53593 terminal0 14:51:10:
  main4 binaries match full-tested3-source candidate; named lifetime12 launches
  EACH BIOS/UEFI PASS. Normal manifest resident-accounting-promotion-normal.sha256
  all4 hashes OK16:21:13. No experimental sources promoted.
## Banked experimental evidence
- Indexed ownership ledger: duplicate/collision/model/OOM host ASan/UBSan PASS;
  actual process anonymous/file rollback/rights PASS; clean full retry65645
  terminal0 16:03:22,normal4 hashes OK,audit490 inputs PASS. Not promoted.
- GS private identity leaves FS/ELF TLS unchanged; custom-FS24workers/reuse,
  TLS and cross-signal tests BOTH firmware PASS. Batched prune host rollback/
  rights and shared/named lifetime BOTH firmware PASS. Still experimental.
- TCG native visual markers proved14..28s input latency despite clean closes.
  Wordcopy A/B98465 terminal0:16..25s vs20..27s,one launch each; not responsive.
- Atomic raw PCs matched byte-identical frozen image/ELFs;55 post-HLT idle,
  54 exact syscall-entry,6 memcpy; NOT CPU percentages or task attribution.
  Details/failed attempts retained in HANDOFF and docs/wip/native-pmm512.
## KVM and ATA trial (not production)
- Owner authorized group kvm; appended16:15:10. Fresh non-root session access,
  API12/createVM OK. Normal image boot to shell/query-kvm enabled PASS16:19:32.
  No root QEMU/no WSL restart; explicit -accel kvm -cpu host,no TCG fallback.
-63295 k4zzj9e7: first visible583.39s,key a visible<=1.015s; only19s visible
  before600s limit.51requests/assets3,clean close602.12s. Second launch stopped
  deliberately SIGINT16:30:44; terminal-2,NOT acceptance or guest crash.
- ATA root /home/xytron404/ata-rep-read-S8iLch: only read transfer uses256-word
  REP INSW,unchanged status/IRQ/write guards. Image9858e435...,runtime08945598.
  Asset-v2/v1 KVM BIOS40112 and UEFI89858 terminal0: large/readonly/relaunch/
  exact resources PASS. Missing isolated font fixed before UEFI retry.
-66375 TERMINAL0 16:38:49,ia30b2lw:2 launches215.54/215.95s,315requests,
  assets3;first visible95.38/95.75s,120s observation each. All10 keys visible
  <=1.312s;click/scroll,timers,cert negatives and both closes/resident exact PASS.
  Heap409344->450304->450304 bounded cache. Audit655 inputs: only ATA differs
  from matched native baseline. Host RAM-peak inventory NOT captured yet.
## Next
- Banked native KVM evidence is NOT an integrated production/phase PASS.
- Audit native launcher before integration: private nine-argv layout/owned
  executable path,controlled DNS mapping,jitless and verbose diagnostics are NOT
  yet production. Preserve verifier/W^X; do not copy private diagnostics blindly.
- Run required integrated regression,restore normal images,update docs/commit
  before closure. Detailed evidence in HANDOFF and docs/wip/native-pmm512.
- activity.log bounded48MiB; NEVER read through run_logged.py. No private keys.
