# Phase 33.2 sorting/PRNG/finalizer SDK

tools/build_sort_sdk.py uses the verified musl1.2.6 source from the math
builder (archive SHA d585fd3b613c66151fc3249e8ed44f77020cb5e6c1e635a616d3f9f82460512a).
It copies the license, builds real smoothsort/qsort/qsort_r and PRNG, and
checks the actual nm undefined closure. No foreign atomic/syscall/stdio state.

Source adaptations are explicit: replace only the pure ctz helper dependency
with an x64 intrinsic, preserving the fallback's zero->0 value; materialize
the weak qsort_r alias; declare the internal comparator entry; clear cycle's
unused scratch pointer before returning instead of retaining a dangling local
address (GCC15 diagnostic). Only the original algorithm's signed comparisons
remain visible nonfatal warnings. Own adapters still compile with Werror.

platform-random.c serializes the real musl generator using native pthread
mutexes; srand/rand share one guest-image state, not host or per-worker state.
It is predictable NON-CRYPTOGRAPHIC randomness, not security entropy.
The SDK header declares RAND_MAX, qsort/qsort_r, rand/srand and atexit.
Already-tested platform-cxx-finalize.c provides actual atexit registration
and owned ELF FINI101 drain. No new kernel export/binding claim.

mesa_sort_target.patch omits unused <thread> only on MyOS, retaining actual
Mesa thread_local comparator/context variables. The native fallback saves
and restores previous per-thread state so nested sorts cannot replace the
outer callback. This is not a shared-global substitution. Native ELF GD TLS,
the real smoothsort callback and eight workers execute in the fixture.

tests/test_sort_sdk.py / user/elf-sort-sdk.cpp prove:

- zero-length no-access, reversed lengths0..257, sentinel preservation;
- large1540-byte records across scratch chunks, payload/key association;
- GNU qsort_r context and real Mesa nested ascending/descending callbacks;
- eight concurrent workers, exact sequential random multiset, independent
  reference generator checks for seeds0/1/123/UINT32_MAX and fresh default;
- reverse atexit callbacks run once on normal return, three relaunches,
  exact heap/page reclamation.

BIOS63099 exit0 16:50:06 sdk-jc3x6zmf. UEFI3822 native PASS sdk-vxdjxmpo;
its following renderer probe correctly fails, not the preceding test.
C/C++ header regression PASS16:53:27 formats-st9llqkq.
Sorting assumes valid arrays/counts/comparators as required by the standard;
no invalid-size overflow error-return or cryptographic randomness claim.
Forced cancellation does not promise guest finalizer execution; existing
kernel group teardown remains responsible for resource reclamation.

Target3822 terminal compile1 16:52:51, target-config-le70fafh: configure/
boundary PASS; rand/atexit/u_qsort errors cleared. Remaining observed:
abort, sscanf/strtod, os_misc platform queries, os_socket ENOSYS constant,
pthread_rwlock ABI/provider, time declaration and C++ cstdint header.
No renderer selected/executed. Later errors may appear after these are fixed.

No normal kernel/runtime/image changes. Correct hashes checked16:53:27,
identical to PHASE331_UNLINK.md. That checkpoint's aggregate full regression
remains applicable; no duplicate architectural run is claimed. All sessions
terminal. Phase33.1/33.2 remain OPEN; Cobalt has not run. No Phase33.3 or push.
