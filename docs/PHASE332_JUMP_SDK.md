# Phase 33.2 native register jumps and stack allocation

Tested checkpoint follows 0e24ba7, 2026-10-01 (+07). No push or Phase33.3.
Phase33.1/33.2 remain OPEN: native Cobalt acceptance has not been executed.

## Implementation and boundaries
- tools/build_jump_sdk.py builds unchanged x64 setjmp.s/longjmp.s from the
  hash-verified musl1.2.6 source used by build_math_sdk.py; retains COPYRIGHT.
- SDK setjmp.h uses the sealed LP64 200-byte/alignment8 jmp_buf layout,
  returns_twice/noreturn declarations and C++ linkage. No sigsetjmp/siglongjmp.
- Saves/restores RBX/RBP/R12-R15/RSP/RIP; longjmp zero returns1. No undefined
  host providers or DT_NEEDED libraries; generated objects get GNU-stack notes.
- alloca.h exposes real compiler stack allocation, included by GNU/BSD stdlib.
  This is not heap allocation or a fake function; caller bounds still matter.
- This proves the tested native SDK subset, NOT every pinned Cobalt jmp ABI,
  signal delivery, cross-thread jumps or C++ destructor/exception unwinding.

## Reproduction and evidence
Use tools/run_logged.py for commands in /home/xytron404/myos:

    python3 tests/test_jump_sdk.py
    python3 tests/test_jump_sdk.py --uefi
    python3 tools/probe_renderer_target.py --compile --keep-going

Check the test script's arguments before changing invocation/profile.
Final native session55677: BIOS exit0 18:48:48.062599, sdk-gyzh1wtn;
UEFI exit0 18:49:14.267661, sdk-5onlaiu3 (under build/format-evaluation/).
Both C11 and C++17 artifacts run three times per boot profile; each exits89
and prints native sealed jumps/registers/preemption/nested stacks/8workers PASS.
Canaries and the unused136-byte jmp_buf tail remain unchanged; errno retained.
Values include0/1/-1/7/-7/INT_MAX/INT_MIN and nested _setjmp/_longjmp.
The assembly fixture clobbers all six saved registers and really nanosleeps
before longjmp; eight workers each exercise eight variants with preemption.
Nested frames touch aligned variable-size stack blocks. Each launch asserts
exact live heap/page recovery; private boot copies verify unchanged hashes.
Earlier setup attempts failed on recursion annotation/missing nanosleep adapter;
these were corrected, not counted as passes. Existing platform-c11-time.c is
linked for the actual sleep adapter; no kernel/runtime change was required.

Whole renderer session87828 ended18:55:07.094895 with compile_exit1:
build/renderer-evaluation/target-config-u8ev3uvw/compile.log and report.json.
Configure/boundary checks PASS;33 failed units/47 error lines, previously53/80.
alloca/setjmp groups clear. Exit/abort, OS-info, file locks/stdio and C++
headers/dependencies remain; native_execution/native_backend_selected=false.
This probe is NOT native EGL/GLES/Starboard rendering or Cobalt acceptance.

## Next agent / safe checkpoint
Read AGENTS.md, PROJECT_STATE.md, HANDOFF.md and PHASE33_EXECUTION_PLAN.md.
Inspect latest Git; never push. Do not start33.3 or mark33.1/33.2 passed.
Start with remaining full compile log, not imports-count or success stubs.
Audit kernel/syscall.c SYS_EXIT, user/elf-start.asm and platform-runtime.c:
raw SYS_EXIT skips FINI by design; pthread_exit terminates a worker (main path
waits for workers), not general process exit. PHASE332_FINALIZERS.md records
normal return versus abrupt termination. Proper exit/abort lifecycle must be
designed/tested before exposing them; no implementation is claimed here.
Kernel/runtime and normal BIOS/UEFI hashes unchanged at18:50:07. Full coverage
remains prefix83161 + resumed suffix89247, not uninterrupted full-run PASS.
No jobs remain running or own shared images. Logs capped; never read
activity.log through run_logged.py. Cobalt verified acceptance0/5, imports160/242.
