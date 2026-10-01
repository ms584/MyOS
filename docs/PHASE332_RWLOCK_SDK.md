# Phase 33.2 rwlock/time/integer SDK boundary

Pinned Cobalt wrapper declares rwlock storage80 bytes aligned8 and an8-byte
byte-union attribute object (not vanilla musl's56-byte native rwlock).
Cached contracts passed tools/check_cobalt_contract.py --headers at17:06:03,
verifying all header hashes. This is static provenance, not API acceptance.
user/sdk/pthread.h now declares the existing runtime's five rwlock functions,
correct storage types and zero initializer. No kernel/runtime code changed.

Real sealed Mesa25.0.7 rwlock.c/h, compiled unmodified with HAVE_PTHREAD,
executes through those native imports in tests/test_rwlock_sdk.py. The C++
fixture checks storage canaries and direct pthread error numbers with unchanged
errno; two readers must simultaneously hold the lock (bounded condition wait),
a writer must remain blocked until both release, then eight workers verify
exclusive writes/shared reads across intentional preemption inside writes.
Forced Escape cancellation kills a blocked reader and its writer-owner/main
waiting in join; four normal relaunches recover exact pages/heap.

The SDK does NOT prove full rwlock API conformance. Existing runtime ignores
non-NULL attributes; only NULL/default process-local behavior is used/proven.
No pshared, timed/try locking, priority fairness, foreign unlock diagnostics
or attribute setters are advertised here. Resolve any real engine dependency
on these before completion; never infer full ABI from type size or imports.

platform-calendar.c adds time() by calling the real MyOS realtime clock,
never host time. Optional output is assigned only after clock success.
Native tests bracket returned seconds with real realtime samples and check
NULL output, adjacent guards and error/untouched-output behavior. Failure uses
a test-only linker clock wrapper returning EIO; normally it delegates to the
real provider. Production has no injection. Existing clock/calendar/Mesa wait
regression also executes; timezone/DST limitations are unchanged.

user/sdk/cstdint reexports all28 C integer types into std, with no host C++
headers/library. C++ type identity assertions and C pinned-storage assertions
compile and execute in the fixture. errno.h adds numeric ENOSYS38 so Mesa's
existing unsupported socket fallback compiles; this is not socket success.

BIOS26564 exit0 17:12:56 sdk-273zo3fx. Batch68696 UEFI PASS sdk-nl5epkux,
then tests/test_mesa_time.py PASS sdk-z58nkzag (following probe reached only
because both tests exited0). Probe exits1 17:15:36, target-config-du_wdizy:
configure/boundary PASS; rwlock/time/cstdint/ENOSYS errors cleared. Header
regression PASS17:23:05 formats-lkxzeds6. Normal hashes unchanged17:18:02.
Previous PHASE331_UNLINK.md aggregate full regression remains applicable;
no duplicate full/normal-image rebuild is claimed for these SDK-only changes.

## Whole dependency measurement

tools/probe_renderer_target.py --compile --keep-going now uses ninja -k0
instead of the default8-failure stop and records failed-unit/error counts.
80264 terminal compile1 17:26:10, target-config-epnbvant: configure/boundary
PASS,100 failed units/212 error lines, reached877/887 scheduled steps; failed
dependencies still prevent the remaining commands/link. Counts are NOT native
context/renderer/API completion. No host libraries or selected backend.

Grouped primary failures from that actual log:

- math M_PI29/M_LOG2E27 errors: investigate target feature visibility;
- C strings/allocation/formatting/conversion/scanning declarations/providers
  (strncmp12, alloca8, sprintf7, abs6, abort6, strcpy5, exit5, sscanf4, etc.);
- setjmp.h13 units; target C++ type_traits4 plus vector/mutex/cstring/cstdlib;
- os_misc real system queries, file/locking headers and missing direct includes.

Plan coherent native SDK/provider batches from the complete log rather than
rebuilding to reveal only the next8 errors. More errors may emerge after
blocked dependencies succeed. Do not replace real services with dummy success
or claim C++ standard library conformance from aliases/partial headers.
All sessions terminal; normal BIOS/UEFI preserved. Phase33.1/33.2 OPEN,
Cobalt not executed; no Phase33.3 or push.
