# Phase33.2 target renderer linker closure (2026-10-04)

Follow-up to2944b63. This checkpoint closes observed compiler/linker errors,
NOT native renderer/Cobalt acceptance. No Phase33.3 work and no push.

## Changes

- SDK errno constants/aliases exactly match sealed musl1.2.6 x86_64 generic ABI.
  tests/test_errno_sdk.py verifies archive SHA256 and exact reference bytes,
  every macro in C11/C++98/17 and actual generated GCC error_constants.h.
  Defining error numbers does not implement all associated APIs.
- Added the twelve genuine sealed musl math functions required by linker:
  copysign/copysignf,llrint,llround,log,lrint/lrintf,lroundf,rintf,
  round/roundf,truncf. MPFR is test-data generation only, not a target dependency.
- Container builder integrates existing real static-initialization guard and
  freestanding x86_64 __popcountdi2 software helper; no host libgcc or fake result.
- Native fixture checks 256 popcount cases (zero/all bits/every single bit/
  deterministic random inputs) against independent volatile bit-loop oracle,
  actual compiler-emitted guard initialization and per-worker errno isolation.
  nm confirms __cxa_guard_acquire/release and __popcountdi2 references are real.
- Shared compiler profile no longer forces executable entry main. Boundary
  tests require entry0/no undefined symbols for C11/C++98/17 shared libraries.
- tools/refresh_renderer_provider.py reuses existing compiled Mesa objects,
  refreshes sealed providers, then replays only Ninja's real final link command.
  Appended archive/wrapper are not Ninja dependencies: no-op Ninja alone was
  stale evidence. This avoids recompiling hundreds of unaffected units.

## Evidence (+07)

- Errno/native containers74750 exit0 13:44:44: BIOS sdk-l0rdleah,
  UEFI sdk-4wro59os; all positive/fatal/recovery launches and exact resources.
  Logs build/cxx-container-evaluation/errno-native[ -uefi].log.
- Math72599 exit0 13:43:03: BIOS target-x78nqckn/UEFI target-8hlldvwk,
  2357 independent MPFR cases, signed zero/NaN/infinity, four rounding modes,
  integer rounding beyond32bit, thread FP preservation and resources/relaunch.
  Logs build/math-evaluation/renderer-closure-native[ -uefi].log.
- Helpers3601 exit0 13:48:04: BIOS sdk-es1x1mt3/UEFI sdk-tqj7sdq7,
  unchanged positive container evidence reused; all three fatal paths followed
  by new guard/popcount/errno/eight-worker recovery and exact resources.
  Logs build/cxx-container-evaluation/closure-native[ -uefi].log.
- Expanded compiler boundary PASS13:48:01. Final incremental link exit0
  13:49:17: target-config-snob6k4x, libOSMesa.so.8.0.0=16298592 bytes,
  no strong undefined symbols or DT_NEEDED. Original compile report stays
  historical; use incremental-report.json and closure-link.log for this result.

## Still not native

The library embeds libmyos-runtime-probe.a for configure/link checks. Do NOT
execute that archive's fixed runtime/TLS assumptions in native ELF fixtures.
It has PT_TLS, R_X86_64_DTPMOD64 relocations and weak unresolved
_ZTH23_mesa_glapi_tls_Context. Real thread-local context/initializer semantics
need implementation or Mesa's real non-TLS thread-specific path, not weak-zero
substitution. Then use genuine native providers and test context/pixels/textures/
destruction/relaunch/resources before wiring EGL/GLES and SABI/Cobalt.

Full93613 remains valid: no kernel/runtime architecture changes; all four
normal hashes unchanged13:50:03 (PHASE332_OS_INFO.md). Full libstdc++ still
incomplete; latest whole make preceded errno fix and was not rerun here.
Imports163/242=67.4% subset coverage only. Actual Cobalt0/5, Phase33.1/33.2 OPEN.
Required gate remains HTML/JS/DOM, input, verified HTTPS/concurrent assets,
>=60s interactive endurance and close/relaunch. No native render or Cobalt pass.
