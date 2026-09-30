# Phase 33.1 pthread runtime slice

Status: implemented and tested as a dependency slice. Phase 33.1 and 33.2 remain
open until native Cobalt runs the HTML/JavaScript/input endurance gate.

MyOS exports the pthread/semaphore imports required by the pinned Cobalt 27 LTS
image plus realtime/monotonic clock services. The implementation runs in the
fixed Ring 3 platform runtime and uses MyOS thread, wait/wake, timer, RTC and VM
syscalls.

Implemented contracts:

- joinable and detached creation, join, detach, bounded stack attributes and
  detached slot/stack reclamation;
- normal, recursive and error-check mutex behavior;
- condition signal/broadcast/wait, condition clock attributes and absolute
  realtime or monotonic timed wait;
- semaphores and absolute realtime timed wait;
- read/write locks and once initialization;
- per-thread keys, values and four-pass destructor execution;
- self/name/attribute/scheduler inspection plus per-thread signal-mask state
  and inheritance;
- validated stale/invalid handles, one successful join consumer, detached and
  completed-thread reclamation, and cleared values when TLS keys are reused.

The acceptance fixture runs multiple Ring 3 workers, serializes shared counter
updates, verifies return values and destructors, exercises realtime and
monotonic condition deadlines, semaphore wakeups/timeouts, signal masks and
invalid/stale handles, and creates more detached workers than the simultaneous
thread-slot limit to prove reuse. The fixture returns status 35 only after all
checks pass. Process teardown still verifies complete VM/page reclamation.

The current register has 139 fixture-covered strong imports and 103 missing
strong imports, plus a separately tracked weak TLS initializer. Counts do not
prove complete semantics or engine compatibility. Cobalt still does not execute.

## Pinned ABI/error audit, 2026-09-30

The pinned Cobalt `musl_sigset_t` is 128 BYTES (16 x 64-bit words on x64), not
128 bits. pthread_sigmask now writes the entire output object and zeros its
reserved words, rather than leaving 112 bytes untouched. Supported mask state
is signals 1..127; signal 128 and reserved words are discarded and SIGKILL/
SIGSTOP cannot be blocked. Child threads inherit supported mask state without
changing the parent. Query-only calls ignore how. Tests use full-size objects,
surrounding canaries, dirty reserved words, highest valid signal and child
inheritance. This is mask state only: nonzero pthread_kill delivery is not
implemented; the four missing signal imports remain missing.

The exact pinned wrapper source is
[starboard_layer_posix_signal_abi_wrappers.cc](https://github.com/youtube/cobalt/blob/3befd332486952f6603ae3baf2a8713878d6e200/starboard/shared/modular/starboard_layer_posix_signal_abi_wrappers.cc).
Its helper iterates signals through 127; its comment saying 128 bits does not
override the actual 128-byte typedef.

Semaphore failures now return -1 and set errno: NULL/invalid initial values
use EINVAL, unsupported process-shared semaphores use ENOSYS, timeout uses
ETIMEDOUT and posting at INT_MAX uses EOVERFLOW without changing the value.
An atomic compare/exchange prevents two concurrent posters overflowing the
counter. The fixture checks exactly one successful post and one thread-local
EOVERFLOW at INT_MAX-1, plus invalid/timed-wait cases. See primary manuals for
[sem_init](https://www.man7.org/linux/man-pages/man3/sem_init.3.html) and
[sem_post](https://man7.org/linux/man-pages/man3/sem_post.3.html).

pthread_setname_np validates the whole name before mutation (max 15 bytes plus
NUL). pthread_getname_np requires the 16-byte name buffer and leaves a short
buffer untouched. These pthread APIs return errors directly without setting
errno; tests preserve sentinel errno/output/name values on failure. prctl
PR_SET_NAME still truncates to 15 bytes, as its separate contract requires.
See [pthread naming](https://man7.org/linux/man-pages/man3/pthread_setname_np.3.html).

Focused native fixtures and pinned staging passed at 19:51 +07. Full regression
passed at 20:17 +07 and restored the normal framebuffer image.

Validation:

- python3 tests/test_elf_dynamic.py
- python3 tests/test_elf_dynamic.py --cobalt build/engine-evaluation/libcobalt.so
- full python3 tests/test_system.py (normal framebuffer image restored)

Still open: ELF TLS relocations and TLS destructors owned by the loader,
remaining file/storage/system APIs, sockets/network, Starboard launcher/SABI/
event contracts, graphics/input/media bindings, and the native Cobalt
HTML/JavaScript/DOM/input/HTTPS endurance gate. The runtime descriptor design
now covers eventfd, pipe/pipe2, poll, epoll and FIONREAD and is the required
basis for future socket lifetime and cancellation behavior.
