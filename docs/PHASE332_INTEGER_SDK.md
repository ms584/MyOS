# Phase 33.2 — target integer and descriptor headers

2026-10-01; checkpoint follows 11fd4cd. Renderer/Cobalt acceptance remains open.

limits.h derives integer bounds from compiler constants, including signed or
unsigned plain char. inttypes.h supplies 154 printing/scanning format macros
for the audited GCC x86_64 LP64 fixed/least/fast/max/pointer types. The compile
test passes every argument/pointer through compiler format attributes in C
and C++, with both plain-char signedness modes. These declarations do not
implement printf/scanf or intmax conversion functions.

sys/types.h declares only size/offset/mode/process/owner widths matching the
existing runtime. fcntl.h and unistd.h expose actual existing descriptor
providers and flags. No open(), unlink(), advisory locking, credential policy
or complete POSIX implementation is claimed. Existing storage/descriptor
bounds and unsupported behavior remain in PHASE331_FILES.md. The runtime
fcntl consumes long variadic operands; native tests explicitly pass long
for F_SETFD. Full int-operand source-level compatibility still needs audit.
EBADF is now available with its actual runtime error value (9).

## Evidence

- tests/test_integer_sdk.py PASS at 13:28:08 +07. Four C/C++ signed/unsigned
  char compilations, 154 format macros each, LP64 min/max checks.
  Artifacts: build/integer-evaluation/formats-4ii30cw1/.
- Native BIOS session 13727 exited 0 at 13:34:07 +07:
  tests/test_integer_native.py compiled real sealed Mesa rounding.h with the
  already sealed math SDK. Ties-even scalar/SSE conversions include negative
  ties and an LP64 result greater than INT32_MAX. Typed file calls test flags,
  shared dup offsets, positional reads, close/error, nonblocking pipe/EOF and
  three relaunches with exact page/heap recovery.
  Artifacts: build/math-evaluation/target-m0zdrla_/; integer-sdk.log.
  First compilation found missing EBADF; it was fixed and rerun, not counted
  PASS. Math's pre-existing visible __rem_pio2_large warning remains unchanged.
- Renderer session 67816 at 13:31:48: setup 0, compile 1,
  build/renderer-evaluation/target-config-xmrn0q_0/. Previous missing integer
  macros/long bounds have moved past compilation. Remaining observed failures:
  getgid/getegid/getenv, stdio/FILE, sys/stat.h and C11 static_assert.

SDK headers/fixtures only: no kernel/runtime changes, no new UEFI/full-suite
claim. Native tests use a hash-checked private normal BIOS snapshot; shared
images remain unchanged. No graphics/backend/Cobalt startup evidence and no
increase in pinned import coverage are claimed. Phases 33.1/33.2 OPEN;
no Phase 33.3 or push. Next: real libc/stdio/environment closure, C++ target
support, native EGL/GLES/presentation, then the combined Cobalt web gate.
