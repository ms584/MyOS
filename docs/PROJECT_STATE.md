# MyOS project state
Updated: 2026-10-08 09:16 +07. Repo /home/xytron404/myos, main.
Source baseline3a99d70; origin/main a746889 fetched03:43; no push.
## Scope and honest status
- Phase33.1/33.2 OPEN. Actual native HTML/JS/DOM/timers/key/click observed;
  trusted HTTPS/concurrent assets/cert negatives now observed in private WIP.
  Responsive>=60s, scrolling/relaunch cleanup NOT accepted. No33.3/no push;
  YouTube unproven, Phase32/33 NOT PASSED.
- Strong paired imports242/242, registered240; missing0. Fifteen unavailable
  media +nine POSIX ENOSYS are explicit boundaries, NOT complete API semantics.
## Newly promoted full-tested runtime
- pmm-syscall-R4sQZZ full retry2759 TERMINAL_EXIT=0 at06:42:30. Mandatory
  test_system.py +six affected scopes BOTH BIOS/UEFI, normal restored.
- Main promotion78188 TERMINAL_EXIT=0 at08:56:12. Reviewed13 changed tracked
  inputs, copied exact candidate/new focused fixtures; FOUR normal binaries
  byte-identical to full-tested candidate. No diagnostic/boot experiment copied.
- PMM max512MiB (actual E820 usable pages only), metadata expanded after heap
  bootstrap; BIOS BSS unchanged. No swap, demand paging or COW.
- Safe syscall copy validates PMM-owned frames instead of stale256MiB cap.
  U/S, RW, canonical/range preflight retained. Actual280MiB per-page random/log,
  RO/NONE/supervisor/cross-page rejection/two-launch exact resources PASS both.
- Shared backing8MiB, direct reference aliases without temporary private pages;
  near-image best-effort mmap hints/pool, fixed/NOREPLACE protections preserved.
- Kernel32 file handles/runtime64 slots; advertised NOFILE128 upper bound.
  O_NOFOLLOW correct for symlink-free namespaces; EMFILE closes kernel handle.
  Server/socketpair fixtures use advertised limits, not old hardcoded counts.
- Prior combined SHM8/direct full47797+10 scopes BOTH firmware PASS. Old22674
  missing ABI cache and7014 missing core failures retained, NOT rewritten PASS.
- Hash entry: build/pmm-syscall-promotion-normal.sha256. Previous import-image
  promotion hashes are historical and superseded. Data disks untouched.
## Actual native Cobalt / private WIP
- hfcgru2i/n693w3zt show actual HTML/DOM42, timers, key/click. Long refresh final
  Timer96/Clicks2 versus156 timers/click3: substantial visual lag remains.
- Private normal boot-probe retirement keeps PROCESS_TESTS=1 stress unchanged.
  BIOS/UEFI focus: counters run then stop, PIT/shell live, pages exact.
  c3sthg1e DOM74.62s/background95s vs126.49/186s, keys a/b/c, NO clicks this run;
  final Timer114 vs131. Heap409088->454144; warm cache/relaunch accounting pending.
  Retirement NOT promoted; CPU-clock runner lacked its test file, NOT CPU PASS.
- Native TLS IP attempt3p379730: ERR_CERT_NON_UNIQUE_NAME,0HTTP, stopped only
  after actual navigation failure. DNS SAN fixture uses enforcement=true,
  public test CA through actual hashed cert assets, DNS-only host mapping.
- TLS58269 terminal0 observation06:11:15,duv0icq3: ONE valid HTTPS GET55.86s;
  no /app.js, DOM, blue pixels or concurrent assets. NOT HTTPS acceptance.
- TLS snapshot91255 terminal0 observation08:56:48,cb5eie3f: one GET54.01s.
  At150s all user tasks blocked (some timed), kernel boot task runnable;
  snapshot is not proof of deadlock or responsive timing. Pages/threads stop;
  heap409344->450304, cleanup still unaccepted. Keys never recorded/committed.
- TCP trace46944 terminal0; verbose18956 terminal0 showed actual Cobalt cancels
  navigation on missing CSP header, NOT a proven TLS/readiness deadlock.
- CSP fixture-only fix77831 terminal0 at09:15:04, bundle c4sx6f02: actual
  native HTTPS HTML/external app.js/DOM42,93 timer beacons, key a/click1;
  three assets loaded, peak concurrent3, three certificate-error beacons,
  ZERO HTTP on wrong-host/expired/untrusted servers. No TLS/CSP bypass.
- Real final screenshot: DOM42,Timer77,Clicks1,Rejected certificates3/3.
  Timer93 in journal: visual lag remains, not responsive60s acceptance.
  Escape: pages reclaimed/all threads stopped, heap409344->450304 (+40960).
  No relaunch proof yet. Controlled disposable CA/DNS map, NOT public Internet.
## Next and artifacts
- Diagnose visual backlog; repeat input/scroll with timed actual frames and
  warm close/relaunch accounting. CSP failure/fix is public evidence below.
- Resolve visual latency/PS2 motion, scrolling, >=60s responsiveness and warm
  close/relaunch resources. Do not close phases on imports/fixture results.
- HANDOFF.md; docs/wip/native-pmm512 preserves slices/logs/actual screenshots.
  Private TLS keys stay in private build dirs; never copy whole TLS bundles.
- activity.log bounded48MiB; NEVER read it through run_logged.py. Goal ACTIVE.
