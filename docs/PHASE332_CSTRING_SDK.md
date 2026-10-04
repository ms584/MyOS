# Phase 33.2 native Cstring / C++98 dependency

2026-10-04, follows0f3f768. Not complete C++/renderer/Cobalt acceptance.

## Implementation

tools/build_cstring_sdk.py verifies the existing musl1.2.6 archive SHA256
d585fd3b613c66151fc3249e8ed44f77020cb5e6c1e635a616d3f9f82460512a.
It preserves COPYRIGHT and extracts original pure strcoll/strxfrm bodies,
removing only unused locale_t/current-locale/_l alias plumbing. The fixed
bytewise profile agrees with strcmp; strxfrm reports the full source length
and copies the terminating NUL when capacity permits. No locale switching or
language-sensitive sorting is claimed. The archive includes existing target
string primitives, not host libc; it is consumer-linked, not new runtime exports.

SDK pthread/time ABI checks and noreturn syntax now support C++98 as well as
C++11+, with unchanged object layouts. Target stdint/compiler boundary expose
integer constants/limits for older C++ too. Actual target gthread code, not
handwritten STL replacements, runs with --std c++98 and c++17.

## Evidence (+07)

- Gthread99435 exit0 12:27:36: --std c++98 BIOS native-rqhhjn2p and UEFI
  native-_tcq_rfk, eight launches/two cancellations, exclusion/publication/once/
  key destructors/recursive mutex/realtime timed wait/exact resource recovery.
- Cstring59789 exit0 12:30:55: BIOS sdk-jid918r6 and UEFI sdk-7au8141x,
  twelve launches total: C11 and actual GCC <cstring> C++98/17. Ten ordered
  byte strings, all pair comparisons, transformed-key ordering, NULL/zero-size
  query, exact/insufficient capacity, errno, canaries, 32 source alignments and
  unmapped-page source/destination edges, eight workers x20, exact resources.
  Insufficient-capacity result bytes are unspecified and are not asserted.
- Integer154 macro/type/char-sign checks hosted+freestanding and expanded
  renderer boundary C11/C++98/C++17 PASS12:32:29: no undefined/DT_NEEDED,
  upstream <new> precedence and prohibited host library routes rejected.
- Whole C++ make still exit2 12:32:29, build-after-cstring.log under
  build/cxx-target-evaluation/configure-jf2fl8q3. Cstring declarations and
  C++98 pthread/time syntax errors cleared; stdio/ctime/locale/math and omitted
  support-source groups remain. No native standard container library yet.
- Kernel/runtime architecture unchanged: full93613 remains valid. All four
  normal image hashes match PHASE332_OS_INFO.md12:32:44; private fixture images
  did not mutate shared normal BIOS/UEFI. No live job/image owner.

Reproduce tests/test_cstring_sdk.py --configuration
build/cxx-target-evaluation/configure-jf2fl8q3 [--uefi], and test_gthread.py
with the same configuration plus --std c++98 [--uefi].
Next: real missing C++ dependencies, then native containers/Mesa EGL/GLES and
actual Cobalt HTML/JS/DOM/input/verified HTTPS endurance. Imports remain
163/242 (67.4% subset coverage), actual Cobalt0/5; phases OPEN, no33.3/push.
