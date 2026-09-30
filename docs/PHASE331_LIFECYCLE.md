# Phase 33.1 explicit long-running application lifecycle

Dependency slice only. Phase 33.1/33.2 remain OPEN; Cobalt does not execute and
the real HTML/JavaScript/DOM/input/verified-HTTPS gate is not passed.

## Behavior and ownership

`elf live ASSET` starts a foreground Ring 3 image without the demo's five-second
deadline. `elf run ASSET` retains its five-second limit; link-only staging is
unchanged. The shell polls PS/2 input while scheduling the application. Escape
stops the entire process, including sleeping worker threads, and then reaps it.
Cancellation returns -125 and the shell reports that all threads stopped.

Process teardown detaches every task before releasing its shared address space,
kernel stacks and process-owned filesystem handles. The loader checks its page
baseline on exit/cancellation/fault and failed staging. Live mode additionally
prints allocated kernel-heap bytes before and after teardown, including early
staging failures. `heap_allocated()` walks heap blocks with interrupts disabled.
Production does not assert equal heap use after every successful application:
an application can legitimately create persistent RAMFS nodes/data.

This is force cancellation, not cooperative SbEventHandle stop events, pthread
cleanup handlers, ELF finalizers or TLS destructors. Non-Escape input is currently
consumed by the shell poll; this is NOT Cobalt keyboard/mouse input delivery.
No desktop integration, background process supervisor or engine launcher is
claimed. Existing fixed runtime/allocator/thread limits remain.

## Native evidence

`user/elf-platform-lifecycle.c` runs in MyOS at 16 MiB, retains a 32 KiB runtime
allocation and an open file handle, and starts three workers blocked respectively
in an empty pipe read, a zero semaphore wait and a condition wait. Its main
thread sleeps between monotonic clock observations and emits a marker only
after more than 60 guest seconds. The host fixture independently checks that
at least 60 host seconds elapsed, then injects a real PS/2 Escape via QMP.

`tests/test_elf_dynamic.py` requires cancellation within five host seconds,
complete page recovery and equal measured kernel-heap use. It pre-creates the
file so a legitimate RAMFS allocation is included in the starting baseline,
then removes it after cancellation to prove the retained file handle is gone.
It repeats launch/cancel twice, cancels an uncooperative constructor, checks
normal exit and RELRO fault cleanup, and stages a 32 MiB load segment at 16 MiB
to exercise early OOM rollback. The legacy constructor timeout still passes.

Focused tests including pinned Cobalt staging passed at 19:51 +07 on 2026-09-30.
Full `tests/test_system.py` passed at 20:17 +07 and restored the normal
framebuffer image (232368-byte kernel). This is runtime evidence only.

## Remaining engine lifecycle work

Implement the pinned Evergreen entry/SABI events, scheduled callback ownership,
cooperative start/stop, native input routing and renderer context lifetime.
Audit PT_TLS/weak TLS initialization, fini/dependency requirements and cleanup
against actual binaries. Repeat endurance and reclamation using Cobalt itself;
the generic long-running fixture is necessary runtime evidence, not that gate.
