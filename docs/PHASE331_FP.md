# Phase 33.1 continuation: native floating-point context

This completes one runtime prerequisite, not Phase 33.1 or Phase 33.2 as a
native Cobalt integration. All 242 strong Cobalt imports remain unresolved.
No engine HTML/JavaScript/input smoke or YouTube playback is claimed.

## Implementation

The single-CPU scheduler checks x87/FXSR/SSE2 support, clears CR0.EM/TS,
enables MP/NE and CR4.OSFXSR/OSXMMEXCPT. Each task owns a 16-byte-aligned
512-byte image. Every actual task change eagerly saves/restores x87/MMX,
XMM0 through XMM15 and MXCSR with FXSAVE64/FXRSTOR64. All kernel C and
interrupt handlers must retain -mgeneral-regs-only: this is not a facility
for using floating-point in kernel/IRQ code. New and recycled slots receive
zero registers, an empty x87 stack and default masked-exception controls.
No lazy FPU ownership and no user pointers to saved contexts are used.

CR4.OSXSAVE stays disabled. AVX/AVX-512/XSAVE, TLS, user threads, signal
contexts and SMP are not supplied by this change. Future CPU capability
exports must honor these limits even if raw CPUID advertises AVX hardware.

Instruction contract: [Intel SDM](https://cdrdv2-public.intel.com/868137/325462-089-sdm-vol-1-2abcd-3abcd-4.pdf),
FXSAVE/FXRSTOR instruction descriptions and floating-point context management.

## Tests

The PROCESS_TESTS build runs two Ring 3 processes with different x87 values,
all 16 XMM values, x87 control words and MXCSR rounding modes. It requires
timer preemption, voluntary yields and continuing valid state. Three rounds
kill/reap and reuse the task slots, checking clean initial state and physical
page recovery. The former x87-unavailable fault is replaced by an AVX #UD
probe; a successful AVX instruction exits instead, causing the test to fail.

Run from the repository:

```sh
python3 tools/run_logged.py -- python3 tests/test_system.py --suite memory
python3 tools/run_logged.py -- python3 tests/test_system.py
```

Validation on 2026-09-28: the full `tests/test_system.py` passed, including
the strengthened timer/yield probes, RAM 16/64/256 MiB, dynamic ELF,
storage, GUI, media and VGA regressions. The normal framebuffer image was
restored (196816-byte kernel). A separate QEMU `-cpu max` boot also passed
the FP-isolation and unsupported-AVX fault probes. Logs:
`build/phase331-fp-regression.log`, `build/phase331-fp-max.log` and
`build/system-tests/fp-cpu-max.log`. The static Cobalt contract checker still
reports 242 unresolved global imports and one weak import; it is not engine
execution evidence.

## Remaining acceptance

Keep Phase 33.3 unopened. Phase 33.1 still needs actual process VM, user
threads/TLS/waits/cancellation, platform ABI implementations, a selected
native renderer and Cobalt HTML/JavaScript/input smoke. Phase 33.2 currently
has a tested restricted dynamic loader, not real engine binding/startup.
The per-import register must remain unimplemented until actual APIs and their
contract tests exist. Do not bind missing symbols to fake successful stubs.
