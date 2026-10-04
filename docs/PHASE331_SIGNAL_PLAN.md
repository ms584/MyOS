# Phase 33.1 signal ABI groundwork and delivery plan

Updated 2026-10-04; follows a536b93. Delivery below is DESIGN, not implemented.
Later implementation/evidence: PHASE331_SIGNALS.md supersedes delivery status.
Phase33.1/33.2 remain OPEN; no new core imports or actual Cobalt acceptance.

## Implemented and measured in this checkpoint

SDK signal.h now supplies sigaddset and pinned x64 signal object layouts.
No signal/sigaction/raise/pause/abort provider or prototype is supplied.
The constants/layouts do not promise delivery or supported sigaction flags.
Pinned source: build/engine-evaluation/contracts/
starboard_layer_posix_signal_abi_wrappers.h, SHA256
b076da2463a4e03d8917323314e3383919875d777702477e675ba98bc1140e3e.
Exact definitions (not host libc) are compared in tests/test_signal_sdk.py:
sigset128/align8, siginfo128/align8, sigaction152/align8;
handler0, mask8, flags136, restorer144, nested siginfo field offsets/SA constants.
siginfo field spelling is private SDK layout, not a full public libc header.

Native fixture user/elf-signal-sdk.c tests all127 single-bit add/member/delete
positions, invalid bounds/NULL, errno and output preservation, mask canaries,
dirty padding, aliased set/old output, query-only how, unblockable KILL/STOP,
and eight concurrent workers inheriting then isolating masks under preemption.
Final34821 exit0 08:37:33+07: BIOS sdk-1noj828y + UEFI sdk-lokp4w8s,
C11/C++17 six launches/profile with exact heap/page recovery; integer headers PASS.
Initial attempts failed fixture indentation/name checks, not native runtime;
fixed without suppressing warnings or weakening assertions.
Kernel/runtime/bootstrap and normal BIOS/UEFI hashes are unchanged from
PHASE332_EXIT_SDK.md; frozen test images never replace shared images.
No architectural change: reuse uninterrupted full78177 PASS, do not rerun it.

## Current gaps and integration points

pthread_sigmask stores two supported words per thread at private TLS page+384;
it clears output padding and KILL/STOP but does not deliver pending signals.
pthread_kill only implements signal0 validation. Core four imports stay missing.
process_fault currently terminates every ordinary user exception; no handlers.
scheduler waits distinguish address wake/timeout, not signal interruption.
schedule maintains task FXSAVE512 images, not nested interrupted-handler state.
Runtime is RX (mutable globals forbidden); no host libc/kernel signal machinery.

## Ordered delivery slices and acceptance (all pending)

1. Resolve layout capacity before adding architecture. Normal kernel252952;
   PROCESS_TESTS kernel261144 vs262144 cap (1000 bytes). BSS end0x7d2a0 vs
   BIOS stack0x80000; only11616 bytes of BSS headroom. Runtime29136/65536,
   but embedded runtime growth also consumes kernel image. Do not raise a cap
   or remove regression coverage merely to fit. Measure section/map sizes;
   any layout revision must audit boot/stage2.asm CHS/EDD segment destinations,
   linker bounds, BIOS stack/DMA, UEFI allocation and low-memory full regression.
2. Add native kernel signal state/operations: process-wide dispositions;
   per-task masks/pending/coalescing and actual termination reason separate
   from exit status and CPU fault vector. Lazy bounded allocation, rollback
   and teardown accounting. Kernel is authoritative; replace runtime-only
   masks atomically, including inheritance before a new worker becomes runnable.
   Validate full guest ranges/handlers, signums, flags and uncatchable signals;
   signal0 checks lifetime without queuing. SDK helpers do not imply that every
   1..127 mask bit has an implemented action. Document unsupported delivery.
   Test DFL/IGN/handler ordering, blocked/unblocked pending, invalid untouched
   outputs, stale worker handles and cancellation/relaunch/OOM.
3. Implement a single user-return delivery path for syscall return, scheduled
   IRQ return and synchronous fault redirection. Save all15 GP registers,
   RIP/RSP/RFLAGS, actual FS/TLS identity and x87/SSE FXSAVE512 before handler;
   use owned nested context records, red-zone-safe aligned user stack and
   fixed trusted trampoline/sigreturn. Existing scheduler FP slots alone do
   not preserve the interrupted FP image across a handler. Never restore
   guest-supplied privileged CS/SS/IOPL/CR3/FS or unchecked MXCSR in kernel.
   Define the real callback context ABI before exposing SA_SIGINFO/resumable
   faults. Bound nesting; invalid/unmapped stack/context or allocation failure
   terminates only that process, with deterministic reason and resource recovery.
   Test register/FP/errno/TLS preservation, nested/preempted handlers, readonly/
   guard faults, tampered return, cancellation and exact resources. Reject
   SA_ONSTACK/RESTART/etc until their actual semantics pass; no ignored flags.
4. Wire signal-aware waits without changing descriptor lifetime: wake targeted
   task with an explicit interruption reason, retain partial completed I/O;
   restart policy depends on operation, not unconditional retry. pthread waits
   reacquire mutex and hide EINTR per contract; pause returns EINTR after a
   handler, sleeps preserve remaining duration/absolute deadline. Test pipes,
   sockets, poll/epoll, condition locks, close/reuse and lost-wakeup races.
5. Bind core signal/sigaction/raise/pause only after native contracts pass.
   abort raises real SIGABRT first; if blocked/ignored/handler returns, force
   default SIGABRT termination, not UD2/fake exit134. Test handler non-return,
   return/block/ignore/default, all workers stopped and no finalizer invocation.
   Recheck pinned bindings, affected renderer units, full system regression,
   normal image restoration. No signal fixture closes Cobalt web acceptance.

Relevant code: memory/process.c, cpu/scheduler.c, cpu/interrupts.c,
kernel/syscall.c, include/process.h, include/os.h, user/platform-runtime.c.
Next concrete work: section/layout measurement, then slice2; no Phase33.3/push.
