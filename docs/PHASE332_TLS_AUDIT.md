# Pinned Cobalt weak TLS and emulated TLS audit

Static binary/source evidence only; no claim that native Cobalt/TLS execution
passes. Phase 33.1/33.2 remain OPEN. The weak row remains unimplemented in the
contract CSV because its engine runtime behavior is not yet exercised.

The exact SHA256-pinned x64 release has no PT_TLS or DT_NEEDED. It defines
`__emutls_get_address` inside the image at 0x60204f1 (422 bytes). Inspection shows
pthread_once, mutex lock/unlock, getspecific/setspecific and malloc/realloc used
to allocate aligned per-thread storage, copy initialization data or zero-fill.
This makes emulated TLS a real pthread/allocator/destructor integration task;
absence of PT_TLS does not mean absence of thread-local state.

The sole undefined WEAK symbol is
`_ZTHN2v88internal12trap_handler21g_thread_in_wasm_codeE`, with GLOB_DAT at
0x621cc10 and JUMP_SLOT at 0x621fc48 (PLT entry 0x60b8570). The loader already
zero-binds both weak relocations instead of installing a fake callable stub.
The pinned [V8 header](https://github.com/youtube/cobalt/blob/3befd332486952f6603ae3baf2a8713878d6e200/v8/src/trap-handler/trap-handler.h)
declares the thread-local int; its
[definition](https://github.com/youtube/cobalt/blob/3befd332486952f6603ae3baf2a8713878d6e200/v8/src/trap-handler/handler-shared.cc)
has no dynamic initializer expression. Binary call guards are stronger evidence
for nullable initialization than inferring behavior solely from the name.

`tools/check_cobalt_tls.py` verifies the binary hash, weak relocation offsets,
in-image emulated TLS and every disassembled direct call to that weak PLT slot.
All 160 direct calls have NULL/skip guards. One compound block uses the same
guard for two calls: compare at 0x22951eb, NULL branch to 0x2295219, calls at
0x22951f5 and 0x2295212. The checker validates the exact intervening pattern;
the whole function was also manually inspected. It rejects unexpected GOT
references or unrecognized guard patterns rather than claiming compatibility.
This is a pinned call-pattern audit, not a general CFG verifier or a proof of
arbitrary indirect calls, signal safety, Wasm trap handling or TLS destructors.

Adversarial `tests/test_cobalt_tls_audit.py` covers missing/wrong guards, unsafe
branches, wrong GOT, additional references and modified compound blocks. Unit
tests and the pinned checker passed on 2026-09-30 at 20:31 +07. Disassembly is
streamed and only the small summary is logged; do not save the full output to
activity.log or feed that log through the logger.

Next: run an upstream-compatible emulated-TLS fixture natively in MyOS with
zero/template initialization, alignment, concurrent first access, growth,
per-thread isolation, join/detach cleanup and destructor iterations. Then verify
the actual core's startup/thread shutdown with all essential imports and the
real launcher. PT_TLS/dependency support for a chosen renderer remains subject
to that binary's measured requirements, not inferred from this core.
