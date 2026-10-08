# MyOS project state
Updated: 2026-10-08 09:51 +07. /home/xytron404/myos, main.
Source3a99d70, docs6f743f5; origin/main a746889 fetched09:49:18, unchanged.
## Goal / acceptance
- Goal ACTIVE; Phase33.1/33.2 OPEN, no33.3/no push; YouTube unproven.
- Native HTML/JS/DOM42/timers/key/click and controlled verified HTTPS observed.
  Three concurrent assets and wrong-host/expired/untrusted rejections observed.
  NOT public Internet/DNS proof. Responsive>=60s/scrolling still NOT accepted.
- Imports242/242, missing0;15 unavailable media +9 POSIX ENOSYS explicit.
  Counts/fixtures are NOT complete API semantics or closure percentages.
## Production baseline (unchanged this span)
- Full retry2759 terminal0 06:42:30 +six affected scopes BIOS/UEFI passed.
  Main promotion78188 terminal0 08:56:12; four normal binaries byte-identical.
- PMM512MiB actual E820 frames/upper-frame syscall checks; direct8MiB SHM,
  nonfixed mmap hints, kernel32/runtime64 handles, nofollow/EMFILE cleanup.
  W^X/RELRO/user-pointer guards retained. No swap/demandpaging/COW.
- Current normal hashes: build/pmm-syscall-promotion-normal.sha256.
  Old import-image hashes superseded; owner data disks untouched.
## Native HTTPS / relaunch evidence
- CSP fixture fix77831 terminal0,c4sx6f02: actual external app.js,DOM42,
  timer93,key/click,assets3/3 peak3,cert negatives3/3,0 invalid HTTP.
  Final actual Timer77/Clicks1: visual backlog, NOT responsive acceptance.
- Same-VM two-launch85764 terminal0,0dgifhbw: DOM/key a,b,c/click both;
  pages exact/all threads stopped/shell returned. Second asset2 onerror;
  native SHM creation ENOSPC. Heap409344->454400->450304, NOT full PASS.
- Fixed VA-aware resource observer81191 terminal0 09:37:50,p1rh6c7a:
  after1/before2/after2 heap454400 exact; no shared nodes after closes.
  RAMFS cache capacity8192->53248 accounts45056; mount catalogue36608
  explains initial before-loader heap372736 vs loader409344.
- Earlier psoj5ki3 zero heap totals INVALID (physical read of virtual heap).
  Raw evidence/analyzer retain limitation; never cite zeros as reclamation.
## SHM inode constraint / private candidate
- Exact kernel trace62739 terminal0 09:41:20,qpkptyt4: inode used64,
  shared24/unlinked25 at ENOSPC. Active snapshot62 is post-failure race,
  not exact occupancy at failure. File cache and SHM share original64 slots.
- Private4-file fix: FS_NODES64 persistent/snapshot unchanged; separate64
  /dev/shm child slots,128 live nodes, all live traversal/close/import checks.
  Namespace statvfs quotas separate; snapshot still64 slots, no disk migration.
- Focused filesystem PASS including both pools full, last-close/reuse,
  snapshot rejects high pinned fd, closed ephemeral cleanup/exact resources.
  First wrapper11944 exit2 wrong postcondition log filename, NOT suite failure;
  retained status. Actual framebuffer-64M marker validated, then resumed full.
- Full28032 RUNNING since09:43:23 in /home/xytron404/pmm-syscall-R4sQZZ;
  mandatory test_system +shared_memory/statvfs/fs_runtime_lifetime BIOS/UEFI.
  Restore trap active. Do NOT mutate candidate/restart while live.
- Native pool91757 RUNNING since09:43:46,cobalt-closed-imports-xnVZeH,
  bundle25nzgz6j, two launches with idle VA-aware resource snapshots.
  First assets3/3/keys3/cert negatives3,0 SHM ENOSPC; NO click callback.
  Second live at09:48; inspect final evidence, don't infer PASS from exit0.
  Full suite runs concurrently: timing not isolated performance acceptance.
## Next
- Inspect SAME live28032/91757 handles; only promote after full+focus checks.
  Then fix actual mouse-feedback delivery, scrolling and visual backlog;
  prove >=60s interaction plus bounded warm relaunch on normal launcher.
- Docs/wip/native-pmm512 holds source slices/runners/public evidence only.
  No private TLS keys. No boot-retirement/verbose diagnostic launcher promoted.
- activity.log bounded48MiB; NEVER read it through run_logged.py.
