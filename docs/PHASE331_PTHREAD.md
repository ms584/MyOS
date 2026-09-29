# Phase 33.1 pthread runtime slice

Status: implemented and tested as a dependency slice. Phase 33.1 and 33.2 remain
open until native Cobalt runs the HTML/JavaScript/input endurance gate.

MyOS now exports the 48 pthread/semaphore imports required by the pinned Cobalt
27 LTS image. The implementation runs in the fixed Ring 3 platform runtime and
uses MyOS thread, wait/wake, timer and VM syscalls.

Implemented contracts:

- joinable and detached creation, join, detach, bounded stack attributes and
  detached slot/stack reclamation;
- normal, recursive and error-check mutex behavior;
- condition signal/broadcast/wait and absolute timed wait;
- semaphores and absolute timed wait;
- read/write locks and once initialization;
- per-thread keys, values and four-pass destructor execution;
- self/name/attribute/scheduler inspection plus signal-mask compatibility.

The acceptance fixture runs multiple Ring 3 workers, serializes shared counter
updates, verifies return values and destructors, exercises condition and
semaphore wakeups, and creates more detached workers than the simultaneous
thread-slot limit to prove reuse. The fixture returns status 35 only after all
checks pass. Process teardown still verifies complete VM/page reclamation.

The runtime exports 62 tested Cobalt imports in total: 7 allocator, 7 VM/errno
and 48 pthread/semaphore symbols. The pinned engine now stops before execution
on 180 missing strong imports instead of 228.

Validation:

- python3 tests/test_elf_dynamic.py
- python3 tests/test_elf_dynamic.py --cobalt build/engine-evaluation/libcobalt.so
- full python3 tests/test_system.py was started after the focused pass and stopped at the owner's request after the Phase 26 recovery cases; rerun it before the next integration checkpoint

Still open: ELF TLS relocations and TLS destructors owned by the loader,
poll/epoll/eventfd/pipe, file/clock/network APIs, Starboard launcher/SABI/event
contracts, graphics/input/media bindings, and the native Cobalt endurance gate.
