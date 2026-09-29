# Phase 33.1 POSIX VM and errno continuation

Status: implemented and tested as a prerequisite; Phase 33.1 and 33.2 remain open.

The fixed-address Ring 3 platform runtime now exports mmap, mprotect, munmap,
mincore, madvise, msync, and __errno_location. Anonymous private/shared
mappings, address hints, MAP_FIXED, MAP_FIXED_NOREPLACE, PROT_NONE,
read/write/execute transitions and overflow checks use real MyOS page tables.
Write+execute remains rejected. File-backed mappings remain part of the later
filesystem binding.

mincore validates the complete result vector in the kernel before writing it.
madvise(DONTNEED/FREE) zeroes the physical pages; other accepted advice is
advisory. msync validates every anonymous page and has no backing store to
flush. Unsupported flags and unmapped ranges fail with POSIX-style errno.

Every ELF using platform bindings receives separate writable errno pages for
its main task and eight worker slots. __errno_location chooses the page for
the currently scheduled task. A Ring 3 fixture proves that main and worker
errno values and addresses are isolated.

Validation:

- bash build.sh
- python3 tests/test_elf_dynamic.py
- python3 tests/test_elf_dynamic.py --cobalt build/engine-evaluation/libcobalt.so

The pinned Cobalt image now resolves 14 allocator/VM/errno imports and stops
before execution on 228 missing strong imports. Both normal staging and the
64 MiB out-of-memory path reclaim all resources. Native Cobalt
HTML/JavaScript/input is still not running.
