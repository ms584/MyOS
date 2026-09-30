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

The runtime exports 75 tested Cobalt imports in total after the clock and
descriptor/wait slice. The pinned engine still stops before execution on 167
missing strong imports.

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
