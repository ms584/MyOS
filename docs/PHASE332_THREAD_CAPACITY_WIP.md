# Phase33.2 thread-capacity WIP

Updated2026-10-07 12:59 +07. Private, not main promotion/phase closure.
Actual Cobalt startup-zyc7sv42 fills8 worker slots, then six thread-create
EAGAIN(-11), followed by ImmediateCrash. Shared262144/4-byte mappings already
succeeded; unsupported-import probes are diagnostic only, not completed APIs.

## Source/configuration
- thread-limit.h shared kernel/runtime bound:16 workers +main =17 threads;
 4 processes =>68 user tasks. Scheduler bound includes4 kernel tasks as well.
- kSbMaxThreads/_SC_THREAD_THREADS_MAX17 and RLIMIT_NPROC68 match that model.
  Not a configurable rlimit/multi-user/scheduling-priority implementation.
- Scheduler FPU state72*512 bytes is16-byte aligned kernel heap allocation
  before fp_init/STI, not large low-memory BSS; original BIOS reservation kept.
- Native ELF TLS base moves to runtime base+0x30000; runtime TLS17 pages +guard
  cannot collide. Compile-time separation assertion. Native TLS slot guards
  remain unmapped; fixture computes actual runtime guard from configured bound.
- TSS key creation/deletion clears all17 runtime slots, not old hardcoded9.
- Native fixture holds all16 workers, checks seventeenth fails EAGAIN without
  changing output/errno, distinct PT_TLS/main data, deletes/reuses populated TSS
  key while workers wait, then join/reuse3 waves and relaunch4 times/profile.

## Evidence / failures preserved
- Static enlarged FPU BSS triggered linker reservation assertion; not bypassed.
- First header assembly lacked existing VM WIP declarations; corrected merge.
- First16-slot TLS overlapped old native TLS base; loader correctly refused.
  Moved native region, not fake successful allocation.
- Old gd3 guard address became real expanded TLS page. Fixture now targets
  actual guard; still requires write fault error6 and exact fault address.
  thread-capacity-tls-old-guard.log preserves old assertion failure.
- PTY96189 terminal0 at12:57:29. BIOS16/64 q743m2ks PASS saturation,
  independent PT_TLS/key clearing/reuse/relaunch/exact resources.
- Legacy native TLS GD/IE/TLSDESC/guards/NX/faults/cancel scope PASS, and main/
  worker teardown/timed mutex/join-detach races PASS observed12:57:01.
- UEFI64/25642_pfwgv PASS. Whole script terminal0; not actual Cobalt proof.

## Recovery / next
Apply on private file-map/filename/volume-audio/shared-memory snapshots:
process-capacity*.h ->include/process.h,thread-limit.h ->include,
scheduler-capacity*.c ->cpu/scheduler.c, runtime-statvfs-clean*.c ->runtime,
elf-platform-system-capacity*.c ->system fixture, elf-native-tls-capacity*.c
and test_native_tls_capacity*.py ->native TLS fixture/harness. These are clean
sources, not diagnostic traps. See script run_thread_capacity_20261007.sh.
Genuine Cobalt trace PTY80030 started12:58:47, LIVE; freeze cd-QJYMwG until
terminal. Then clean
full architectural regression for combined batch. Production still28 missing;
three real imports unpromoted; no push/no33.3/browser percentage claim.
