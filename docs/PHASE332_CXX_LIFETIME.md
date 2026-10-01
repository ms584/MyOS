# Phase 33.2 target no-exception C++ lifetime SDK

2026-10-01; follows 26c97aa. Kernel/runtime remain fully tested 45e08d7.
Phase 33.1/33.2 remain OPEN. This is not renderer-linked, full libstdc++,
exception/unwind support or actual Cobalt acceptance. No Phase 33.3 work.

## Implemented target profile

user/platform-cxx-allocation.cpp / user/sdk/new provide scalar/array, sized,
aligned and nothrow allocation/deallocation ABI forms using real MyOS
malloc/free/posix_memalign. Zero-size new allocates one byte; default alignment
is 16, over-aligned requests use the native aligned allocator. Invalid explicit
alignment is rejected, not silently accepted. Nothrow failure returns null.
Ordinary allocation failure terminates as an owned vector-6 process fault.

This provider requires -fno-exceptions (compile-time enforced), matching the
measured renderer's cpp_eh=none configuration. Fatal std::__throw_bad_alloc,
__throw_bad_array_new_length, __throw_length_error and __throw_system_error
helpers never return fake success. The profile has no new-handler API, exception
objects, unwinding, RTTI/vtables or std::bad_alloc catch support. It is not a
complete standard <new> library and must not be used for an exception-enabled
module. Compiler-emitted ABI names, not host libstdc++ types, bind
the native compiler-generated calls. It does not implement SIGABRT delivery.

user/platform-cxx-finalize.c implements process-local __cxa_atexit, atexit and
__cxa_finalize with real dynamically allocated records and a pthread mutex.
Registration failure returns nonzero. Finalize filters by DSO (null selects
all), removes records in reverse registration order before calling user code,
and invokes each at most once. Callbacks may register more callbacks or finalize
recursively without holding the registry lock. Concurrent callbacks can overlap;
completion order is not serialized. Sequential selection/invocation is LIFO.
This does not promise process shutdown in the presence of unjoined active
workers or implement __cxa_thread_atexit/TLS destructors.

One owned ELF image supplies a hidden unique __dso_handle. A priority-101
FINI_ARRAY hook drains pending callbacks on cooperative loader return, including
real compiler-registered global objects. This is not a multi-DSO dynamic loader.
Integration must avoid conflicting CRT __dso_handle definitions, and must arrange
any teardown observers after SDK shutdown. Fault, Escape cancel, SYS_EXIT/_Exit
and constructor-time failure skip user finalizers; kernel reclamation still owns
all process mappings/resources. No cancellation cleanup handlers are fabricated.

## Native evidence

tests/test_cxx_lifetime.py builds only against MyOS SDK/compiler intrinsic
headers (-nostdinc/-nostdinc++, no Linux platform macros, no exceptions/RTTI).
Fixture links no host DT_NEEDED; normal boot images are private verified copies.
The real constructor creates a global owning object; its compiler-generated
__cxa_atexit registration and destructor free are verified after SDK shutdown.
The fixture's equal-priority FINI_ARRAY observer is linked before the provider,
so reverse array traversal runs shutdown then observer. This order is explicit,
not inferred from entry returning success.

Four workers exercise new/delete/new[]/delete[], compiler-sized and 4096-byte
aligned calls and exact constructor/destructor counts. Zero-size and huge/invalid
nothrow requests, DSO filters, repeated finalize, recursive finalize and callback
registration during finalize are checked. Two finalizer workers are forced to
overlap callbacks, checking the same registrations are never invoked twice.
atexit ordering and the global destructor are checked exactly once.

Resource-pressure variant fills the native allocator with 1 MiB then 4 KiB
requests until failure (memory or mapping budget, not claimed specifically PMM).
Both registration APIs return errors under exhaustion, all blocks are freed,
then the ordinary lifetime still succeeds. A separate exhausted ordinary-new
variant faults without running finalizers and recovers the shell/resources.
Four allocated sleeping workers are force-cancelled, then the app launches again.
Every completed/fault/cancel lifetime checks exact page/heap recovery.

Final expanded source evidence (+07): BIOS 58320 PASS 11:46:12;
UEFI 27537 PASS 11:47:44. Both pipelines exited 0 and are terminal.
Logs: build/system-tests/cxx-lifetime.log and cxx-lifetime-uefi.log.
Unchanged kernel/image full regression remains valid; scoped SDK tests cover
this change. No duplicate full build/run or Cobalt import-count increase.

Remaining: libc/C++ container/prime-rehash dependency closure and real target
renderer linking, native GLES/EGL/context/presentation, then actual Cobalt
HTML/JavaScript/DOM/input/verified HTTPS >=60s and close/relaunch. Never push.
