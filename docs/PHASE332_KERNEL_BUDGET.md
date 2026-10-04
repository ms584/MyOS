# Phase33.2: kernel budget for actual provider/core integration

2026-10-05 (+07), baseline a2cf871. Phase33.1/33.2 remain OPEN.
No Phase33.3 or push. Imports163/242 (67.4% fixture subset); Cobalt0/5.

## Build change, not a loader-completion claim

The previous PROCESS_TESTS=1 kernel was262072 bytes against262144 cap (72 free).
Function/data sections alone removed80 text bytes without reducing image size:
the original page-aligned data boundary absorbed the reduction. Kernel LTO alone
produced257984 test-image bytes. Kernel plus vendored Mbed TLS LTO produced249792
test-image bytes (12352 free), preserving compiler ABI flags, the linker script,
the256KiB loader cap and low-memory/BSS assertion. User platform runtime and
renderer libraries are not rebuilt with LTO by this change.

GCC drives the kernel link with its matching LTO plugin; -nostdlib/-static/-no-pie
exclude host startup, libc and a host dynamic loader. Existing explicit libgcc
and the freestanding Mbed TLS archive remain the only linked libraries.
No security guard, tested service, boot cap or memory reservation was removed.

Initial full regression43137 failed21:33:44 at desktop test symbol lookup:
GCC renamed x_position/y_position to .lto_priv.0, not a native input failure.
tests/test_system.py now retains every exact nm symbol and supplies a base alias
only for one unique numeric .lto_priv suffix; ambiguous private objects get no
alias. Exact original names take precedence. test_system_symbols.py verifies
unique/exact/ambiguous/malformed/undefined cases. Actual guest-memory/input/pixel
oracles remain intact, not replaced with synthetic values or skipped assertions.

Final full regression32406 terminal exit0 at2026-10-04 22:07:28; all selected
tests PASS and normal BIOS image restored. Highest test image249792, normal
245692/runtime29904. Normal BSS end0x7b648 remains below0x80000, ELF entry0x10000,
two RX/RW LOADs, no dynamic section or unresolved symbols. UEFI rebuilt onOct5.
Pinned core44797 exit0 at02:21:32: actual hash/import/staging/RELRO/79-missing
fail-closed and64MiB OOM/recovery PASS. Not core execution.
Provider9564 exit0 at02:23:05: BIOS64fvgwxn7j/UEFI128wfuu_zql stages plus
UEFI64OOM1awugoc5/BIOS16OOM4fan64t5 PASS; each repeated/exact page return and
entry-zero execution rejected. Not provider constructors/getters execution.
Native81712 exit0 at02:26:20: BIOS native-2czt0ygc/UEFI native-s5wv1hcj PASS,
each two launches;
actual GLSL/VBO/texture/glyph/present/input, pinned EGL44/GLES246 dispatch,
EGL1.5/sRGB/fence/image/lifetime tests and exact resources, not Cobalt.
Logs: build/renderer-evaluation/kernel-lto-*.log. All jobs terminal; no live owner.

Normal hashes measured02:21:35 and unchanged after native tests02:26:29:
- kernel.bin cc0ef58168a8b16b3c53db22382c63f44b41f12783b046340d5407a3b3a003cc
- platform-runtime.bin ad9dc5c818b4dd933f9689a9e9a415396cee6852f36aa906b2d8ad97aa82c924
- myos-current.img a0eaba4728f24314f1f60a846a6b6bac277a96ac031d0373ddaad23047e84311
- myos-uefi.img 6cfbcea47ce325ee0d8ce46e9e31a317d38b24a0322fad17d0b7582af61ba45e

## Measured layout for the next loader slice (not implemented)

Pinned libcobalt.so LOAD end0x632faa0; no PT_TLS in the pinned core.
Actual provider-njs6506e LOAD end0xed21b8, TLS initialized8/total80/alignment8.
Putting provider at96MiB would overlap core. Choose checked page-rounded ownership
after the actual core LOAD end, with an unmapped separation page; verify overflow,
per-module mappings, shared runtime/stack reservations and total ELF_SPAN bounds.
These measured spans fit the existing128MiB image namespace in principle, but no
coexistence/rollback/permissions/relocation execution proof is claimed yet.

Pinned core uses emulated TLS; provider is the only native PT_TLS image in this
pair. A bounded two-image loader need not pretend to support arbitrary DTVs or
multiple native TLS images. Any such bounded policy must reject additional TLS
images explicitly, retain pristine per-worker snapshots/guards and test actual
provider module-ID/offset relocations and core emutls isolation under preemption.
Still required: real provider export lookup, SABI before any constructors,
dependency init/fini order, cross-image C++/exit ownership, fault/cancel/low-memory
cleanup and repeat launch; then remaining imports and actual Cobalt acceptance.
