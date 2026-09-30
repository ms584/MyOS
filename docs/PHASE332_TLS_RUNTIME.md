# Phase 33.2 TLS runtime fixtures

The weak V8 TLS initializer remains weak-zero bound by the loader. The pinned
Cobalt library's in-image `__emutls_get_address` implementation was inspected;
it initializes pthread key/mutex/once state, allocates aligned template-backed
objects, grows a per-thread pointer array and frees it through a key destructor.

`third_party/compiler-rt/emutls.c` is the unmodified LLVM `llvmorg-18.1.8`
implementation, SHA-256 `a4437f3e329a0835adbc7df38e36df12f358d05c073b11813c409f849c2345d2`.
Its Apache-2.0 WITH LLVM-exception license is included beside it. The fixture
uses pinned Cobalt SABI key/mutex/once declarations in `tests/emutls-abi` and
runs as a Ring 3 ELF in MyOS. It does not claim that MyOS builds or executes the
Cobalt library's own object code or reproduces the pinned Clang toolchain.

`tests/test_elf_dynamic.py --emutls-only` covers 48 TLS controls, pointer-array
growth, zero and template initialization, 64-byte and page alignment,
concurrent first access, per-thread values/errno, weak TLS zero binding,
joined and detached worker destructors, and unmapping of worker allocations.
The same fixture is part of the normal ELF regression.

The pinned `pthread_key_t` is `void *`. The runtime now uses pointer-typed
create/delete/get/set entry points, writes the complete 64-bit output and
validates all opaque key bits before indexing. The pthread fixture tests
canaries, high-bit invalid values, all 32 slots, exhaustion without overwriting
the caller's output, key reuse, thread isolation and the four-pass destructor
limit. These tests establish the exercised local ABI behavior; they do not
close Phase 33.1 or 33.2. Cobalt itself still has not run in MyOS.
