# Phase 33.1 platform runtime continuation

Status: **in progress; Phase 33.1 and Phase 33.2 are not closed.**

This slice introduces the first executable external platform bindings for staged
ET_DYN programs. MyOS maps a 544-byte read/execute runtime into every staged
Ring 3 ELF address space at address 0x803f000000. The runtime uses the existing
anonymous VM syscalls; it is not a kernel-address trampoline and it does not
grant user access to supervisor pages.

## Implemented allocator contract

The eager linker now resolves these seven Cobalt imports only:

- malloc, calloc, realloc, free
- aligned_alloc, posix_memalign, malloc_usable_size

Each live allocation owns an anonymous, zero-filled page range. free unmaps
that range. Checked size arithmetic rejects overflow. Alignment and
posix_memalign error behavior are covered by a Ring 3 dynamic ELF fixture.
realloc preserves the common prefix and keeps the old allocation on failure.

The external resolver remains allowlisted. Missing strong symbols still abort
startup before constructors or entry, and weak unresolved symbols remain zero.
The pinned Cobalt 27 LTS image now resolves these seven imports and stops at
235 missing strong imports (previously 242), then releases all staged pages.

The dynamic loader also accepts an ELF containing only PLT relocations without
requiring the unused DT_RELAENT tag. Existing malformed-relocation,
constructor, RELRO, timeout and rollback checks remain active.

## Validation

- bash build.sh
- python3 tests/test_elf_dynamic.py
- python3 tests/test_elf_dynamic.py --cobalt build/engine-evaluation/libcobalt.so
- focused Snake persistence retry after an earlier nondeterministic timeout

The allocator fixture checks zero-fill, overflow rejection, 16/64/128-byte
alignment, usable size, data preservation across realloc, invalid alignment,
release, repeated execution and return to the shell.

## Still required before closing 33.1/33.2

POSIX VM and errno/TLS, pthread synchronization and lifecycle, waitable file
descriptors, the remaining external symbols, Starboard launcher/SABI/event
loop, filesystem/storage/clock/network/HTTPS bindings, window/EGL/GLES/input,
and the native HTML/JavaScript/input endurance gate remain open. Cobalt is not
executed yet. Phase 33.3 and YouTube playback have not started.
