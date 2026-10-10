# Phase 33.1-33.5 execution plan

Current2026-10-10 13:20+07:33.4 ACCEPTED on the measured native MyOS/QEMU
profile. PHASE334_ACCEPTANCE.md/checkpoint13 contain the final audit,actual
SABR expiry/recovery and restored-normal proof; checkpoint11 contains direct/
search/watch/controls/sound and focused checks. Local commits only/no push.
No33.5/parent33/Rickroll acceptance. Historical notes retain their original scope.

Planning checkpoint: 2026-09-29, implementation HEAD 9f58024.
Status: planning only. Neither 33.1 nor 33.2 is closed.
This plan follows the owner's five-stage table. It supersedes the old handoff
sentence that placed real Rickroll acceptance in 33.3. It does not weaken the
parent acceptance in PHASE33.md. The combined 33.1/33.2 closure still requires
the real native engine smoke; neither phase closes on symbol counts alone.

## Current execution checkpoint  2026-10-09
The historical planning baseline below is retained. Native33.1/33.2 was accepted
on2026-10-08 (PHASE33_NATIVE_WEB_ACCEPTANCE.md). Standard-media33.3 is now
accepted on its declared MyOS/QEMU profile (PHASE333_ACCEPTANCE.md).
That earlier task stopped at33.3. On2026-10-09 the owner explicitly authorized
Phase33.4 only. PROJECT_STATE.md/PHASE334.md are current: checkpoint11 proves
real native public YouTube playback, controls, bounded sync and network-loss
recovery. Real signed-URI expiry and its recovery remain pending in checkpoint12. No33.5, Rickroll or parent Phase33 acceptance is claimed.

## Verified starting point and uncertainty

- Ring 3 ELF assets, staging, supported relocations, RELRO and fixture startup exist.
- VM, FP/SIMD preservation and custom thread/wait primitives have test coverage.
- The checkpoint resolves 62 of 242 strong Cobalt imports; 180 remain, plus
  one separately tracked weak TLS initializer. This is symbol-resolution
  progress, not percentage completion or proof of full API semantics.
- Existing pthread fixtures passed. Current code still needs a contract audit:
  cond_init ignores its clock attribute, deadline_ticks uses PIT time,
  pthread_sigmask discards its requested mask, and thread ID/name/error behavior
  needs stronger tests. Do not label these complete solely from fixture PASS.
- The pinned Cobalt core has not executed in MyOS. EGL/GLES renderer selection,
  actual engine RAM/stack needs and YouTube frontend compatibility remain open.
- Full regression for 9f58024 was interrupted by the owner, not completed.
  A normal build was restored. No new regression is claimed by this plan.
- Native HTTPS and custom MYV1/SB16 playback are existing building blocks;
  neither proves a modern browser or standard compressed media playback.

## Execution rules

Use focused, reviewable implementation commits and log commands via
tools/run_logged.py. Run focused tests for each slice and the full system
suite at architectural checkpoints; restore the normal image. Do not rebuild
or mutate shared build outputs while another suite uses them.
Record supported behavior, measured bounds, unsupported errors and regressions.
The owner pushes manually. Never call git push.
WSL may compile, package and provide deterministic test fixtures. Production
network fetch, TLS, JavaScript, demux, decode, rendering and audio must run in
MyOS; host transcoding, host playback and a Linux substitute do not pass.
Resolve optional APIs with documented unavailable behavior only where the pinned
engine permits it; essential features must be implemented and exercised.

## 33.1 - application and OS runtime

### A. Establish the checkpoint
1. Run the existing full system suite to completion before adding architecture.
2. Recheck the pinned Cobalt binary/header hashes and the import register.
3. Audit opaque object sizes/alignment/static initializers against the pinned
   ABI, not the host libc by assumption.
4. Add adversarial tests for realtime versus monotonic deadlines, mutex owner
   transitions under preemption, key reuse/destructors, signal masks, invalid
   handles and small output buffers. Fix behavior or record unsupported status.
5. Review current low-memory kernel layout and the new 0x73000 linker bound
   against boot reservations, stacks, page tables and BIOS/UEFI layouts.

Deliverable: a reproducible green baseline and honest per-contract status.

### B. Clock, descriptors and waits
Define a shared descriptor/open-object model and lifetime rules first so files,
pipes, sockets and readiness APIs can reuse it. Distinguish descriptor close,
object reference, duplicated handle, offset and wakeup ownership.
Implement realtime/monotonic clocks, bounded sleeps and deadline conversion,
then eventfd, pipe/pipe2, read/write/close behavior, poll, epoll and FIONREAD.
Support the modes actually needed by the engine; reject unimplemented flags.
Test lost wakeups, timeout boundaries, partial I/O, EOF, full pipes, multiple
waiters, close during wait, handle reuse and cancellation cleanup.

Deliverable: a Ring 3 producer/consumer test that blocks and wakes without busy
spinning, survives close/error/cancel and returns resource counts to baseline.

### C. Long-running tasks, TLS and loader integration
Implement explicit start/stop/cancel lifecycle for engine apps rather than
globally removing the five-second demo timeout. Audit thread stacks and guard
pages. Prove TLS/errno/FP state under preemption.
Add PT_TLS, relocations, init/fini/dependency behavior only as required by the
pinned binary or native renderer/codec dependencies, with rejection for other
unsupported cases. Resolve the weak TLS initializer's actual call contract.
Test responsive execution beyond 60 seconds, forced exit with sleeping workers,
fault isolation, repeated launch/close and complete resource reclamation.

### D. Files, storage, system and network services
Expose large asset random reads, 64-bit offsets, seek and metadata to Ring 3.
Implement required file/dir/dup/sync semantics and Starboard persistent records;
test bounds, readonly protection, EOF and reboot persistence on disposable disks.
Implement real IDs, scheduling/resource queries, constants and signal behavior
as required; never fabricate capabilities or silently accept unsupported policy.
Replace the single active kernel stream limitation with tested concurrent
user sockets, resolver, options, readiness, partial I/O, EOF, cancel and timeouts.
Reuse existing native TLS where its actual integration contract permits; audit
the engine's own TLS boundary rather than assuming it uses the kernel fetcher.
Test simultaneous page/script/asset requests, DNS failure, reconnect and TLS
hostname/chain/time failures.

33.1 runtime evidence: all required runtime fixtures, >60-second cancellable app,
bounded resources, fault/low-memory recovery and full system regression.
Formal closure remains coupled to the native-engine smoke in 33.2, per the
previously agreed gate.

## 33.2 - Cobalt/Starboard and real web execution

### A. Resolve feasibility before a broad renderer port
Pin the core, SABI headers, resources, toolchain and all dependencies.
Build a renderer feasibility report: exact EGL/GLES entry points, dependencies,
C++/TLS/VM/thread requirements, binary size, CPU features, framebuffer integration.
Choose a concrete backend only after a native context/present/texture/text test.
Software renderer candidates in PHASE331_ABI.md remain candidates, not decisions.
Measure memory and QEMU performance. Do not publish guessed minimum RAM.
Investigate the certification/signing and frontend compatibility questions early;
record unknowns and actual evidence without inventing credentials.

### B. Implement the real launcher and adapters
Implement the correct SABI boundary, SbEventHandle lifecycle, start/stop events,
scheduled callbacks and cancellation, paths/logging/properties/locale/resources.
Complete every required imported function AND returned function-table contract:
SbGetEglInterface/SbGetGlesInterface counts alone hide many renderer entry points.
Bridge native windows/framebuffers and keyboard/mouse focus, text and pointer
input. Define context/thread ownership and safe callback teardown.
Supply native audio plumbing and documented capability behavior needed to boot
the engine; compressed-media feature completion belongs to 33.3.
A missing essential interface blocks execution; a fake success is not a fix.

### C. Native web acceptance
Run local deterministic HTML with JS arithmetic, DOM text updates, timers,
keyboard typing, clicks and scrolling. Then load a controlled verified HTTPS
page with external scripts/assets and exercise concurrent requests.
Keep it interactive for at least 60 seconds, then close and launch again.
Capture QEMU frames, serial logs, JS/DOM results, real input events, RAM peaks,
elapsed startup time and resource counts. Run applicable upstream contract
tests plus local isolation/regression tests.

33.2 passes only when Cobalt actually renders and executes JS/DOM with input
and verified HTTPS in MyOS. Link success, text-browser output and host pages
are not that evidence. Close 33.1 and 33.2 together only once both gates pass.

## 33.3 - standard compressed media

After 33.1/33.2 closure, pin a measured demux/codec combination supported by the
engine and useful to the selected online format. WebM plus VP8/VP9 and Opus is
a candidate; inspect real requirements before committing to it. Evaluate other
containers/codecs when required, recording configuration and dependency closure.
Port and run demux and software decoders in MyOS. Implement Starboard player,
audio sink and decode target callbacks, backpressure, timestamps, EOS/errors,
buffer budgets and a shared A/V clock. Adapt PCM to the measured audio backend.
Implement pause/resume, seek flushing and timestamp reset, stop/restart.
Stream in bounded buffers; do not require a whole online video to fit in RAM.

Tests: standard-format local asset -> controlled HTTPS streaming -> engine media
element/player. Use known expected duration/timestamps and decoded frame/audio
evidence, plus truncated/corrupt input, network stalls and teardown tests.
Set the resolution, frame rate, acceptable sync error and RAM budget before
acceptance from measured capabilities; report actual underruns/dropped frames.
Proposed initial sync target is within 100 ms, subject to measured review.

33.3 passes on at least 30 seconds of real standard-format moving video with
sound and verified controls in MyOS. This is media-platform acceptance, not
YouTube or Rickroll acceptance. MYV1 synthetic clips cannot close this stage.

## 33.4 - actual YouTube integration

Use the native engine to open YouTube and a real watch URL. Implement/fix
frontend navigation, JavaScript, media interfaces such as MSE if required,
stream selection, additional HTTPS media hosts, redirects, range/segment
requests, buffering, seeking and recovery based on observed failures.
Handle consent/navigation using MyOS input. Search may be native site search
or the agreed MyOS search path; a direct URL is also accepted by PHASE33.md.
Check frontend/certification restrictions and supported stream formats early.
Do not spoof successful licensing/signatures or replace the frontend with a
host helper. If the selected engine proves incompatible, retain evidence and
revisit the documented fallback with the owner before expanding the port.

33.4 passes when a real online YouTube video can be opened and played with
moving frames and audible sound in MyOS, with pause/resume, seek, close and
reopen demonstrated. Record test date, URL, build, QEMU options, media format,
resolution, frame rate, buffer/memory behavior and network failures.

## 33.5 - final Rickroll acceptance

Use the normal MyOS image, open the MyOS browser, navigate to
https://www.youtube.com/watch?v=dQw4w9WgXcQ and play the actual online video.
Demonstrate >=30 seconds of motion and audible music from the same session,
pause stopping both, resume continuing both, seek changing the timeline and
continuing playback, and close leaving the desktop/shell responsive.
Repeat after a fresh boot to detect reliance on developer-only state.

Evidence bundle: commit/build hash, pinned dependency versions, date, exact
QEMU command and hardware profile, screen recording, captured audio, serial
and relevant network logs, control observations and applicable full regression.
Record RAM/CPU settings as tested configuration, not an unmeasured minimum.
Have the visual/audio/control behavior checked interactively as PHASE33.md
requires. Mark Phase 33 passed only with this evidence. Reconcile Phase 32's
broader search/select/player acceptance separately rather than automatically
closing it from a single Rickroll URL.

## Next session scope and stop point

1. Read AGENTS.md, HANDOFF.md, this plan and the checkpoint diff.
2. Finish the outstanding full regression. Fix regressions before feature work.
3. Audit/fix pthread clocks, masks and pinned ABI behavior; update fixtures/status.
4. Specify descriptor ownership, clocks/deadlines and cancel behavior.
5. Implement the first focused clock/wait slice; expand to eventfd/pipes and
   then poll/epoll through separate tested checkpoints.
6. Update handoff with evidence, failed cases and the exact next slice; commit
   locally when appropriate. Never label a dependency slice a finished phase.

Do not attempt all five stages as one patch or promise completion from the
number of resolved symbols. The renderer and live frontend are feasibility
gates; measurements determine subsequent resource and implementation choices.


### 2026-10-10 01:03 +07 — checkpoint 6; Phase 33.4 remains incomplete

VM-area 1024 eliminated the observed public-session reservation ENOMEM. Native YouTube
144p Stats proves 256x144@30 VP9/Opus; search/select/reopen and both post-close desktops worked.
A/V max 560083 us overall / 351299 us at 144p still FAILS. The bounded VP9 batch candidate
passes native fixture controls/EOS/cleanup; its public sync is pending. Actual idle NIC-loss
baseline was reproduced; ENETDOWN/waiter/reconnect fix passes native BIOS and UEFI.
Full regression is PENDING for VM-area 1024 + carrier fix. Earlier PMM full PASS does not cover it.
See `docs/PHASE334.md`, `docs/HANDOFF.md` and `docs/wip/phase334/checkpoint6/` for commands,
source identities, evidence and remaining online recovery/sync gates. No Phase 33.5/33/Rickroll acceptance.
