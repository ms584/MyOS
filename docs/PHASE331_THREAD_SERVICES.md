# Phase 33.1/33.2 native pthread services

2026-10-01, work from local abf3cd9. Phase 33.1/33.2 remain OPEN.
This is a renderer dependency slice, not complete POSIX/C11 or Cobalt acceptance.

## Contracts

- pthread_mutex_timedlock uses an absolute CLOCK_REALTIME deadline, converted
  once to the kernel's monotonic tick deadline. It blocks via compare/wait,
  rather than a busy loop or restarting its timeout after a wake.
- Immediate acquisition succeeds without inspecting the deadline. Contention
  validates nanoseconds and rejects seconds-plus-nanoseconds overflow. Past
  deadlines time out; recursive/error-check ownership retains its existing
  rules. Pthread errors return directly, leaving errno unchanged.
- Shared checked time conversion also rejects uint64 nanosecond overflow in
  relative/absolute clock_nanosleep. Timer granularity remains 10 milliseconds.
- pthread_exit publishes the worker result and runs the existing four-pass
  TSS destructors before terminating. Returning from a worker uses the same
  path. pthread_equal compares the runtime's opaque identity values.
- Main pthread_exit publishes its join value, permits main join/detach, waits
  internally for wrapper-created workers, then resumes the loader-owned
  finalizer continuation with process status zero. Application frames do not
  resume. Process-level cancellation still tears down all tasks/resources.
- Join and detach claim a single atomic ownership word. Independent flags
  previously allowed both claimants to win. The claim test keeps the target
  alive through both calls; it does not use an expired post-join handle.
- Main private page offsets 16/24/40/56 hold worker count/result/flags/owned
  frame. Native FS TLS and the existing errno, keys and signal masks remain
  separate. No mutable state is introduced into RX runtime pages.

The main exit continuation is installed after constructors and SABI validation.
Main pthread_exit during constructors/preflight is unsupported and traps.
No pthread cancellation API, cleanup-handler stack, signal delivery, dynamic
TLS module, __cxa_thread_atexit or C++ unwinding support is claimed. Raw kernel
thread creation is outside the wrapper worker-count contract. The main helper
retains the loader stack while waiting; it is not a scheduler thread replacement.

## Size and evidence

The first PROCESS_TESTS=1 image was 262392 bytes, correctly rejected by the
unchanged 262144-byte loader bound. Shared time/tick/vector-I/O helpers reduce
duplication; compiling only platform-runtime.c with GCC -Oz reduces size
further. No loader/BSS/stack/DMA limits are relaxed. Both variants build:
normal kernel 252952 bytes, process-test kernel 261144 bytes (09:42:54 +07).
The normal framebuffer image was restored after this size check.

Native combined fixture PASS 09:48:03 +07: three launches each of main exit,
main detach, timed mutex and 12 join/detach contention waves, repeated TSS
destructors, worker results and finalizers; realtime versus monotonic deadline,
overflow/errors/errno; blocked timed-worker/main-exit cancellation and relaunch;
exact page/heap reclamation. tests/test_thread_services.py is included in
the full apps group and supports --uefi. Standalone preliminary tests passed
09:29:24 (exit) and 09:36:54 (timed mutex), before the final ownership/size edits.

Fresh uninterrupted tests/test_system.py PASS on the frozen changed source.
Its pipeline 52428 (full, SDK guard, UEFI build and native pthread/TLS) exited 0
at 10:23:34. All fault/VGA/storage/16+256 MiB and full apps checks passed;
the normal framebuffer image was restored. Extra UEFI guard/launcher/events/UI
pipeline 9769 exited 0 at 10:26:06. Actual pinned Cobalt staging/refusal and
64 MiB OOM rollback pipeline 2578 exited 0 at 10:26:26: still 160 resolved,
82 strong missing, no Cobalt execution. All handles are terminal.
This is fresh coverage for the changed source, not reused abf3cd9 evidence.
Normal kernel/runtime/BSS: 252952/29136/0x7d2a0; image SHA256
0849bb23f54b1f9c0bdf10b7c89f881263c3209827d8d3eaa9466f10da61fa6a.

Relevant files: user/platform-runtime.c, user/platform-tls.asm,
user/elf-start.asm, memory/process.c, build.sh, tools/make_runtime_exports.py,
user/elf-thread-{exit,timed,claim}.c and tests/test_thread_services.py.

Semantics reference: [pthread_mutex_timedlock](https://pubs.opengroup.org/onlinepubs/009695199/functions/pthread_mutex_timedlock.html)
and [pthread_exit](https://pubs.opengroup.org/onlinepubs/009696899/functions/pthread_exit.html).
