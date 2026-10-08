# MyOS project state
Updated: 2026-10-08 19:13 +07. /home/xytron404/myos,main.
Production source79f4639; prior evidence48181ed. Owner pushes only.
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
-66375 TERMINAL0 16:38:49,ia30b2lw: private native2launches,315requests,
  keys<=1.312s,click/scroll/TLS negatives/120s each/both exact closes observed.
## Clean integration (NOT main source yet)
- Root /home/xytron404/native-integrated-XZBHuo; clean production-derived source,
  no private diagnostics/name magic. Explicit elf cobalt CORE PROVIDER URL
  [HOST=IPv4]; jitless JS/W^X,HTTPS enforced; optional routing never trust bypass.
  Shared argv9slots/name584; legacy pair-starboard counts unchanged. All elf_start
  constructors memset full struct. Host route ASan/UBSan and BIOS/UEFI builds PASS.
-88404 TERMINAL0 19:11:07,h35djree:clean native2launches222.03/211.34s,
  marker spans118.78/119.69s;all10 keys<=1.328s. DOM42,timers,click/scroll,
  asset concurrency3,cert rejects3/3 each,both closes/resident exact PASS.
  PNG-rechecking verifier PASS19:12:08; heap450304->450304 after warm close.
  PMM sampled peak279449600bytes,420samples,pause3.349s;not exact peak/RSS,
  includes kernel/residents,excludes reserved heap backing. Before/after exact.
- Audit783 baseline inputs: only16 expected changes. Existing resource helper
  unchanged (first audit expected it changed;preserved corrected audit). Observer
  host safety3 tests PASS; no guest payload reads. Manifest5 images exact.
-12681 LIVE19:12:40:GS/custom-FS/shared/named lifetime/executable path BOTH
  firmware,then complete test_system.py. Logs integrated-*.log/status in root.
  No source/build edits there while running. Approval quota recovered19:03;
  earlier blocked launch never executed. Normal main images untouched.
## Next
- Poll12681 exact handle;inspect failures before retry. Native candidate pass
  alone does not close phases. Promote only exact audited/regression-tested code.
- Run required integrated regression,restore normal images,update docs/commit
  before closure. Detailed evidence in HANDOFF and docs/wip/native-pmm512.
- activity.log bounded48MiB; NEVER read through run_logged.py. No private keys.
