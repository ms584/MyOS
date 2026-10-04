# Phase33.2 genuine native OSMesa checkpoint (2026-10-04)

Follow-up to a38b85b. This is NOT EGL/GLES, Cobalt, or Phase33 acceptance.
No push, no33.3. Kernel/runtime architecture unchanged; full93613 remains valid.

## Implementation

- tests/test_native_renderer.py links genuine Mesa25.0.7 softpipe archives and
  native-tested pure SDK providers into a Ring3 ET_DYN asset. Installed MyOS
  runtime supplies imported allocator/thread/window services. The configure-only
  libmyos-runtime-probe.a is NEVER embedded; only sealed pure floor/ceil objects
  are extracted by the existing container builder.
- Native process-local environment/finalization/exit/jump providers and original
  OSMesa factory object close seven initial missing imports. Final native module
  has missing0/weak-zero1; this is not pixel or functional acceptance.
- Existing eager native ELF TLS supports the real Mesa32-byte template. The
  weak C++ _ZTH23_mesa_glapi_tls_Context initializer is optional for the C-defined
  zero-initialized TLS pointer, not a fake initializer implementation. Actual
  worker context/GetCurrentContext binds; main/teardown isolation still pending.
- SDK pthread_attr_t is pinned 80bytes/alignment8, with C11/C++98/17 guards and
  declarations for existing native attr APIs. Fixture uses512KiB worker stack,
  validates stack attribute and sentinel bytes. Main keeps its small stack.
- tools/mesa_memory_target.patch preserves16 cache entries/all texture API limits,
  changes MyOS-only softpipe tile32x32 to8x8 through original algorithm macros.
  Six stages x128 sampler views otherwise eagerly allocate roughly192MiB float
  caches; profile reduces this to roughly12MiB. More eviction is a performance
  tradeoff. Mandatory blitter shader pre-cache is NOT bypassed.
- MyOS PMM MAX_PAGES is256MiB: raising QEMU RAM512 does not raise managed RAM.
  No speculative allocator, kernel memory cap or resource-accounting changes.
- Test-only --diagnostic wrappers forward real calls; no locks/guards bypassed.

## Required functional checks (not all reached)

Create/bind context; clear/read exact blue pixels; compile GLSL120 shaders;
real VBO/texture drawing; exact four-color oracle; all1024 gradient/checker
pixels with64 source tiles exceeding16 cache entries; texture re-upload;
actual native framebuffer present; actual key input; context/window teardown;
exit90/exact resource baseline; repeated launch. Desktop GL2.1 smoke is not GLES.
Each VM uses a private frozen image/asset volume, normal image hashes preserved.

## Observed evidence (+07)

- Header compiler boundary PASS14:07:19, native-header-boundary.log.
- Target cache profile applies cleanly;20/20 affected softpipe units compile.
  build/renderer-evaluation/memory-profile-build.log. Old incremental-report.json
  is historical linker evidence, NOT this native test's outcome.
- Earlier120s runs FAILED before context bind. CPU/trace samples showed advancing
  allocation/once/lock work, not proof of deadlock. 512MiB VM did not fix this.
- native-wfnqmaee (16302720-byte module), 300s run20817: context bound observed
  14:46:19; terminal exit1 14:49:16, before pixel/shader/present proof. Samples
  include kernel vm_leaf; process_vm_map scans from VM_BASE for each malloc,
  and vm_forget scans an append-only page ledger. Allocation cost needs measuring,
  not speculative lock replacement. failure-registers.log is preserved there.
- Run86476 from14:53:24: native-bcahc3qq, BIOS256MiB, explicit600s bound;
  terminal exit1 15:03:34. Context and exact clear/read blue pixels pass;
  vertex compilation did not finish. Adds individual phase markers without
  weakening assertions. Samples at0x1478a and0x13abb resolve process_vm_map and
  vm_leaf; disassembly confirms first-fit search from VM_BASE. Not deadlock proof.
  Log build/renderer-evaluation/native-cold-600.log. UEFI/positive relaunch pending.
- Normal four hashes unchanged15:03:54; no live VM owner. PHASE332_OS_INFO.md baseline.

Reproduce via tools/run_logged.py:
python3 tests/test_native_renderer.py --renderer-probe
build/renderer-evaluation/target-config-snob6k4x --configuration
build/cxx-target-evaluation/configure-_1j4xqjo --launches 1 --startup-timeout 600
Use --uefi for UEFI; default2 launches for same-VM close/relaunch evidence.

Imports163/242=67.4% subset only; actual Cobalt0/5 verified, not phase percentage.
After real pixels/text/context/resource feasibility, wire real EGL/GLES/SABI,
then run Cobalt HTML/JS/DOM/input/verified HTTPS/concurrent assets >=60s and
close/relaunch. Keep Phase33.1/33.2 OPEN until every actual engine gate passes.
