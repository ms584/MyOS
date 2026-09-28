# Phase 33.1 continuation: user VM, threads, TLS and waits

Status: the native runtime primitives in this document are implemented and
tested. They are prerequisites for Cobalt, not a completed Starboard/POSIX
layer. The pinned Cobalt image is still not started and its 242 strong imports
remain unresolved. Phase 33.1, Phase 33.2 and the online playback gate remain
open; Phase 33.3 has not started.

## Implemented contracts

- Anonymous user mappings occupy the fixed half-open range
  `0x8020000000..0x803f000000` (496 MiB of virtual address space). Mapping is
  page-aligned and zero-filled. The kernel supports unmap and read/write/
  execute permission changes, rejects write+execute, validates overflow and
  never accepts kernel addresses. Physical pages and page-table pages remain
  owned by the process and are reclaimed on exit, fault, kill or failed load.
- A process can create up to eight additional Ring 3 threads. Each thread has
  its own interrupt/kernel stack, register frame, eager x87/SSE context and
  x86-64 FS base. Threads share the process CR3 and user mappings. Creation
  validates executable entry, writable user stack and readable TLS base.
- Thread exit is separate from process exit. A non-current zombie thread can
  be reaped for its signed status; its scheduler slot and kernel stack are
  reused. Killing or faulting the process stops all its threads, and process
  reap detaches every task before freeing the shared address space.
- Wait compares one aligned 64-bit user word while interrupts are disabled,
  then blocks the calling task atomically. Wake targets matching waiters in
  the same process only. Absolute PIT-tick deadlines return timeout; a changed
  value returns retry instead of sleeping. This is the kernel primitive for a
  later pthread mutex/condition/semaphore and eventfd implementation.

The exposed interface is currently the MyOS INT 0x80 ABI (`SYS_VM_*`,
`SYS_THREAD_*`, `SYS_WAIT`, `SYS_WAKE`). It is not yet the musl ABI expected by
Cobalt. File-backed mmap, shared mappings, fork, signals, cancellation cleanup,
per-thread errno, pthread opaque objects and ELF PT_TLS are still missing. AVX
remains disabled because its state is not preserved.

## Verification

The embedded Ring 3 fixture checks zero fill, mapping and permission changes,
read-only page faults, an RW-to-RX executable transition, thread creation,
FS-base isolation, timer/yield switching, atomic wake, timeout, signed status,
reap and complete page recovery. It remains below the legacy one-page fixture
limit. The system suite requires the VM and thread PASS markers on each normal
boot and repeats boot/memory checks at 16, 64 and 256 MiB.

```sh
cd ~/myos
python3 tools/run_logged.py -- python3 tests/test_system.py --suite memory
python3 tools/run_logged.py -- python3 tests/test_system.py
```

Focused validation passed on 2026-09-28 and restored the normal framebuffer
image. Full-system regression status is pending for this continuation.

## Remaining gate before Phase 33.3

Build the real user ABI on these primitives: allocator and file-backed VM,
pthread/TLS/errno, poll/epoll/eventfd and networking waits. Then implement the
pinned Starboard contracts, external-symbol binding, launcher/event lifecycle,
graphics and input. Phase 33.1 passes only when the pinned Cobalt engine renders
a local HTML page, executes JavaScript and receives input inside MyOS/QEMU.
Phase 33.2 needs that real runtime binding/startup rather than only a
self-contained dynamic fixture. Do not resolve missing imports to successful
no-op stubs.
