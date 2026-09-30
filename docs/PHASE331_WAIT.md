# Phase 33.1 descriptor and wait slice

Status: implemented and fixture-tested as a dependency slice. Phase 33.1 and
33.2 remain open until native Cobalt passes HTML/JavaScript/DOM/input/HTTPS
acceptance in MyOS.

## Lifetime model

The fixed Ring 3 platform runtime owns a per-process descriptor table. A table
entry carries a generation and points to a refcounted open object. Operations
acquire a temporary reference after validating the descriptor and generation;
`close` invalidates the entry first, marks the object closed, wakes blocked
operations and drops the table reference. The object is destroyed only after
all temporary and epoll-held references are released.

This separation prevents slot reuse from turning an old descriptor into access
to a new object. A blocking operation observes close as cancellation and
returns `EBADF`. Epoll registrations hold object references rather than raw
descriptor-table pointers and release them on delete or epoll destruction.
The same ownership rules are intended for sockets; socket syscalls are not yet
implemented.

## Implemented contracts

- `eventfd` with nonblocking and semaphore modes;
- `pipe` and `pipe2`, bounded buffering, EOF/HUP and close cancellation;
- `poll` readiness and bounded timeout;
- `epoll_create1`, add/delete/control and wait with caller data preserved;
- `ioctl(FIONREAD)` for supported runtime descriptors;
- common `read`, `write` and `close` entry points, forwarding existing MyOS
  regular-file descriptors while handling runtime-owned objects locally.

The focused fixture verifies eventfd modes, poll and epoll readiness, epoll
data and removal, pipe transfer/FIONREAD/EOF/HUP, timeout, a blocked reader
woken by close with `EBADF`, and repeated create/close slot recycling. The ELF
loader's process teardown checks that staged pages are fully reclaimed.

## Validation

- `python3 tests/test_elf_dynamic.py`
- `python3 tests/test_elf_dynamic.py --cobalt build/engine-evaluation/libcobalt.so`
- `python3 tests/test_system.py`

The pinned-Cobalt run resolves 75 tested strong imports and stops safely with
167 strong imports missing. That count is not proof that Cobalt can execute or
that the Phase 33.1/33.2 acceptance gate has passed.
