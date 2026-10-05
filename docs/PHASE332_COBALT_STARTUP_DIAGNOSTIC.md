# Phase 33.1/33.2 — actual Cobalt failure-only startup diagnostic

Updated 2026-10-05 16:37 +07. PRIVATE ONLY, no33.3/push/phase closure.
Tree /tmp/myos-startup-diagnostic-d8WnKd; genuine core/provider hashes checked.

28 unresolved functions bind to assembly call-abort probes that identify the
called name and terminate status99, NEVER returning success/handles/capability.
Diagnostic missing=0 is NOT implementation credit. Production remains28missing.
Host serves controlled HTML data only; native Cobalt executes inside MyOS.

## Evidence and next action

- Build83378 terminal0 16:22:01, kernel235840B, normal private PROCESS_TESTS0.
- Native47704 terminal0 16:24:08: SABI18 matched and engine began execution;
  USER FAULT vector13/error1a at bias+34a38d8. Zero host HTTP requests; all pages
  reclaimed, heap90624 before/after. No missing-function probe called.
- Disassembly: genuine core opens literal /dev/urandom via openat(-100,path,
  O_CLOEXEC). Failure (-1 except EINTR retry) triggers internal int3/ud2.
  MyOS already has real checked SYSTEM_RANDOM backed by supported RDRAND, but
  no entropy file descriptor. This is an existing contract gap, not missing28.
- Private runtime-urandom.c adds readonly exact /dev/urandom character device,
  shared descriptor/dup/close lifetime, <=256B checked short read via SYSTEM_RANDOM,
  character metadata, poll readiness and seek ESPIPE. No PRNG/time/fake bytes.
  Unsupported write/append/truncate reject. Generic /dev namespace not claimed.
- Private61917 terminal0 16:35:22 now passes open but faults at34a3895, the
  internal CHECK after read completion. Probe VM default CPU lacks entropy;
  existing system entropy fixtures use CPU=max for success, qemu64 for failure.
- CPU=max actual startup89011 LIVE16:36:52; poll SAME, do not restart on timeout.
  Output startup-diagnostic-dtbmkrny. No result/acceptance credited yet.

Need native adversarial entropy-device fixtures (success+no-entropy, dup/flags,
metadata, short reads, bad pointers, table exhaustion and cleanup), then proper
production promotion/regression if correct. Do not bypass core assertions or
patch engine to pretend success. Later resource/renderer blockers still unknown.

Recovery mirrors: platform-missing-abort.asm/.c, exports-startup-diagnostic.py,
build-startup-diagnostic.sh, build_startup_diagnostic.sh,
test_startup_diagnostic.py, run_startup_diagnostic.sh, runtime-urandom.c,
run_urandom_startup.sh. All repo commands via tools/run_logged.py.
Actual HTML/JS/DOM/input/verified HTTPS/endurance/relaunch acceptance remains0/5.
