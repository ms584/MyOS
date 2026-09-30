# Phase 33.2 single-image finalizer lifecycle

Implemented dependency slice; native focused tests and full regression PASS.
Phase 33.1/33.2 remain OPEN. No Cobalt web execution or Phase 33.3 is claimed.

## Binary evidence and scope

The pinned libcobalt.so has DT_FINI at offset 0x35aa0bc, eleven init-array
entries and no PT_TLS/DT_NEEDED. The existing loader rejects DT_FINI after
resolving imports; implementing it removes a real later startup blocker, not
the 103 currently missing imports or the absent SbEventHandle launcher.

This slice accepts executable in-image DT_FINI and up to 128 relocated
DT_FINI_ARRAY pointers, validates their sizes/spans/targets before any user
constructor runs, and checks init/fini offsets before adding USER_BASE to avoid
integer wrap into a seemingly valid address. PT_TLS, preinit, IFUNC, dependency
loading and unrelated relocation kinds remain rejected.

On a normal return from the custom ELF entry, the Ring 3 trampoline preserves
the signed return status, invokes the fini array in reverse order, then DT_FINI,
and finally exits the process. Each finalizer is called once on that path.
This follows the [ELF initialization/termination order](https://refspecs.linuxfoundation.org/elf/gabi4+/ch5.dynamic.html#init_fini).
It is not a complete libc exit/atexit or C++/TLS destructor implementation.
Direct SYS_EXIT/_Exit, constructor/entry/finalizer faults, timeout and Escape
force-cancel do not invoke or restart finalizers; kernel teardown still owns
all process resources. A looping finalizer remains cancellable and bounded in
elf run. Cooperative engine stop must eventually return through this path.

## Test design and current status

`user/elf-finalizer.c` checks init -> two constructors -> entry -> second
destructor -> first destructor -> fini, clobbering return registers through
real serial writes. Variants isolate fini-only and array-only behavior.
Native tests must require status 41, exactly one PASS marker, repeat-launch
page/heap equality, and unchanged read-only asset volumes.

Malformed fini address, wrapped init/fini, array span/size/count and relocated
non-executable target must reject before execution. Faulting/looping finalizers,
looping constructor/entry, direct process exit, demo timeout and Escape cancel
must recover to the shell without spuriously reporting finalizer success.
Finalizer fixtures use a separate <=32-entry asset volume rather than changing
the production asset format limit. Existing runtime fixtures remain intact.

Focused tests including native finalizers, existing runtime/lifecycle fixtures,
8 KiB binary reboot persistence, pinned Cobalt 256 MiB staging and 64 MiB OOM
rollback passed at 20:24 +07 on 2026-09-30. Full tests/test_system.py passed at
20:53 +07 and restored the normal framebuffer image (kernel 232368 bytes).
This checkpoint is local only; the owner handles any push.
