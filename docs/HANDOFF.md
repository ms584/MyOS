# MyOS handoff — 2026-09-29

Current entry point: [PROJECT_STATE.md](PROJECT_STATE.md). Read it first;
this document retains historical context and may describe earlier checkpoints.

## Phase 33.2 native window/input continuation — 2026-10-01

Continuation from local 2f4425f. Six pinned SbWindow imports plus private
MyOS present/poll helpers now have an isolated Ring 3 implementation, exclusive
process/generation ownership, real framebuffer copying, independent PS/2 raw
events and close/fault/kill reclamation. Native VRAM/screen/input/wait/reuse and
supplementary kernel ASan/UBSan tests passed at 04:00 +07, then again with the
self-test build at 04:06. See PHASE332_NATIVE_UI.md for exact ABI, bounds and
the still-missing EGL/GLES and SbInputData/event-loop integration.

The first full attempt stopped at self-test link because BSS reached 0x809c0,
past the unchanged BIOS 0x80000 stack boundary. The present path now uses
64-pixel stripes, saving 4864 scratch bytes without losing whole-request source
validation. Self-test BSS ends at 0x7f6c0; kernel file is 256280 bytes. The
replacement full suite PASSED at 04:35 +07 including native UI, lifecycle,
network/storage/fault/low-memory and VGA ENOTSUP fallback. Normal framebuffer
image restored: kernel 248088 bytes, image 1474560, BSS end 0x7d6c0. Cobalt
staging/relocation/RELRO/reclamation and 64 MiB OOM rollback PASSED 04:37, but
the engine was NOT executed. Packaged UEFI DMA/stack/real frames/PCM (435/655 Hz)
and network passed 04:37; native UI/input/close/fault/cancel/relaunch and exact
page/heap/sample restoration passed on UEFI at 04:38. Six window exports make
156 fixture-covered strong imports; 86 strong imports and weak TLS remain.
These supported subsets do not prove renderer or Cobalt compatibility. Fixed
800x600/no mode switching, raw overflow loss, no wheel/IME, and reserved Escape
force-cancel remain limitations. Cobalt HTML/JS/DOM/input/verified HTTPS >=60s
acceptance is absent. Phase 33.1/33.2 stay OPEN; no Phase 33.3 and no push.

## Phase 33.2 renderer feasibility continuation — 2026-10-01

Runtime baseline is local 8003d20; its full/UEFI regression is green and normal
image restored. The isolated Mesa 25.0.7 softpipe core built on WSL at 03:06;
host-only context/shader/texture/glyph-mask/guard tests passed at 03:07/03:12.
The measured 17854872-byte library depends on libc/libm/libgcc/libstdc++ and
uses PT_TLS/TPOFF64/TLSDESC; it cannot be used unchanged by the MyOS loader.
The pinned tables have 44 EGL and 246 GLES slots. Inventory verifies ordered
offsets/hashes, not API completeness. Source checksum and reproducible host-only
compatibility patch are recorded in PHASE332_RENDERER.md. No native backend
is chosen: actual context/present/texture/text, native memory/CPU and engine
acceptance still need implementation and proof. Production code/image did not
change in this spike. Phase 33.1/33.2 remain OPEN, no Phase 33.3, no push.

## Phase 33.1 socket timeout continuation — 2026-10-01

Continuation from a6e2899. SOL_SOCKET receive/send timeout options use the
pinned x64 timeval boundary and share ownership with duplicated descriptors;
socketpair endpoints remain independent. Read/write/send/recv snapshot one
monotonic deadline, preserve partial I/O/EOF/nonblocking behavior, and expire
with EAGAIN without renewing on spurious wakes or later option changes.
Blocking connect expires with EINPROGRESS without deleting its handshake.
Internal status now reads persistent failure independently of the consumed
SO_ERROR value, preventing another thread's getter from hiding connect failure.
Unsupported options still return ENOPROTOOPT. See PHASE331_SOCKET_OPTIONS.md.

Native Ring 3 fixture and TCP ASan/UBSan + concurrent-stream regressions passed
at 02:18 +07. The new fixture runs twice, verifies timeval validation/rounding/
overflow, endpoint/dup sharing, fixed deadlines, nonblocking and partial I/O,
close/reuse and cleanup. QEMU packet buffering genuinely stalls SYN and data;
native connect/send time out, then the same connection resumes and delivers
all accepted bytes and EOF. No guest-memory or syscall-result substitution.
The persistent/consumed-error distinction is checked through the actual
Ring 3 syscall boundary as well as the host state-machine test.

The first new full run was stopped intentionally at 02:12 for the error race
fix and restored the normal image; it is not a PASS. Replacement full regression
passed at 02:49 +07, including the native socket fixture, lifecycle, persistence,
memory/fault recovery and VGA fallback. Normal kernel: 242808 bytes; image:
1474560 bytes. Cobalt-only staging passed at 02:50: 150 strong imports resolved,
92 missing, weak TLS zero, relocation/RELRO/reclamation and 64 MiB OOM rollback.
Packaged UEFI DMA/framebuffer/captured PCM and network checks passed at 02:50.
The pinned contract check passed at 02:05. Cobalt has not executed: staging and
supported-subset fixtures do not prove HTML/JS/DOM/input/HTTPS acceptance.
Phase 33.1/33.2 remain OPEN; no Phase 33.3, no push.

## Phase 33.1 concurrent TCP continuation — 2026-10-01

Continuation from local c835cfe on main (owner pushes manually; never push).
Added actual independently owned IPv4 TCP streams, not aliases of the legacy
single-stream client. The bounded 32-stream pool has process-private generation
handles, separate peer/port/sequence/window/rings/retry/EOF state, nonblocking
kernel syscalls and timer-driven NIC progress. Runtime descriptors reuse the
existing object/ref/dup/generation cancellation model and poll/epoll/FIONREAD.
See PHASE331_TCP.md for supported getters/flags, resource bounds, compatibility
and the still-missing resolver/options/TLS integration. Both dup paths count
open descriptors separately from borrowed references; close of a waited-on
descriptor cancels that operation without breaking its surviving duplicate.
Process stop releases connections and linger states before page reclamation.

Native QEMU tests passed at 00:17 +07: two Ring 3 pthreads transfer different
32768-byte patterns concurrently, each receives 20000 bytes and EOF after
write shutdown; nonblocking connect, SO_ERROR, readiness and epoll timeout,
peek/FIONREAD, distinct tuples, address/option short-buffer canaries, no NIC,
pool exhaustion, refusal, dup/close/slot reuse, repeat launch and blocked-worker
forced cancellation all pass. Pages/heap return to baseline each time.
The actual TCP state machine passes ASan/UBSan packet-boundary tests including
partial ACK/retransmit, sequence wrap, duplicates/out-of-order, bounded/zero
window, FIN/RST, timeout and allocation/owner/generation isolation. The five
new fixture-covered exports make 149/242 strong imports (93 missing), with
the weak TLS initializer tracked separately; these are supported subsets,
not proof of complete ABI or engine compatibility. Pinned hash/contract
inventory passed at 00:08 +07.

Two full system attempts stopped at the initial legacy-video frame and restored
the normal image. A stronger image test confirmed black pixels in actual guest
VRAM, not just QEMU display refresh. Root cause: the fixed SB16 DMA buffer
0x70000..0x73fff overlapped grown BSS, including graphics state in self-test
images. BIOS DMA now uses 0x90000..0x93fff, outside kernel/BSS (<0x80000) and
the initial stack (0x80000..0x8ffff). Extending UEFI's fixed low-memory
reservation failed in OVMF. UEFI now allocates its own 64 KiB initial stack
and DMA allocation below 16 MiB; boot ABI v2 passes the aligned 16 KiB DMA
subrange to SB16. The firmware memory map reserves both allocations from PMM.
The driver validates the ISA DMA boundary and non-overlap with the stack.
Shared C constants/static assertions and an audio graphics-state integrity
check guard the layout. Previously failing self-test-image video/audio passed
at 00:46/00:47; packaged UEFI DMA/framebuffer and network checks passed at
00:54/00:55 +07. No media feature or Phase 33.3 work was added. Legacy HTTPS/HTTP
passed at 00:22/00:23. Full tests/test_system.py passed at 01:28 +07 and restored
the normal framebuffer image, including native TCP, lifecycle/teardown, storage,
memory/fault and VGA fallback. Final packaged UEFI DMA/graphics test at 01:29
also verifies captured PCM (435/655 Hz). Cobalt-only staging passed at 01:30:
149 strong imports resolved, 93 missing, weak TLS zero binding, staged pages
reclaimed and 64 MiB OOM rollback. Cobalt was not executed.
Normal image after final edge fixes: 1474560 bytes; kernel 241752 bytes.

Still OPEN: signal delivery, resolver/interfaces/required socket options and
I/O deadlines, real native Cobalt TLS integration, renderer feasibility and
Evergreen lifecycle/window/input adapters. Cobalt has not executed in MyOS;
HTML/JavaScript/DOM/input/verified-HTTPS >=60s acceptance remains unproven.
Do not close 33.1/33.2 or start 33.3 on import counts or fixture PASS.

## Phase 33.1 Starboard storage continuation — 2026-09-30

Starting from local commit 18191dd, the storage slice is committed locally on
branch main (do not push; no changes were pushed). Before the commit, the branch
was six commits ahead of origin/main. Implemented Starboard
open/close/size/read/delete and the header's write API using two checksummed
record generations, fsync on
the persistent v2 disk, and durable tombstones. The runtime export allowlist
now exposes these APIs. The five Cobalt-exported storage imports move from
139/242 to 144/242 tested; `SbStorageWriteRecord` is implemented as the header
API but is not an import in the pinned export CSV.

`tests/test_elf_dynamic.py --storage-only` and the full
`tests/test_elf_dynamic.py` passed at 22:26 +07. A contract edge case was then
corrected (deleting an existing tombstone returns false); the focused storage
test and full dynamic suite passed again at 23:01 +07. Ring 3 checks immediate
read-back, single-open/name validation, data after reboot, durable deletion,
repeat-delete behavior, and tombstone state after another reboot. The full
`tests/test_system.py` regression passed at 22:54 +07 and restored the normal
framebuffer image. The final normal image is 1474560 bytes with a 234896-byte
kernel. See PHASE331_STORAGE.md for limits and failure behavior. This is a
local checkpoint only; no push.

The pinned actual Cobalt staging run
(`tests/test_elf_dynamic.py --cobalt build/engine-evaluation/libcobalt.so`)
passed at 23:13 +07: relocations were processed, 144 strong imports resolved,
98 remained missing, all staged pages were reclaimed, and the 64 MiB OOM
rollback passed. Cobalt was not executed. No browser, HTML, JavaScript, DOM,
input or HTTPS acceptance has passed; Phase 33.1/33.2 remain OPEN and 33.3 has
not started.

Phase 33.1/33.2 remain OPEN: Cobalt itself still does not execute in MyOS and
HTML/JavaScript/DOM/input/HTTPS endurance acceptance remains absent. Do not
start Phase 33.3.

## Phase 33.2 finalizer continuation — 2026-09-30

Local baseline 74cdf9a passed full lifecycle/pthread regression at 20:17 +07.
Tested finalizer slice now supports in-image DT_FINI and bounded reverse
FINI_ARRAY on normal entry return, preserving status. Wrapped init/fini offsets
and malformed array addresses/sizes/targets reject before any constructor.
The Ring 3 fixtures pass fini-only/array-only/combined order, exactly-once markers,
fault/timeout/Escape/direct-exit behavior, repeat launch and page/heap recovery.
Focused dynamic/persistence/pinned Cobalt staging passed at 20:24 +07; full
tests/test_system.py passed at 20:53 +07 and restored the normal framebuffer
image (kernel 232368 bytes). The checkpoint is local only; no push.
See PHASE332_FINALIZERS.md. Import counts remain 139 tested/103 missing; Cobalt
does not execute. Neither Phase 33.1 nor 33.2 closes, and no Phase 33.3 work.

Pinned source and binary inspection show emulated TLS inside libcobalt.so and
NULL guards around the weak V8 TLS initializer. The static call-site checker
tools/check_cobalt_tls.py and adversarial tests/test_cobalt_tls_audit.py do not
execute TLS/engine code. The verifier passed at 20:31 +07: all 160 direct calls
have audited NULL/skip guards, including one two-call shared guard checked with
its exact reviewed instruction pattern. Native emulated TLS allocation/destructor/errno/
preemption and real engine execution remain required before claiming compatibility.
The next audit found a real ABI mismatch: pinned musl_pthread_key_t is a 64-bit
pointer, while current create output and key arguments use unsigned (32 bits).
The prepared full-width correction and canary/high-bit tests are not yet native
validated; this checkpoint does not claim that correction is complete.

## Phase 33.1 live lifecycle and pthread ABI audit — 2026-09-30

Tested local continuation from 57e2b97: explicit `elf live` runs beyond the demo
deadline and Escape kills/reaps all threads; `elf run` remains bounded. The
16 MiB native fixture runs for >60 seconds with blocked pipe/semaphore/condition
workers, retained allocation and open file, then verifies page/heap/file cleanup,
repeat launch, constructor cancellation, normal exit, RELRO fault and early OOM.
See PHASE331_LIFECYCLE.md for force-cancel/input/finalizer limits.

The pinned signal mask ABI is 128 bytes, not 128 bits. Full output initialization,
reserved/highest-signal handling, inheritance and canaries are tested; masks
still do not deliver signals. Semaphore errors now use -1/errno and atomic
overflow rejection; pthread name failures do not partially mutate names/output
or errno. See PHASE331_PTHREAD.md. Focused tests/hash checks/pinned Cobalt staging
passed at 19:51 +07 (139 resolved, 103 missing; 64 MiB OOM reclamation). Full
tests/test_system.py passed at 20:17 +07 and restored the normal framebuffer
image. Next: single-image finalizer lifecycle, then remaining essential services.
Phase 33.1/33.2 remain OPEN, Cobalt web acceptance is unproven, no Phase 33.3.

## Phase 33.1 local socket continuation — 2026-09-30

After c8c9c96, AF_UNIX SOCK_STREAM socketpair reuses shared descriptors and waits
with endpoint-local shared status, duplex 4096-byte rings, send/recv/read/write,
half/full shutdown, poll/epoll/FIONREAD and generation-based close cancellation.
The native 16 MiB fixture passes blocked-reader/writer cancellation and wakeup,
two waiting readers, duplicates, slot reuse and partial-install rollback.
See PHASE331_SOCKETPAIR.md for supported subsets, SIGPIPE/network limits and
the concurrent-TCP follow-on design. Pinned Cobalt staging passes (139 tested
strong imports, 103 missing; 64 MiB OOM cleanup). Full system regression passed
at 19:38 +07 and restored the normal framebuffer image, including logger tests.
Phase 33.1/33.2 remain OPEN; no real Cobalt web execution and no Phase 33.3.

## Phase 33.1 system ABI continuation — 2026-09-30

The following file slice is the tested local checkpoint after 4be9c3b. See
[PHASE331_FILES.md](PHASE331_FILES.md). Regular files now use the shared
descriptor registry, with shared offsets/status flags, exclusive create,
large read-only asset access, directories/temp files and real fsync/fdatasync
snapshot commits. Focused Ring 3 tests and an 8 KiB binary reboot-persistence
test pass. The register has 135 tested strong imports, with 107 still missing.
Full system regression passed at 19:02 +07 and restored the normal framebuffer
image. Pinned Cobalt staging passed: 135 bound/107 missing at 256 MiB with
complete reclamation, and OOM rollback at 64 MiB. Bounded logger tests pass;
the current plus two rotated logs are capped at 16 MiB each, and feedback
commands are refused. Watch using unwrapped tail -F. Phase 33.1/33.2 remain
OPEN; Cobalt still does not execute. Phase 33.3 is not started.

After the clock/wait commit, the tested Cobalt allowlist advances from 75 to
108 strong imports. A new Ring 3 fixture validates actual MyOS PID versus TID,
uid/euid, auxv, resource limits, CPU affinity, scheduler-policy rejection,
yield, priority rejection, prctl names, sysconf, uname and the four imported
Starboard filesystem/thread constants. A kernel `SYS_GETTID` returns the real
scheduler thread identity. DRM, microphone and speech imports implement their
pinned documented unavailable behavior and never advertise fake devices or
sessions. Focused dynamic tests and pinned Cobalt 256/64 MiB staging/OOM tests
pass; Cobalt now stops safely on 134 missing strong imports and the weak TLS
initializer remains zero-bound. This remains dependency progress only: the
HTML/JavaScript/DOM/input/HTTPS gate is not passed and Phase 33.3 is not started.

## Phase 33.1 runtime clocks and waits — 2026-09-30

This local slice starts from b6c74b5 and remains inside Phase 33.1/33.2. It
adds realtime/monotonic clock services and corrects the pthread ABI around
condition clock attributes, absolute timed waits, per-thread signal-mask
inheritance, handle validation, single-consumer joins, detached reclamation and
TLS-key reuse. The focused Ring 3 fixture covers these contracts directly.

A shared runtime descriptor registry now owns refcounted open objects with
generation validation and close-driven wake/cancellation. Eventfd, pipe/pipe2,
poll, epoll and FIONREAD are implemented on it, with ordinary MyOS file
descriptors forwarded through the same read/write/close entry points. The model
is intended for the still-missing sockets; socket APIs were not implemented in
this slice. See [PHASE331_WAIT.md](PHASE331_WAIT.md). The focused descriptor fixture covers readiness, epoll data/add/del,
nonblocking/semaphore eventfd, pipe EOF/HUP/FIONREAD, timeout, blocked-reader
close cancellation and descriptor recycling.

The self-test image crossed the old 0x73000 linker reservation. BIOS stage 2
and the UEFI loader were audited; the memory guard now uses the actually
reserved 0x10000..0x7ffff region while retaining the independent 256 KiB kernel
file limit. Host-load-sensitive boot waits were raised and stuck QEMU teardown
now falls back to kill. Focused ELF dynamic, pinned-Cobalt staging, focused
audio and full system regressions pass; the full run restores the normal
framebuffer image.

The pinned Cobalt audit now resolves 75 tested strong imports and reports 167
missing strong imports (plus the separately tracked weak TLS symbol). This is
staging evidence only. Cobalt still does not execute, so Phase 33.1 and 33.2
remain OPEN. Do not start Phase 33.3 until Cobalt itself opens HTML, runs
JavaScript/DOM, accepts real input and uses HTTPS inside MyOS for the required
endurance gate.

`build/activity.log` was truncated after accidental recursive amplification.
Never inspect that file through `tools/run_logged.py`, because the wrapper
appends the command's output to the same log; inspect only bounded excerpts
outside the wrapper. The owner pushes manually; do not push this local commit.

The latest Phase 33.1 continuation adds tested anonymous user VM, extra Ring 3
threads, FS-base TLS and atomic wait/wake/timeout primitives. See
`docs/PHASE331_RUNTIME.md`. These custom kernel primitives now back a fixed-address Ring 3 platform runtime.
Sixty-two tested allocator/VM/errno/pthread imports resolve through an exact allowlist;
180 strong Cobalt imports remain. Native Cobalt HTML/JavaScript/input
smoke remains the required gate before Phase 33.3.

## Next-session plan

The owner requested detailed planning only after pausing implementation. Read
[PHASE33_EXECUTION_PLAN.md](PHASE33_EXECUTION_PLAN.md). Resolved imports are
not proof of full contract semantics; clock attributes and signal masks need
audit before further API expansion. No tests were run for this docs-only plan.

## Phase 33.1/33.2 pthread checkpoint - 2026-09-29

Local work after commit 290f1dc adds a 4,960-byte Ring 3 platform runtime
covering all 48 pthread/semaphore imports required by the pinned Cobalt image.
Together with allocator and POSIX VM/errno support, 62 of 242 strong imports now
resolve and 180 remain. The acceptance fixture proves joinable and detached
thread lifecycles, detached slot/stack reuse, mutex/condition/semaphore/rwlock/
once behavior, timed waits, thread-specific keys with destructors, names, stack
attributes and scheduler compatibility. Detached kernel stacks and user mappings
are reclaimed. The exact implementation and limits are in
docs/PHASE331_PTHREAD.md.

python3 tests/test_elf_dynamic.py passed after the final stack-attribute fix.
The optional pinned Cobalt staging test passed during this slice with 62 resolved
imports, 180 missing imports and full staged-page reclamation. The full
tests/test_system.py run was started and passed boot, shell, graphics, mouse,
desktop, Settings, Calculator, Snake, audio, video, browser, release integration
and the Phase 26 app cases shown in build/activity.log; it was stopped at the
owner's request before completion. Run the complete suite next. A normal
bash build.sh image was restored after stopping it.

Neither Phase 33.1 nor Phase 33.2 is closed. Continue these twelve gates in
order, preserving real acceptance tests rather than no-op symbol stubs:

1. Finish the exact ABI register and keep every implemented import fixture-tested.
2. Preserve allocator, anonymous VM, errno and process cleanup coverage.
3. Preserve pthread synchronization, thread lifecycle and TLS-key coverage.
4. Add ELF PT_TLS/TLS relocations, destructors, needed dependency/finalizer support.
5. Add cancellation and waitable descriptors: poll, epoll, eventfd and pipes.
6. Add required file, storage, clock and system contracts.
7. Add the remaining POSIX socket/network contracts on the existing NIC/TCP/TLS stack.
8. Implement the real Evergreen SABI launcher, SbEventHandle and event loop.
9. Implement Starboard window, EGL/GLES bridge and keyboard/mouse input delivery.
10. Implement Starboard audio, decode-target and media/player contracts.
11. Run native Cobalt HTML/JavaScript with real input for at least 60 seconds and prove cleanup.
12. After combined runtime/engine closure: 33.3 standard media, 33.4 actual YouTube, then 33.5 Rickroll acceptance.

Next session: finish the outstanding full regression, audit existing pthread
clock/mask/ABI behavior, then design shared descriptors and implement clock/wait
slices. See PHASE33_EXECUTION_PLAN.md for the detailed updated sequence. The contract CSV currently groups the still-unimplemented entries as
41 file, 23 media, 23 network, 23 platform, 16 optional, 13 system, 10 graphics,
8 wait, 5 audio, 5 storage, 4 constants, 4 signals, 2 clock, 2 event,
1 certification and 1 weak-TLS symbol. The owner always pushes manually.

Read root AGENTS.md and docs/ROADMAP.md before starting. The owner asked to
continue the approved phase sequence and **always pushes to GitHub personally**.
Make local commits when work is verified; never run git push. Log development
commands with python3 tools/run_logged.py -- ... so the owner can watch
tail -f build/activity.log in VS Code.

## Verified baseline

- Repo: /home/xytron404/myos in WSL Ubuntu, branch main.
- Phases 28, 29 and 30 have documented **QEMU baselines**. Phase 28 has native
  certificate-verified TLS 1.2; Phase 29 has a bounded kernel-mode text
  browser with real DuckDuckGo Lite HTTPS search, result links and cached Back;
  Phase 30 has QEMU SB16 PCM output and WAV capture.
- Phase 29 is not a modern CSS/JavaScript browser. YouTube playback has **not**
  been implemented. Physical hardware support has not been proven.
- The owner keeps the Git push step. Check local commits and origin/main before
  starting the next phase; do not assume this handoff is already pushed.
  Earlier commits: 6689afb (Phase 30), ac43b74 (Phase 28), fa52adc (Phase 29).
- Phase 31 now adds a native MYV1 local video player (80x60, 10 fps palette/RLE,
  11025 Hz unsigned PCM) on the framebuffer and SB16 with PIT-based sync.
  See docs/PHASE31.md for the QEMU tests, demo disk and limitations.
- Phase 32 is in progress, not accepted: MyOS now verifies the live YouTube
  HTTPS connection with SHA-384 and a GTS Root R1 certificate scoped to the
  exact www.youtube.com hostname. The Browser reports its current 64 KiB
  page-size gate; it still lacks JavaScript and online media decoding.
  The owner requires native MyOS playback at every step; WSL may build and
  test but must not fetch/decode/play YouTube media for MyOS. Native TLS
  streaming past 64 KiB has a 256 KiB QEMU integrity test; the GUI still
  uses its 64 KiB/160-line bounded text renderer. A live QEMU native TLS
  probe streamed 902,111 bytes from www.youtube.com on 2026-09-27, but
  no YouTube audio/video playback has been implemented. A 2026-09-28 native
  QEMU probe also received 1,344,776 bytes from the Rickroll watch URL;
  this is transport evidence only. Tor/search do not supply the missing
  JavaScript, HTML5 and codecs.
  See docs/PHASE32.md. Next work is the full YouTube player acceptance.
- Phase 33 is in progress but not passed. A new `video net HOST PORT PATH`
  command fetches a <=64 KiB MYV1 clip over verified HTTPS inside MyOS,
  validates HTTP status/length and plays framebuffer motion plus SB16 audio.
  The focused QEMU test rejects bad length and wrong-host TLS. This is a
  whole-clip synthetic format test, not YouTube media. Left/Right seek
  works only inside the loaded MYV1 clip.
  Phase 33 still requires actual online Rickroll video plus sound and
  controls in MyOS/QEMU;
  it cannot pass on an HTTPS fetch, search result, static frame, local
  MYV1 clip, Tor route, or host playback. See docs/PHASE33.md.
- Remaining Phase 24 hardware preparation and Phase 25 physical-machine
  acceptance follow; Phase 25 is deferred because the owner has no USB device.

Phase 31 full tests/test_system.py passed, including 30 native video frames,
measured 435/655 Hz QEMU SB16 WAV output, pause/resume/stop and malformed
file recovery. BIOS and UEFI packaged images both passed video/audio smoke
tests. The sample data disk mounted and played at 32 MiB RAM; 16 MiB rejected
it gracefully. Two existing GUI screenshot tests were made deterministic by
waiting for their matching desktop frame before reading QEMU pixels.

## Recent validation and unresolved observation

Phase 29 passed local and live QEMU browser tests, TLS and network suites,
the full tests/test_system.py regression, plus packaged BIOS/UEFI browser
tests. The production framebuffer image was restored afterward. Phase 30's
automated tests/test_audio.py passed previously; see docs/PHASE30.md.

The owner reported that manually entering audio tone 440 500 in QEMU appeared
to hang without a new myos> prompt. An automated QEMU probe using the same
-cpu max, RTL8139 NIC and WAV audio backend returned Audio tone queued
promptly, so the manual UI symptom is **not yet reproduced or fixed**.
The owner's finalized build/phase30-demo.wav was a valid 44100 Hz stereo
16-bit WAV, about 456.7 seconds long, with audible signal in its first
second. A short build/phase30-tone-clip.wav was created for playback.
Both are ignored build artifacts. The owner also saw a transient VS Code/WSL
E_UNEXPECTED / WebSocket 1006 connection failure, later reported normal.
Do not infer an OS bug or a WSL root cause from those screenshots alone.

If the manual audio hang recurs, reproduce with a QEMU serial log and check
whether Audio tone queued is emitted before changing the SB16 driver. The
shell command should queue the 500 ms tone and return promptly; QEMU itself
stays in the foreground until its window is closed or Ctrl+C is sent to the
host terminal. Avoid opening the WAV capture in VS Code while QEMU writes it.

## Work rules for Phases 32–33

Start from docs/PHASE31.md, docs/PHASE29.md, docs/PHASE32.md and
docs/PHASE33.md. Do not count local MYV1, a host-side player or static
frames as online YouTube playback. Preserve boot, GUI, shell, storage, network
and audio baselines. Run focused QEMU tests, the required
python3 tests/test_system.py for architectural changes, and bash build.sh to
restore the normal image after test variants. Document limits plainly.
The owner wants a local commit ready for their manual push.

## Engine assessment, 2026-09-28

Static comparison and the official Cobalt x64 ELF audit are recorded in
[PHASE33_ENGINE_EVALUATION.md](PHASE33_ENGINE_EVALUATION.md). Cobalt 27 LTS /
Starboard 18 is the conditional native-port target. The first Phase 33.1 slice
now has an exact import-contract register, read-only large assets, restricted
Ring 3 ELF execution and native Cobalt import inspection. See [PHASE331.md](PHASE331.md)
for tests and limits. Phase 33.1 remains incomplete: external platform bindings, the native
Starboard runtime and HTML/JavaScript smoke are still open. Native engine
startup and actual YouTube playback remain unproven.
Phase 32/33 remain **NOT PASSED**.

## Phase 33.2 — dynamic loader continuation

The next loader milestone is documented in [PHASE332.md](PHASE332.md): staged
ET_DYN mapping, four relocation types, in-image/weak resolution, RELRO and
Ring 3 constructor startup for self-contained fixtures. Actual Cobalt maps
and processes relocations, resolves 62 tested allocator/VM/errno/pthread imports, and stops on
180 remaining strong imports, so it is released without execution. Full native-engine/HTML/JavaScript support
and the original Phase 33.1 gate remain open; this does not close Phase 32/33.

Phase 33.2 loader baseline: focused tests and the full system regression passed
on 2026-09-28; normal framebuffer image restored. See PHASE332.md for the
restricted loader scope and the still-open native-engine gate.

## Runtime continuation — 2026-09-28

Per-task x87/MMX/SSE context support has been added; see
[implementation and test status](PHASE331_FP.md). This supersedes the earlier
FP/SIMD limitation only. The owner requires completion of 33.1 and 33.2
before starting 33.3. Cobalt startup remains unimplemented; neither parent
gate is being marked complete.

## Local checkpoint — 2026-09-30

Commit `571c559` records DT_FINI/reverse FINI_ARRAY and the weak TLS static
audit. Its focused/pinned checks and full system regression passed; normal
framebuffer image restored at 232368 kernel bytes. No push was made.

Follow-up work fixes the pinned `pthread_key_t` ABI to pointer-typed `void *`
entry points, validating full-width opaque values and all 32 key slots. The
runtime key/destructor fixture, compiler-rt `llvmorg-18.1.8` emutls Ring 3
fixture and weak TLS audit unit test pass. Full `tests/test_system.py` passed
again at 21:54 +07, restoring the normal framebuffer image (232368 bytes).
The added ABI/emutls sources, tests and docs remain local; owner pushes manually.

This does not run Cobalt or prove its compiler build, native weak initializer,
Starboard storage, TCP concurrency, renderer/input bridge, HTML/JavaScript/DOM,
or verified HTTPS acceptance. Keep Phases 33.1/33.2 open and do not start 33.3.
Next: continue the Starboard storage/runtime and concurrent TCP dependencies,
then implement the real Evergreen launcher and renderer/input path.
