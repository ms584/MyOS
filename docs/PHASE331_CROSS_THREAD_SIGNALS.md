# Phase 33.1 next slice: targeted signals and interruptible waits

2026-10-05 08:47 +07. PRIVATE PROTOTYPE TESTED; NOT INSTALLED IN ACTUAL REPO.
Do not install code while DNS full architecture handle79950 uses shared sources.
Native Cobalt remains0/5; no33.3. Read PHASE331_SIGNALS.md for actual semantics.

## Isolated prototype — 08:29:38 +07

Later private image builds/proof supersede compile-only status below:
- Private image18567 exit0 08:37:35, /tmp/myos-cross-image-mGxmcs:
  kernel261662/cap262144 (482 bytes free), raw34272/packed28176.
- Native44083 exit0 08:38:33, build/cross-um657cux: four launches, exact
  heap/pages; targeted send, blocked coalescing, raw pause syscall, relative sleep
  remainder, sem/pipe/poll/epoll EINTR, condition reacquisition and timed mutex
  hiding EINTR, worker->main send and stale reaped handle PASS.
- Added pause export PRIVATE generator work/exports-cross.py; image33230
  exit0 08:39:27, /tmp/myos-cross-image-19gJ3m, same kernel/runtime budget.
  Native98461 exit0 08:40:16, build/cross-c691iud6: same four-launch fixture
  using actual pause() export/errno (not raw syscall) PASS.
- Initial image4xnPPG/fixture95736 failed before entry with apparent OOM:
  memory directory symlink made relative ../build header use original runtime
  size. Fixed private build to COPY memory directory. No production source fix
  was needed; failing artifacts retained. No RAM increase/assertion weakened.

Further proof on19gJ3m:
-62914 exit0 08:43:45: cross-y7lgd08e UEFI four relaunches/exact resources PASS;
 existing delivery-5qk_hian BIOS and delivery-aiburv9q UEFI each36 launches/
 6 cancels,15GP/DF/16XMM/x87/MXCSR, fault repair/nesting/SIGABRT/forged return/
 worker-exit slot reuse/exact resources PASS (unchanged existing fixture).
-15674 exit0 08:45:45: strengthened BIOS cross-zfl4m0kg + UEFI cross-mh_ffp45
 each four relaunches PASS: exited-unreaped/reaped liveness, foreign/full-width
 tid, reserved fields/high signal bits/invalid pause, ignored signal does not
 wake pause, blocked coalescing. Fixture warning fixed with braces, not Werror
 suppression. No production source changes needed for these checks.
-63584 exit0 08:47:50, native DNS cross-thread EINTR/owned-job cleanup PASS:
 private-dns-interrupted.log then private-dns-interrupted-uefi.log.
 Windows work/elf-netdb-interrupted.c + test_netdb_interrupted.py;18 real
 getaddrinfo interruptions per launch, sentinel output/errno and slot reuse.
 BIOS dns-z71314jy + UEFI dns-vu9ktab3, three normal launches per profile
 plus forced cancel/relaunch and exact resources;54 interrupted calls/profile.

Main lifetime correction staged after the above image:
-Kernel now owns main_thread_exited (uses existing process padding); SYS_SIGNAL7
 marks logical main exit after destructors, clears abandoned main contexts, and
 prevents later sends/delivery while runtime task waits for workers/finalizers.
 Worker callers/reserved arguments are rejected; main join result is preserved.
-Private64057 exit0 08:50:58, /tmp/myos-cross-image-xPZum8:
 kernel261726/cap262144 leaves418 bytes; raw34304/packed28211, BSS194416.
-Native3669 exit0 08:54:06: BIOS cross-9mkoa19v + UEFI cross-t1pba9go
 each four normal signal launches AND four logical-main-exit launches PASS;
 kernel raw and pthread signal0/nonzero return ESRCH after main join, worker
 survives, process returns0, exact resource recovery. BIOS and UEFI existing
 thread-services PASS (join/detach/TSS/finalizers/timed waits/cancel). No production install.

Still pending: before-wait race coverage, integration and
 mandatory full after DNS checkpoint. Do not blindly promote prototype as complete.
Actual repo imports remain188/242 fixture and190/242 binding; Cobalt0/5.

Windows work/signal-cross.c, scheduler-cross.c, process-cross.h and
platform-runtime-cross.c implement a proposed targeted-send/pause/pending check
and runtime EINTR paths in a PRIVATE COPY ONLY. Not installed in actual repo,
no actual repo exports/import status changed. Compile script:
work/compile_cross_signal.sh (through tools/run_logged.py from actual repo).
First runtime compile failed relative include; corrected -Iuser, then all three
units compile with target flags/Werror. Probe /tmp/myos-cross-signal-GoAxK7:
text signal2824, scheduler1959, runtime24767; scheduler BSS23701.
These non-LTO object sizes are NOT final kernel/runtime budget measurements.
Do not blindly install: measure private linked image budget, audit requirements,
add native tests, wait for DNS full79950 terminal, checkpoint DNS first.

## Evidence-derived gaps

user/platform-runtime.c pthread_kill validates a runtime control mapping but
returns EINVAL for every nonzero signal. Its signal0 path does not query native
thread liveness, so an exited but unreaped control block can appear valid.
Kernel signals own process dispositions/per-slot masks/pending/handler contexts.
Only self sends exist. process_wait validates/copies expected words under the
syscall serialization boundary then scheduler_wait blocks; no pending-signal
check. scheduler_wake is address-based, and timeout returns-110.

Adding sends alone is insufficient: it leaves a blocked target permanently
asleep. Adding wake alone is insufficient: signal-before-wait must not be lost.
Do not wake every waiter on the same descriptor sequence when targeting one tid.
Do not infer the target from a reused slot, foreign user pointer or truncated tid.

Current runtime consequences of new-4(EINTR), requiring coordinated fixes:
- pthread_mutex_timedlock would expose EINTR (must not); untimed lock retries.
- cond_wait reacquires mutex and returns0 for non-timeout; verify permitted
  spurious wake behavior rather than falsely claim notification occurred.
- sem_take, poll and epoll_wait currently retry every non-timeout error: they
  would swallow EINTR, and could spin on persistent invalid address errors.
- clock_nanosleep returns an error but does not calculate remaining relative
  duration; absolute sleeps must retain their original deadline.
- DNS resolve has a close-on-error path; native cross-thread interruption has
  NOT been exercised. Earlier failed test: resolver-native-signal-gap.log.
- Every socket/pipe/eventfd/connect wait loop must be audited for partial I/O,
  lifetime pins and close/reuse when returning EINTR. No universal retry policy.

## Proposed coherent implementation

1. SYS_SIGNAL new operation takes full-width native tid and signal, reserved
   fields zero. Main target uses process pid; workers use monotonic compound tid.
   Locate only PROCESS_THREAD_RUNNING in the caller process; ESRCH on stale or
   foreign identity. Signal0 checks existence without allocation/queue/masks.
   Runtime maps main handle and worker control->tid to this operation. Keep
   pthread_kill direct positive error returns and preserve caller errno.
2. Add scheduler targeted interrupt(p,thread): exact task identity, blocked only,
   saved frame->rax=-4, alive=true, blocked=false. No guest-address dereference.
   Queue only nonignored valid signals. Interrupt only if deliverable under the
   target's current authoritative mask; blocked sends remain pending/coalesced.
   Single-CPU IRQ-disabled serialization must cover queue+interrupt atomically.
3. Expose authoritative pending-deliverable query for current task. Check before
   scheduler_wait commits blocking, after expected-word comparison; this closes
   signal-before-wait without inventing a mutable guest pending flag.
4. Implement pause as atomic pending check plus targeted interruptible sleep,
   no guest scratch word race. Return EINTR after actual handler delivery, not
   on ignored/blocked signals, unrelated descriptor wakes or timer ticks.
5. Audit each runtime wait category as above; conserve existing deadlines and
   exact reference releases. No SA_RESTART/SA_SIGINFO/altstack claim in this slice.
6. Resource/budget gate first: DNS PROCESS_TESTS kernel261336/cap262144 leaves
   only808 bytes (observed full log08:24:23). No cap increase/test deletion.
   Measure compiled implementation and BSS before installing/expanding scope;
   if insufficient, resolve genuine layout/code budget with full verification.

## Required native tests (not host substitute)

- Main->worker, worker->main, worker->worker; masks inherited/isolated, blocked
  coalescing/unmask delivery, ignored discard, caller errno and signal0 liveness.
- Before-block/while-block/after-ready races, competing timeout/send, handler
  register/FP/TLS restoration; nested/preempted existing delivery suite retained.
- Stale exited/unreaped/reaped/reused tid, malformed/foreign identity, full-width
  generation, invalid signal0 no state allocation, OOM rollback/force cancellation.
- pause handler/ignored/blocked cases; mutex timed and condition reacquisition;
  sem EINTR without consuming count, sleep remaining/deadline, poll/epoll EINTR.
- DNS blocked worker signal closes owned job, real retry,16-slot capacity proof;
  pipe/socket/eventfd partial I/O, close/reuse, no lock/reference/page/heap leak.
- BIOS+UEFI scoped; architectural full, normal restore/hashes, docs/local commit.
  Then continue remaining imports/resources and actual Cobalt web gate.
