# MyOS project state
Updated: 2026-10-08 14:15 +07. /home/xytron404/myos,main.
Source c04ff93 descriptor/heap/queue promotion; prior evidence7f527ea.
origin/main a746889 fetched09:49:18 unchanged; owner pushes only.
## Goal / acceptance
- Goal ACTIVE; Phase33.1/33.2 OPEN,no33.3/no push; YouTube unproven.
- Actual HTML/app.js/DOM42/timers/keys/click and controlled verified HTTPS,
  assets3 concurrent,wrong-host/expired/untrusted rejection observed.
  NOT public Internet/DNS proof. Responsive>=60s NOT accepted; visual backlog.
  Keyboard TOP/MIDDLE/TOP observed,not default browser/mouse-wheel scrolling.
- Imports242/242,missing0;15 media unavailable+9 POSIX ENOSYS explicit.
  Symbol counts/fixture PASS are NOT API completeness or phase percentages.
## Production baseline
- Full2759 and full SHM-pool28032 terminal0; affected scopes BOTH firmware PASS.
  Promotion36207 compared all tracked inputs,exactly4 pool changes; normal
 4 binaries byte-identical to tested tree. New kernel64/runtime128 below.
- PMM512MiB E820,upper-frame pointer guards,direct8MiB SHM,mmap hints;
  SHM64 separate child slots,live128; persistent64/snapshot bank unchanged.
  W^X/RELRO intact; no swap/demandpaging/COW; owner disks untouched.
- Normal hashes build/descriptor-promotion-normal.sha256 supersede SHM-pool hashes.
## Promoted descriptor/heap/queue slice
- Reservation fixes reproduced EMFILE orphan files; basic/concurrent4 launches
  BOTH firmware PASS. Runtime128 peak>=68; kernel32 still EMFILE on native.
- Kernel64 removes SHM Error24;3584 close PANIC remains unexplained. Later
 18313/59939 exact pages/shared0 closes do not erase failure.
- Full94832/79953 failed low-memory bound; normal restored; fault3 bss0x80eb0.
- Retry28044 TERMINAL0 12:16,pmm-syscall-R4sQZZ: process filetable heap+SB16
  softwarequeue heap;DMA/capacity unchanged. Fault0..6 builds/PCM BOTH PASS;
  fault3 bss0x70e90. Full system PASS,normal restore/final4 hashes all OK.
- Main promotion97035 terminal0 13:57:44: audited exact9 build/test changes;
  normal BIOS/UEFI build4 hashes match tested tree. No private launcher,
  renderer/provider,boot retirement or accounting implementation promoted.
- CPU59213 names dominant Chrome_InProcGpuThread. GPU61286 saved frame PCs
  include errno/memcpy/softpipe,NOT function time attribution; details HANDOFF.
## Private experiments (not production)
- Wordcopy73116 terminal0 11:52:04,b7lanazn: host guarded ASan/UBSan PASS;
  baseline/trial UEFI64 expected OOM rollback,UEFI256 staging PASS.
  Two native closes exact pages;heap454400/cache53248 after1/before2/after2.
  Assets/input/TLS observed,BUT final2 KeyC/ScrollMIDDLE/Timer80 lags PageUp.
  No measured speedup/60s acceptance; experimental provider NOT promoted.
- Minimal named-SHM exit reproduces production PANIC: legitimate named4KiB
  backing remains,fd closed,no aliases. Raw free-page equality is incorrect;
  NOT proven cause of earlier Cobalt panic. Original evidence retained.
- Private accounting compares free+unique SHM backing and alias refs,keeps
  failed-load checks strict. Named persistence/alias reap/unlink12 launches
  EACH BIOS/UEFI PASS.20408 terminal1 on stale private1MiB fixture only.
- Retry20750 current MAIN8MiB fixture: BIOS16/64 and UEFI64/256 all PASS,
 4 launches each.20750 terminal0 12:10:51,bog4_ll4:3 actual baseline-provider
  closes pages126671 exact/SHM0/aliases0/shell. Heap450304/454400/454400 tracks
  RAMFS cache49152/53248/53248; full accounting regression still pending.
  Cobalt must STILL show temporary SHM0/unlinked0; no full accounting suite yet.
- Clean accounting84234 LIVE14:14,pmm-syscall-R4sQZZ,only fs.h/ramfs.c/elf.c
  beyond promoted source. Builds, named-lifetime/shared-memory both firmware,
  focused BOTH firmware PASS; mandatory full still running,normal hashes pending.
- Present timing87369 LIVE14:14,private native xslpu938:24 completed800x600
  frames,median stripe TSC64850838 vs gap33809396435; median sampled fraction
 0.001895,NOT CPU%/time guarantee. Gap includes userspace/scheduling/waits;
  no basis for kernel-copy bottleneck claim. Diagnostic-only,not acceptance.
  EGL glFinish/present split source prepared but NOT compiled/run yet.
## Next
- Poll84234/87369 exact handles; do not mutate/restart on observation timeout.
- Inspect close/page/SHM metadata and actual frames/input latency; prove
  >=60s responsive interaction/relaunch on normal launcher before closure.
- Accounting needs relevant/full evidence before promotion. Fix actual latency
  and normal production launcher integration; source capacity alone is not5/5.
- Detailed public logs/sources/runners: docs/wip/native-pmm512 and HANDOFF.
  No private TLS keys,boot retirement or verbose private launcher promoted.
- activity.log bounded48MiB; NEVER read it through run_logged.py.
