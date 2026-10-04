# Phase 33.2: explicit two-image execution and exit ownership

2026-10-05, baseline3c568c9. Dependency work, NOT Cobalt acceptance.

## Contract

- `elf pair-link CORE PROVIDER` remains staging only. `elf pair-live` executes
  a verified ET_DYN core entry; `elf pair-starboard CORE PROVIDER [URL]` uses
  the existing pinned SABI validator/Starboard event launcher.
- The core and provider retain separate load biases, LOAD permissions,
  relocation/RELRO validation and lifecycle lists. Only the provider's two
  verified strong file-backed RX getter exports resolve core renderer imports.
  No host loader, generic dependency search, import-success stubs or cap changes.
- Execution requires provider `MyOSPairedExitAbi`: unambiguous strong defined
  8-byte OBJECT, file-backed nonwritable storage, value1. A module exporting
  SDK `exit` must advertise the same contract. Old providers remain stageable,
  but must not interpret the incompatible paired state as a single-image frame.
- SABI preflight precedes every constructor. Provider initializes before core;
  core finalizes before provider. Per image: optional exported `__cxa_finalize(0)`,
  reverse FINI_ARRAY, then DT_FINI. Addresses are validated in the owning image.
- Lifecycle state is a separate process-owned RW/NX page at0x8010011000,
  not the main thread's suspended stack. The main private runtime page publishes
  it only after SABI success. Both paired SDK exit and normal bootstrap return
  invoke shared `MyOSFinalizeImages` on the caller's own stack.
- Cleanup acquires one atomic per-process owner identified by its private TLS
  page. Other exit callers wait; recursive same-owner calls resume consumed
  state. Each callback is consumed before invocation. Partial initialization
  activates only images whose initialization began. No core callback if provider
  constructor exits before core initialization.
- `_Exit`, user faults and forced Escape cancellation do not run user callbacks;
  kernel stops/reaps every task and releases owned images/stacks/TLS/VM state.
  Callback code itself is application-controlled and may fault/block; Escape
  remains an explicit cancellation route. This is not a generic glibc DSO loader.
- At most one PT_TLS image remains supported. The genuine provider owns module1;
  the sealed core uses emulated TLS. Do not claim arbitrary multi-module DTV,
  hidden Cobalt lifetime/env internals or production RAM/thread requirements.

## Reproducible artifacts and tests

`tools/build_renderer_provider.py --paired-exit` preserves the verified genuine
native archive/object closure, replacing only the owned SDK exit object compiled
freestanding/PIC/no red zone. Source/object hashes and command are recorded.
Package: build/renderer-provider-evaluation/provider-84sokh6g/report.json.
The package report deliberately does not assert provider execution or Cobalt pass.

`tests/test_elf_pair_execution.py --provider
build/renderer-provider-evaluation/provider-84sokh6g [--uefi]` builds small cores
against the pinned EGL/GLES declarations. The real provider supplies actual
EGL1.5/context/pbuffer/GLES clear/readback in a pthread worker (exact cyan RGBA),
then destroys/releases/terminates, joins and reuses the TLS slot in another worker
before relaunch. A separately labeled synthetic getter provider exercises lifecycle
protocol with the real SDK callback registry per image; it is not renderer evidence.
Main return, worker exit, recursive exit, provider-constructor exit, raw exit,
SABI mismatch before init, old provider rejection, actual Cobalt77-missing
rejection and Escape cancellation all assert output order/once-only callbacks
and exact kernel heap/page reclamation, followed by Ring3 hello shell recovery.
Eight concurrent exit callers, faults in entry/finalizers, cancellation of a looping
finalizer and execution of the lifecycle RW/NX page (present/user/instruction page
fault) are exercised twice. Low-RAM cases also inflate a bounded genuine provider
LOAD for observed second-image OOM before constructors, then recover the shell.

Basic BIOS/UEFI e4bhyt_s/1z3fzfat and single-image exit frn6lu6t PASS.
Full60258 terminal exit0 04:35:21: ALL architectural regression PASS/normal restore.
Adversarial96920 terminal exit0 04:44:44: BIOS256 kphqch0e/UEFI256 h6imtmtj/
BIOS64 kadxl3lg/UEFI128 q1tg2fw5 PASS (low-RAM OOM, not a Cobalt RAM minimum).
Positive paired SABI57910 terminal exit0 04:50:48: BIOS xhenzlsy/UEFI vmbiwr04
PASS, using the exact descriptor embedded in pinned Cobalt and the full-suite
zero-entry Starboard fixture: SABI before init, readonly argv/URL, 2MiB guarded NX
stack/256KiB probe, real Start/Stop sequence, reverse finalizers and relaunch.
`tests/test_elf_pair_starboard.py` consumes the full-suite generated launch0.so;
the fixture is not the actual Cobalt engine or verified HTTPS (URL is argv only).
Logs: build/renderer-evaluation/pair-execution-{bios,uefi}.log,
pair-execution-adversarial-{bios,uefi,bios64,uefi128}.log, pair-starboard-{bios,uefi}.log,
pair-single-exit-regression.log, pair-start-full-regression.log.

Measured20570 exit0 04:49:15: PROCESS_TESTS1 kernel254240/cap262144 (7904 free),
normal250144/runtime30224/BSS end0x7c790<0x80000, RX/RW LOAD only. Restored normal
hashes match the scoped frozen images. All jobs terminal; no shared image owner.
SHA256:
- provider84sokh6g:55f0875c70997cafc085b0e6151d3a3a6bae88d3c5cc8c87fd50d67f6b326475
- kernel.bin:8f2f3002bb3aa527b4a2c38179ea24ada9542460ca2eea195e4d0036d764859f
- platform-runtime.bin:cba9e105273ed6b5d47b080e50782313dea04c0e76f617101961dfc279098f79
- myos-current.img:7cef6cb693a957d849f7a9f86b2e4da6ef916f6aedf5dc6e9297ad6c410da9e5
- myos-uefi.img:cb2cffbbac79dc0da62e4676f20d4b990f5f9b81cac6f31518a2b895c9807a04

## Limits and next work

These fixtures are not HTML/JS/DOM/input/HTTPS Cobalt acceptance. Coverage stays
163/242 fixture-tested imports67.4%, paired strong binding165/24268.2%;77 remain.
Actual Cobalt0/5, Phase33.1/33.2 OPEN, Phase32/33 NOT PASSED; no Phase33.3/push.
The additional paired adversaries above now have dedicated evidence; original
stage-only OOM results are still not execution proof. Live application workers
must coordinate their own use of objects during callbacks; this is not a generic
foreign-libc quiescing/unload system. Actual Cobalt DSO/env/emutls/worker/RAM needs
remain unmeasured. Next: remaining typed Starboard/system/network imports, required
media capabilities (not33.3 implementation), measured real engine start/stop,
then actual HTML/JS/DOM/input/verified HTTPS >=60s/relaunch.
