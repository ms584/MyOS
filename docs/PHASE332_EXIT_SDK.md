# Phase 33.2 owned-image normal and raw process exit

2026-10-04 tested checkpoint follows0643303; full regression PASS.
Phase33.1/33.2 OPEN. No33.3, SIGABRT, Cobalt execution or import-count increase.

## Implementation / private ABI
- user/platform-exit.c supplies SDK exit/_Exit, no host libc or process helper.
  exit runs the owned __cxa_finalize registry, remaining reverse FINI_ARRAY,
  DT_FINI, fflush(NULL), then native SYS_EXIT. _Exit directly SYS_EXIT, skipping
  callbacks/streams/finalizers. SYS_EXIT stops ALL threads, including blocked
  workers; it is not pthread_exit and does not wait for workers to finish.
- ELF bootstrap publishes its owned frame in runtime TLS+56 after SABI
  validation and BEFORE constructors. Entry arguments/ABI remain unchanged.
  Frame slots0/5/6 are remaining fini count/DT_FINI/fini-array pointer.
- Both normal-return assembly and SDK exit consume each FINI entry BEFORE
  calling it; consume DT_FINI before calling it. Reentrant exit from an atexit
  callback or from an already-running normal-return FINI cannot repeat it.
- SDK reads bootstrap metadata, but executes callbacks on its own caller stack,
  including worker exits. Does not jump onto a sleeping main thread's stack.
  Other threads may run until process termination; arbitrary callback deadlocks
  remain caller concerns. Concurrent competing exit calls are NOT guaranteed.
- This is the owned single-image SDK profile, not a new pinned core binding,
  dependency loader, dlclose, C++ automatic-object unwinding or foreign FILE ABI.
  SABI preflight refusal still bypasses finalizers. SDK exit requires the owned
  published frame; exit inside the preflight getter is not supported.
  abort remains undeclared/unimplemented: no fake SIGABRT via an opcode fault.
  Import register checked08:00: pinned core requires raise/signal/sigaction,
  all still unimplemented; no abort/exit/_Exit rows. SDK exit is renderer work,
  NOT evidence those core signal imports are implemented. Sealed musl abort
  calls the handler first, then resets default/unblocks/re-raises if it returns.

## Native evidence (+07)
Commands run through tools/run_logged.py in /home/xytron404/myos:

    python3 tests/test_exit_sdk.py
    python3 tools/build_uefi.py
    python3 tests/test_exit_sdk.py --uefi
    python3 tests/test_cxx_lifetime.py

Final session74114 terminal exit0 07:50:41.042762:
BIOS sdk-kk_gd8jl and UEFI sdk-0ecedzea PASS,24 launches/profile +3cancels.
Main/worker/constructor exit77; recursive callback exit78; exit during an SDK
FINI and during a normal-return FINI preserves once-only progress/status78.
Reverse registered callbacks -> reverse FINI_ARRAY -> DT_FINI PASS exactly once.
Raw main/worker _Exit(-77) skips PASS/callbacks. Exit kills a genuinely blocked
pipe reader and allocated memory/handles; every launch has exact heap/pages.
Looping finalizer remains Escape-cancellable and relaunches recover resources.
C++ allocation/DSO/concurrent/reentrant/atexit/OOM/cancel regression PASS,
native-ykmvwhly. Initial smaller13413 also passed21 launches but was superseded.
Targeted Mesa5 units compile0 07:49:56 (three lexer units, dd_screen, u_tests).
No native renderer/backend claim from compile success.

## Full regression / next agent
Full78177 launched07:51:03 and terminated exit0 08:23:55.928912:
uninterrupted test_system.py -> build_uefi.py -> normal artifact SHA256 PASS.
Includes original loader/finalizer/main-pthread_exit/TLS/UI/events/launcher,
network/storage/media/16-256MiB/fault/VGA checks. No assertions removed.
Normal BIOS/UEFI restored; no running jobs or shared-image owner.
kernel.bin SHA256 0ab88b29379437903add3b2e2a44bce183051cfabae18a16acf74c737c8c99be
runtime.bin SHA256 f59f749181eeea1b5e71079fc0ff24c1a36c49134141b23f50da517a4823866a
BIOS SHA256 abd0f7add9a9c7c9a5fb4f86d967c030a4c4f2cf6344860bed2e96580ec045c8
UEFI SHA256 fad754756ae7348804aa5bedcb86e81ccdd1e45162c49a56312fec76fbd9cdeb
Renderer1929 terminal compile1 07:56:52.456439, target-config-uay8w01f:
configure/boundary PASS,21 failed units/29 errors (prior33/47). Native execution
and backend selection remain false. Remaining abort/OS-info/C++/file locks/
mkdir/_IOLBF; no failure from sprintf/getc/fgets/exit. Original reports retained.
No source changes during full tests. Next: real signal contracts/abort,
OS-info/C++/locking/buffering, then EGL/GLES and actual Cobalt native gate.
