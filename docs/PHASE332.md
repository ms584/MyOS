# Phase 33.2 — staged dynamic ELF loader

Scope: the next native loader milestone after the first Phase 33.1 slice.
This does **not** close Phase 33.1's engine/HTML/JavaScript gate, Phase 32, or
Phase 33's real online Rickroll gate. Cobalt is still **not executable** with
the current platform services. No YouTube playback is claimed.

## Implemented behavior

- Processes have a separate `PROCESS_LOADING` state. Their private mappings
  exist but the scheduler cannot run them before linking and validation.
- `elf link NAME` maps the image, reads PT_DYNAMIC, processes supported
  relocations, reports unresolved imports, protects RELRO, then releases the
  entire staged image. It never calls constructors or an entry point.
- `elf run NAME` also accepts a bounded, self-contained ET_DYN fixture whose
  entry is executable and all required symbols resolve within that image.
  The existing static ET_EXEC/MyOS syscall fixture path remains usable.
- Dynamic metadata comes from program headers and dynamic tables, not section
  headers. Both SysV and GNU hash metadata are supported for symbol bounds;
  sectionless dynamic fixtures work.
- Supported x86-64 relocations are RELATIVE (base + addend), 64 (symbol +
  addend), GLOB_DAT and JUMP_SLOT (symbol address). Binding is eager. Defined
  symbols resolve within the same image; undefined weak symbols resolve to
  zero. Null symbol index zero has its ELF zero-value semantics.
- Undefined strong imports are listed and block execution. There is no host
  resolver, fake function table, or successful no-op Starboard binding.
- Relocation writes must target writable loaded memory. Writes into text,
  dynamic metadata, relocation/symbol/string/hash tables, overflowed or
  unmapped destinations are rejected. Unsupported relocation kinds fail.
- GNU RELRO becomes read-only before execution. DT_INIT and the relocated
  INIT_ARRAY are validated and run by a small **Ring 3** trampoline, then the
  entry runs. Constructors never run as kernel calls. Fault/timeout uses the
  existing user-process containment and reclamation path.
- Parse, link, allocation and attachment failures release staged memory.
  The loader checks that the physical free-page count returns to its initial
  value after release/reap; tests also prove another app still runs afterward.

This is a **single-object subset**, not a general Linux dynamic linker or a
complete Evergreen loader. External shared dependencies (DT_NEEDED), symbol
versioning, TLS/IFUNC, lazy binding, preinit and finalizer lifecycle are not
implemented. Runnable images needing finalizers are rejected. No dlopen,
libc, pthread, socket, EGL/GLES or media implementation is supplied here.
Image bounds, fixed load base, page-separated LOAD segments and the existing
five-second execution timeout remain. Synchronous image staging is not yet
cancellable and can pause normal interaction while loading a large file.

Reference contracts:
[ELF dynamic linking](https://refspecs.linuxfoundation.org/elf/gabi4+/ch5.dynamic.html),
[pinned Cobalt relocation implementation](https://github.com/youtube/cobalt/blob/3befd332486952f6603ae3baf2a8713878d6e200/starboard/elf_loader/relocations.cc).
The implementation is intentionally narrower than the full ABI.

## Actual Cobalt observation

Using the exact library hash from [PHASE33_COBALT_AUDIT.json](PHASE33_COBALT_AUDIT.json),
the native QEMU test at **256 MiB** observed:

```text
ELF staged pages: 25449
ELF relocations: relative=117653 absolute=20 global=17 jump=626 skipped=267
ELF bindings: missing=242 weak-zero=1
ELF RELRO: protected
ELF staged image released; not executed
```

Relocation counters count records processed; 267 unresolved relocation writes
were skipped. The 242 missing names match the pinned global import inventory.
No constructor or Cobalt entry was called. Mapping/relocation now runs inside
MyOS, unlike the earlier metadata-only inspection. This remains a loader test.
At **64 MiB**, staging runs out of memory, releases its partial image, and the
shell plus the legacy `/bin/hello` app remain usable. These are test settings,
not a minimum/recommended RAM specification for a running browser.

Cobalt's ELF header has no standalone executable entry. The inspected release
exports `SbEventHandle`; its upstream
[loader_app](https://github.com/youtube/cobalt/blob/3befd332486952f6603ae3baf2a8713878d6e200/starboard/loader_app/loader_app.cc)
looks up this function and checks Evergreen SABI information. Future MyOS
integration must implement that real launcher/event contract and the required
platform services. Patching the ELF entry to an arbitrary function is not a
valid route to running Cobalt.

## Try the dynamic fixture

From `~/myos` in WSL:

```bash
python3 tools/run_logged.py -- bash build.sh
python3 tools/run_logged.py -- nasm -f elf64 user/elf-dynamic.asm -o build/elf-dynamic.o
python3 tools/run_logged.py -- nasm -f elf64 user/elf-dynamic-export.asm -o build/elf-dynamic-export.o
python3 tools/run_logged.py -- ld -shared --hash-style=both -z now -z relro \
  -e _start -init init_first -o build/elf-dynamic.so \
  build/elf-dynamic.o build/elf-dynamic-export.o
python3 tools/run_logged.py -- python3 tools/make_asset_disk.py \
  build/phase332-assets.img dynamic=build/elf-dynamic.so
qemu-system-x86_64 -m 64M \
  -drive format=raw,file=build/myos-current.img,if=floppy -boot order=a \
  -drive format=raw,file=build/phase332-assets.img,if=ide,index=0
```

The disk packer refuses to overwrite existing output; reuse that image or
choose a new filename. Inside MyOS:

```text
elf link dynamic
elf run dynamic
run /bin/hello
```

`link` must report release without any constructor message. `run` must print
the Ring 3 constructor message, the dynamic-fixture PASS and exit status 32.
The final command still exits with status 42.

To stage the real Cobalt artifact, use the separately prepared read-only
Cobalt asset volume described in [PHASE331.md](PHASE331.md), boot with 256 MiB,
and enter `elf link cobalt`. Missing imports and no execution are the expected
result. `elf run cobalt` remains unsupported.

## Acceptance and regression

```bash
python3 tools/run_logged.py -- python3 tests/test_elf_dynamic.py
python3 tools/run_logged.py -- python3 tests/test_elf_dynamic.py \
  --cobalt build/engine-evaluation/libcobalt.so
python3 tools/run_logged.py -- python3 tests/test_system.py
```

Focused checks passed (RELRO fault vector 14/error 7 and constructor invalid-opcode vector 6/error 0 were verified): all four relocation kinds, weak and null symbols,
SysV/GNU tables, sectionless loading, constructors, RELRO write fault,
missing symbols, bad destinations/symbol indices/metadata/constructors,
unsupported features, constructor fault/timeout, repeated cleanup and legacy
app reuse. The real engine staging and low-memory rollback checks passed.
Full system regression status: **passed on 2026-09-28; normal framebuffer image restored**. Its `apps` group includes
the new fixture suite; downloading Cobalt is not required for ordinary tests.
The optional engine test checks the pinned binary hash and asset immutability.

Logs: `build/phase332-focused.log`, `build/phase332-fixtures.log`,
`build/phase332-cobalt-final.log`, `build/phase332-regression.log`, and
`build/system-tests/elf-dynamic*.log`. The full suite restores the normal image.

## Remaining work

The 243-entry contract register is unchanged: no platform implementation was
added. This loader resolves in-image symbols and ELF weak defaults, including
the one weak import; the 242 strong platform imports still block Cobalt.
The weak initializer's actual engine call path has not been exercised. Next: a native Starboard launcher/SABI boundary,
process VM and thread/TLS/FP state, cancellation and waits, then networking,
graphics and compressed media. Add dependency/finalizer support when required
by the selected real runtime, with tests before enabling execution.

Phase 33.1 still requires native Cobalt HTML/JavaScript/input smoke. Actual
online Rickroll with sound for 30 seconds plus pause/resume/seek remains the
unchanged Phase 33 gate. The owner pushes manually.

## Runtime continuation — 2026-09-28

Per-task x87/MMX/SSE context support has been added; see
[implementation and test status](PHASE331_FP.md). This supersedes the earlier
FP/SIMD limitation only. The owner requires completion of 33.1 and 33.2
before starting 33.3. Cobalt startup remains unimplemented; neither parent
gate is being marked complete.
