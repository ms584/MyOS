# MyOS project state
Updated: 2026-10-08 15:45 +07. /home/xytron404/myos,main.
Production source79f4639; prior evidenceb99b520. Owner pushes only.
Remote main08d90f7 observed14:21:53; tracking origin/main may be stale.
## Acceptance
- Goal ACTIVE; Phase33.1/33.2 OPEN,no33.3/no push; YouTube unproven.
- Native HTML/app.js/DOM42/timers,keyboard/click,controlled verified HTTPS,
  assets3 concurrent and wrong-host/expired/untrusted rejection observed.
  NOT public Internet/DNS proof. Responsive>=60s NOT accepted; visible backlog.
- Imports242/242,missing0;15 media unavailable+9 POSIX ENOSYS explicit.
  Counts/fixture PASS are NOT API completeness or phase percentages.
## Production / verified baseline
- PMM512MiB E820,upper-frame guards,direct8MiB SHM,mmap hints; separate64 SHM
  child slots,live128; persistent64/snapshot format unchanged. W^X/RELRO intact.
  No swap/demandpaging/COW. Owner disks untouched.
- Kernel64/runtime128 descriptors,reserve before create/truncate; process FS
  table and SB16 softwarequeue heap. Full28044 and affected BOTH firmware PASS.
- Resident accounting free+unique SHM backing and alias refs after success;
  failed-load checks strict. Legitimate named4KiB inode caused reproduced raw
  page-count PANIC,NOT established cause of older native close panic3584.
- Clean84234 full system terminal0 14:44:36. Promotion53593 terminal0 14:51:10:
  main4 binaries match full-tested3-source candidate; integrated named lifetime
  12 launches EACH BIOS/UEFI PASS. Normal manifest:
  build/resident-accounting-promotion-normal.sha256; all4 hashes OK,normal images.
## Experiments (NOT production)
- Native launcher/boot retirement/resolver/verbose profiling NOT promoted.
  Wordcopy and GS errno fastpath did not establish native latency acceptance.
  GS3238 both firmware oracle/thread tests and2 native closes PASS; restored.
- Serial EGL timing puts long interval before glFinish/present; not function
  CPU attribution. Kernel print diagnostic damaged framebuffer,preserved failure.
- Atomic56212 timrxrfo terminal0:157 paused CPU PCs,vm_forget34/prune23; noCPU%.
  Task/tid invalid due same-name current symbol; registers valid. Final Keyc/
  MIDDLE despite PageUp => backlog. Sources/evidence/disassembly in WIP docs.
## Indexed VM ownership trial
- Expected bucket-chain lookup replaces repeated whole-ledger scans; dense
  pages + links +2*capacity buckets in ONE heap allocation,20B/slot vs8B before.
  No struct/BSS fields. Duplicate SHM physical frames remain separate references;
  swap removal fixes moved indices; grow publishes only after complete rehash.
- Host ASan/UBSan PASS15:08:39: collision/duplicate/model20000 operations,
  grow OOM rollback,65536 pages,no heap leak. Actual process.c anonymous commit
  and immutable file-map rollback/rights tests PASS15:13:08. Immutable harness
  shared hooks assert unreachable; mutable SHM tested natively,not simulated.
- Native10751 terminal0 15:17:39,oeki2uua:2 launches202.34/222.50s,315requests,
  peakassets3,14inputs,final Timer107/PageUp/TOP; both clean closes/SHM0/shell.
  Original process/header absence/normal4 hashes restored; no speedup claim.
- Clean18146 terminal1: desktop snapshot before presentation marker; preserved.
  VM hint/shared/lifetime BOTH firmware PASS. Close-frame fence fixes host-test
  .05s assumption,unchanged pixel assertion. Focused desktop retry PASS;65645
  LIVE15:45 PID582523 verified: full retry now resolver; normal/hash pending.
- Combo54571 TERMINAL0 15:30:51: ledger+GS,oracle BOTH,2 native launches
  192.30/222.44s,310requests,assets3,14inputs; both clean closes/SHM0/shell.
  Original3 sources restored/helper removed/normal4 hashes OK. No latency PASS.
- Custom-FS GS test14461 terminal0 15:26:24,24 workers/launch,2 launches EACH
  BIOS/UEFI: custom FS magic preserved,errno independent,reuse/exact cleanup.
- Frozen SAME combo images10136 terminal0 15:30:03: existing native TLS and
  cross-signal tests BOTH firmware PASS;16 signal launches each. No promotion.
- Visual20261 ended before execution: frozen image/auto-runtime identity mismatch.
  Retry35904 terminal0 15:44:31,bz7q6fof; exact runtime from successful combo
  asset extent,no identity bypass. Actual JS markers read from PNG every1s:
  key->visible21.00..27.88s,frame plateau<=9.24s;164requests,assets3,clean close.
  No responsiveness PASS. Full regression concurrent; not controlled speedup.
  Frozen-kernel nm metadata unverified: exclude host resource capture values.
## Next
- Poll65645/PID582523 exact handle; never restart on observation timeout.
  Inspect actual final input/visual timing/close,then audit and promote only with
  required evidence. Controlled comparison needed before speedup claim.
- /dev/kvm read-only probe EACCES,user outside group kvm; owner choice pending,
  no system change,nested KVM unproven. Not a blocker to code work.
- Integrate audited native launcher and prove60s responsive/relaunch acceptance
  before closure. Detailed evidence in HANDOFF and docs/wip/native-pmm512.
- activity.log bounded48MiB; NEVER read through run_logged.py. No private keys.
