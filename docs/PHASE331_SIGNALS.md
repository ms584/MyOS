# Phase 33.1 native self-signal delivery

## Latest targeted-signal integration — 2026-10-05 09:08 +07

After tested DNS local2a71206: native pthread_kill now translates runtime handle
to full-width live kernel tid; signal0 checks native liveness without allocation.
SYS_SIGNAL5 queues only in caller process and wakes only the targeted task with
EINTR when unmasked/nonignored. scheduler_wait checks authoritative pending
delivery under IRQ/syscall serialization before committing blocking. SYS_SIGNAL6
supplies atomic pause; native pause() returns-1/errno EINTR after handler.
SYS_SIGNAL7 marks logical main pthread_exit, clears abandoned main contexts and
rejects subsequent sends while its task waits for workers/owned finalizers.
Kernel masks/dispositions/return contexts remain authoritative. No new mutable
runtime globals, guest saved-register trust, SA_RESTART/SIGINFO/alt-stack claim.

Runtime timed mutex hides EINTR; condition wait reacquires mutex/spurious wake;
semaphore, pipe/eventfd/socket waits, poll/epoll and DNS return interruption and
release lifetime pins/jobs. Relative clock_nanosleep calculates remaining
duration, absolute remaining output is ignored. Existing deadlines preserved.

Scoped16585 exit0 09:06:05 production BIOS/UEFI PASS: each16 cross-signal
launches/exact resources including stale/foreign/invalid syscall, ignored/blocked,
pause/sleep/semaphore/pipe/poll/epoll, pthread waits, main exit and default15
main/worker termination without FINI. DNS54 interrupted getaddrinfo calls/profile
plus cancel/relaunch PASS; old thread and36-launch/6-cancel handler context suites
PASS both profiles. Logs build/cross-signal-*.log; detail PHASE331_CROSS_THREAD_SIGNALS.md.
New full/pair job7164 started09:07:27, currently pair then full; NOT full PASS yet.
Normal kernel253528/runtime34304/packed28211; private test261726 leaves418 bytes.
Fixture imports189/24278.1%; actual Cobalt0/5, phases OPEN, no33.3/push.

Everything below is historical self-delivery checkpoint evidence, not the latest
unsupported-cross-thread/pause status or current image hashes.

Implementation52267bb follows d506dd5, 2026-10-04. NOT phase closure.
Full regression97678 exit0 09:55:59+07; normal BIOS/UEFI restored.
Final scoped66362 exit0 09:57:35; all jobs terminal, no shared-image owner.
Renderer49393 remains a terminal compile failure; see evidence below.

## Actual supported contracts

Native SYS_SIGNAL49 and kernel/signal.c now own process-wide dispositions,
per-task masks, coalesced self-pending signals and handler return contexts.
raise, signal and sigaction have real fixed-address runtime providers.
The import register marks their TESTED SUBSETS, not full POSIX semantics.
pause and nonzero pthread_kill/cross-task delivery remain unimplemented.
Supported delivery: numbers1..31 except19..22 (no stop/job-control service).
Mask storage still accepts1..127 with padding cleared and KILL/STOP unblocked.
Default CHLD/CONT/URG/WINCH are ignored; other supported defaults terminate.
KILL cannot acquire a handler; setting IGN discards the corresponding pending
bit in every task. Blocked self sends coalesce; unmask delivers before return.
Dispositions are process-wide, masks inherited in kernel before worker startup.
No user-writable mask or saved-frame copy is authoritative anymore.

Pinned 152-byte sigaction ABI and 128-byte masks retained. Whole input/output
ranges are checked before changes, including aliasing; query does not allocate.
Support: simple sa_handler, sa_mask, SA_NODEFER and SA_RESETHAND.
Reject SA_SIGINFO/ONSTACK/RESTART/NOCLDSTOP/NOCLDWAIT/custom restorer with95;
do not silently accept flags. Invalid signums/unblockable changes return22;
invalid guest pointers/nonexecutable callbacks return14. pthread_sigmask returns
errors directly, while signal/sigaction/raise set errno on failure.
signal() does not advertise restart behavior; pending task sends are self only.

## Interrupted state, faults and teardown

All user-return syscall/IRQ/fault paths check pending delivery. Each active
handler has a bounded kernel-owned context: all15 GP registers, RIP/RSP/
RFLAGS and512-byte x87/SSE FXSAVE image, previous mask and nesting chain.
Guest memory holds only a restorer address and user-address token, not trusted
register/FP state. sigreturn checks the top task's token before restoring;
guest pointers cannot supply privileged selectors/flags or unchecked MXCSR.
Existing FS/TLS identity is not changed by delivery. No AVX/XSAVE claim.
Handler entry is8mod16 and skips the interrupted128-byte red zone. DF is
cleared for handler entry and restored afterward. Maximum four nested contexts;
allocation, invalid stack/callback or nesting exhaustion terminates only that
process with signal11. Context/state allocation is lazy and reclaimed on
thread exit/slot reuse and process reap, including forced cancellation.
Normal RETURN-based handlers only: no siglongjmp/sigaltstack/context-edit ABI.

Ordinary Ring3 faults map divide/x87/SIMD toFPE, opcode toILL, debug/breakpoint
toTRAP, alignment toBUS and protection/page faults toSEGV. A simple unblocked
registered handler may repair a known mapping and retry the original instruction.
Uncaught/ignored/blocked faults retain the previous fatal fault/isolation path.
No siginfo fault address or editable ucontext is exposed; SA_SIGINFO remains
an explicit unsupported error, not a NULL/foreign fabricated context.

Native abort first raises real SIGABRT. If its handler returns, ignores or
blocks it, a second allocation-independent service forces default signal6
termination and stops all workers. It neither raisesUD2 nor reports exit134.
No FINI is run on this termination; a non-returning handler can explicitly
exit the process instead. A handler endlessly re-sending itself is itself
non-terminating; cancellation remains available.

## Evidence so far (all +07, 2026-10-04)

- Masks39400 exit0 09:02:26: BIOS sdk-td5hwjrc/UEFI sdk-vwkznhp9, exact pinned
  layouts C/C++,127 bits/guards/errno/aliasing/eight inherited isolated workers.
- Expanded delivery78959 exit0 09:15:46: BIOS delivery-gunzl9cv and UEFI
  delivery-e8y145eo. C11/C++17,36 launches/profile,6 handler cancels/profile,
  masks/coalescing/IGNORE discard/NODEFER/RESETHAND, preempted handlers;
  assembly deliberately overwrites15GP/16XMM/x87/MXCSR and yields, proving
  interrupted restoration including DF; readonly fault repaired and retried.
  Real ABRT default/return/ignore/block/worker/nonreturn; forged return rejected
  with and without live context;16 pthread_exit-in-handler slot reuses per
  launch; four-level nesting exhaustion; exact heap/pages each launch/cancel.
- Earlier fixtures had NASM/include/initializer issues; fault counter needed
  volatile because delivery interrupts an ordinary store. An intentionally
  endless self-resending ABRT fixture was traced, controller PID49734 stopped
  with SIGINT/finally, then replaced by one bounded resend and explicit
  non-return cases. No failed/interrupted run is recorded as PASS.
- Pinned static check09:17 PASS: ELF/header hashes unchanged;163 strong
  fixture-covered bindings,79 strong missing plus weakTLS. 163/242=67.4%
  is coverage only. Actual Cobalt HTML/JS/DOM/input/HTTPS acceptance stays0/5.
- Renderer49393 compile1 09:23:29 target-config-ybt5nn3q:13 failed units/16
  errors (prior21/29); abort errors clear. No native renderer execution.
- Full97678 exit0 09:55:59: uninterrupted test_system.py including low RAM,
  fault variants and VGA fallback, normal BIOS/UEFI restoration and pinned
  Cobalt staging. Actual core maps/relocates at256MiB, missing79 strong imports
  prevent execution; staged pages reclaimed.64MiB OOM rollback leaves apps usable.
- Final strengthened66362 exit0 09:57:35: BIOS delivery-ft2by0y0/UEFI
  delivery-83dghx44, same36 launches/6cancels per profile with explicit RAX
  overwrite and exact full-RFLAGS comparison, all assertions PASS. Frozen boot
  and shared normal image hashes are unchanged; no extra full rerun needed
  because only the fixture was strengthened after the architectural suite began.

Normal SHA256 after full and stronger scoped runs:
```
kernel.bin           1f3ab7bb4a5e16d82726f2d7773dcdfe453fc4fb35157e192a9d3abca6ea9170
platform-runtime.bin 41b9f22d7d8f5b4414f90f0d36f5efcc8426783e762bb2dc038dd4061da91746
myos-current.img     ab64221d6c9b910cd8746ecd9ac3de55411f185d18e0d93afd6670a26ee5802f
myos-uefi.img        37908e9f86ca31f6ebd600e0b3f77c7e663150befa562013ce85dc96ff398cf0
```
Reproduce: tools/run_logged.py -- python3 tests/test_signal_delivery.py
and the same with --uefi; masks via tests/test_signal_sdk.py [--uefi].

## Capacity and next scope

Section-GC experiment saved80 code/160BSS bytes but no image; reverted.
Kernel-only -Oz (runtime already used it) saved2000 code bytes and4096 image
bytes in PROCESS_TESTS profile before signals. TLS/compiler features/caps,
linker/boot reservations and every regression assertion were preserved.
Current normal kernel253080; test kernel261272, cap262144:
872 bytes test-image headroom. Normal runtime29104; kernel text178987,
rodata37352/data35992/BSS194208; BSS end0x7d340 below stack0x80000.
Do not grow statically allocated state or casually raise boot/layout limits.

Next: actual cross-task sends/interruptible waits/
pause, SIGINFO/context
and restart/alternate-stack requirements as needed by Cobalt; native renderer
dependency/OS-info/C++/EGL work and actual web endurance gate remain open.
Read PHASE331_SIGNAL_PLAN.md as historical ordered plan, this doc as actual state.
Relevant: kernel/signal.c, memory/process.c, cpu/interrupts.c,
user/platform-runtime.c, user/platform-tls.asm, tests/test_signal_delivery.py.
No Phase33.3, push, native renderer/backend or Cobalt acceptance claimed.
