# Phase 33.1–33.2: native web acceptance, 2026-10-08

Status: Phase33.1/33.2 native web gate PASSED on the tested QEMU/KVM profile.
Complete system regression82911 exited0 at20:18:35; main promotion76944 exited0
at20:20:04. Source commit9aece1e and all five normal artifacts are byte-identical
to the tested candidate. This closes the coupled native web gate,not Phase32/33
overall,YouTube/Rickroll,complete POSIX or media. No Phase33.3 work was started.

## What actually ran

Pinned Cobalt 27.lts.3 / Evergreen 7.3.2 executes in MyOS Ring 3, using the
MyOS runtime and native Mesa 25.0.7 softpipe provider. The host only serves
controlled HTTPS fixtures and drives QEMU input/screenshots. JavaScript,
DOM, timers, TLS verification and rendering run inside MyOS, not Linux.
See `PHASE33_COBALT_AUDIT.json` for the pinned engine identity and
`wip/native-pmm512/integration-20261008/evidence.json` for actual binary hashes.

The tested configuration is QEMU, 512 MiB RAM, explicit non-root KVM with
`-accel kvm -cpu host`, RTL8139 user networking and a disposable read-only asset
disk. This is not a minimum-RAM measurement or a real-hardware result.
The owner authorized adding xytron404 to the WSL kvm group. No root QEMU,
automatic TCG fallback, WSL restart or verifier bypass is used.

## Requirement-by-requirement evidence

| Requirement | Observed evidence | Limit |
| --- | --- | --- |
| Real HTML, JavaScript, DOM and timers | Native rendered PNG markers, DOM arithmetic 42 and increasing timer in both launches; `/app.js` fetched over HTTPS | Interpreter-only JavaScript (`--jitless`); no JIT/WebAssembly claim |
| Keyboard and mouse | Ten key-to-rendered-marker checks, pointer click count 1 in each launch, PageDown scroll 80 and PageUp scroll 0 | Sampled key response upper bounds 0.701–1.328 seconds, including 1-second screenshot sampling; not a benchmark of all input paths |
| Verified HTTPS and concurrency | Ordinary engine CA asset, valid page/script/assets, three simultaneous delayed assets; wrong-host, expired and untrusted certificates each rejected on both launches, invalid servers received no HTTP | Controlled private fixture CA and explicit hostname-to-IP route; not public Internet/DNS or arbitrary-site compatibility |
| At least 60 seconds interactive | PNG-visible spans 118.779 and 119.693 seconds, continued timers and input, 120-second post-visible host windows | First visible marker took 102.931 and 91.129 seconds; startup is still slow |
| Close/relaunch and cleanup | Two launches and closes; each returns shell, stops all threads and recovers exact free frames, unique SHM backing frames and alias refs | Warmed kernel heap is bounded, not zero-allocation: 409344 before first launch, 450304 after each close |

Native run: session 88404, terminal exit 0 at 19:11:07 +07;
bundle `native-integrated-XZBHuo/build/native-verified-tls-h35djree`.
`verify_native_web_bundle_20261008.py` independently rechecks actual PNG marker
pixels, JS event records, input ACKs, invalid-server HTTP exclusion, visible
duration and both cleanup boundaries; PASS at 19:12:08.
Committed public evidence is under `wip/native-pmm512/integration-20261008/`.
Private TLS keys and complete asset disks are deliberately excluded.

RAM observer: 420 samples, sampled PMM allocated peak 279449600 bytes, total
observer pause 3.349 seconds. This includes kernel/resident PMM allocations,
excludes separately reserved heap backing, is not per-process RSS and may miss
between-sample peaks. All four launch boundaries recover free126671/total130784.
Observer checks matching ELF/image identity, reads only two low physical counters
while paused and resumes in finally; three adversarial host tests passed.

## Changes and regression boundaries

The candidate derives from main, not the old private diagnostic tree. A
786-input baseline audit found exactly 16 expected changes: indexed VM ownership
ledger and batched page-table pruning, GS errno identity independent of FS TLS,
normal boot-probe retirement, ATA REP INSW reads, owned generic native-launch
arguments, the explicit shell launcher, desktop close/frame synchronization and
native acceptance/PMM-observer tests. W^X/RELRO, ATA write guards, legacy launcher
syntax and persistent storage format remain unchanged.

The route parser rejects malformed hostname/IP syntax and is ASan/UBSan tested.
It changes only destination lookup, never certificate hostname/trust validation.
GS/custom-FS thread reuse, shared-memory rights/rollback, named-SHM lifetime and
executable-path checks passed both BIOS and UEFI. Integrated full system
regression passed with unchanged default TCG, not KVM. Native web
acceptance above is BIOS; it does not imply a native Cobalt UEFI run.
The complete default `tests/test_system.py` retry passed,and normal BIOS/UEFI/
runtime artifacts were restored. All790 promoted source inputs match the tested
candidate. Five normal artifact hashes match exactly after main rebuild; see
`wip/native-pmm512/integration-20261008/main-normal.sha256`,
`main-source-identity.json` and `regression/integrated-regression.status`.

First full attempt 12681 exited1 at19:34:45: its isolated tree omitted the pinned
compiler-rt source needed by a host-compiled emutls fixture. The emutls guest did
not execute on this attempt. All five normal artifact hashes were restored.
The three pinned test-only files were copied from main, audit scope expanded
from783 to786 inputs, all16 prior changed-file hashes remained identical and the
failed compiler command then passed. Retry82911 ran the entire default system
suite again and exited0; the ten already-passed focused BIOS/UEFI runs remain valid.

Runtime foundations are documented separately, with their historical test
identities and explicit unsupported behavior: `PHASE331_PTHREAD.md` (pinned ABI,
realtime/monotonic condition clocks, masks and reclamation), `PHASE331_WAIT.md`
(generation/refcount descriptor model, eventfd/pipe/poll/epoll/FIONREAD),
`PHASE331_THREAD_SERVICES.md` (timed mutex and join/detach ownership) and
`PHASE331_CROSS_THREAD_SIGNALS.md` (later targeted signals/interruptible waits).
Older import counts and native-not-executed statements in these slice histories
are not the current acceptance state. Process/close cancellation is supported;
this report does not claim the pthread cancellation API or complete POSIX.

## Reproduction after promotion

Run from `/home/xytron404/myos`, with the pinned core/provider artifacts present.
The resource/font directories below are existing checked packages on this
machine, not files distributed by Git. The test checks their hashes and packs
its matching normal runtime into a new disposable asset image. It never mounts
or overwrites the owner's data disk. Keep private build bundles private.

```sh
python3 tools/run_logged.py -- env \
  MYOS_TLS_VISUAL_LATENCY=1 MYOS_TLS_RESOURCE_ACCOUNTING=1 \
  MYOS_TLS_RAM_SAMPLING=1 MYOS_TLS_LAUNCHES=2 \
  python3 docs/wip/native-pmm512/kvm-ata-20261008/run_existing_kvm_test_20261008.py \
  tests/test_cobalt_native_acceptance.py \
  /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n \
  /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007
```

Recheck the resulting private bundle with the committed PNG verifier:

```sh
python3 tools/run_logged.py -- python3 \
  docs/wip/native-pmm512/integration-20261008/verify_native_web_bundle_20261008.py \
  build/native-verified-tls-REPLACE_WITH_ACTUAL_BUNDLE
```

The actual shell launch is `elf cobalt CORE PROVIDER URL [HOST=IPv4]` with
already packaged engine/provider/resources/fonts/CA. This is not a standalone
browser icon or automatic asset download. The test prints the exact fixture
URL/port; do not reuse an expired fixture URL after its host servers stop.

For an offline review without the private test bundle,run the committed
`docs/wip/native-pmm512/integration-20261008/review_public_native_20261008.sh`
through `tools/run_logged.py`. It extracts only the public PNG archive into a
fresh build directory,copies public JSON/serial evidence and reruns the pixel
verifier. This passed at20:20:39 on all416 archived frames (archive6602600bytes).
It reviews a recorded native run,not a new engine execution or performance test.

## Explicitly not claimed

- Imports242/242 and fixture PASS do not mean complete libc/POSIX or browser APIs.
- Fifteen media contracts report unavailable, and nine POSIX calls ENOSYS.
- No complete media pipeline, YouTube/Rickroll playback or Phase33.3 work.
- No public-site certification, DNS acceptance, universal HTML/CSS/JS behavior,
  real-hardware validation, minimum RAM or TCG responsiveness claim.
- No swap, demand paging, copy-on-write or page replacement was added.
- No push; the owner pushes local commits manually.
