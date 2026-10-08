# MyOS project state
Updated: 2026-10-08 19:37 +07. /home/xytron404/myos,main.
Production source79f4639; prior evidence48181ed. Owner pushes only.
Fetched origin/main01f33ea19:31:05; ancestor of HEAD,local ahead4/no upstream diff.
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
## KVM and earlier experiments (details in HANDOFF)
- TCG native visual markers showed14..28s input latency; no TCG responsiveness
  claim from KVM results. Prior VM ledger/GS/ATA evidence remains banked in WIP.
- Owner authorized group kvm; appended16:15:10. Fresh non-root session access,
  API12/createVM OK. Normal image boot to shell/query-kvm enabled PASS16:19:32.
  No root QEMU/no WSL restart; explicit -accel kvm -cpu host,no TCG fallback.
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
- Audit786 baseline inputs: only16 expected changes. Existing resource helper
  unchanged (first audit expected it changed;preserved corrected audit). Observer
  host safety3 tests PASS; no guest payload reads. Manifest5 images exact.
-12681 terminal1 19:34:45:ten focused B/U runs passed; full stopped at host
  fixture compile because isolated copy omitted pinned third_party/compiler-rt.
  Normal5 hashes restored OK. Copied exact3 dependencies; expanded source audit
  783->786,all16 changed-file hashes unchanged; failed emutls compile now PASS.
-82911 LIVE19:36:20:complete test_system.py retry,not suffix; focused runs reused.
  Log integrated-full-system-retry.log. Full PASS/promotion remain pending.
  Logs integrated-*.log/status in root. Full regression uses unchanged TCG;
  native Cobalt acceptance used explicit non-root KVM,not a global test switch.
  No source/build edits there while running. Approval quota recovered19:03;
  earlier blocked launch never executed. Normal main images untouched.
## Next
- [Native acceptance report](PHASE33_NATIVE_WEB_ACCEPTANCE.md) separates actual
  five-part native web evidence from still-pending full regression/promotion.
- Poll82911 exact handle (12681 terminal). Native candidate pass
  alone does not close phases. Promote only exact audited/regression-tested code.
- Run required integrated regression,restore normal images,update docs/commit
  before closure. Detailed evidence in HANDOFF and docs/wip/native-pmm512.
- activity.log bounded48MiB; NEVER read through run_logged.py. No private keys.
