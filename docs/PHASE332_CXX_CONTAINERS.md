# Phase 33.2 real target GCC containers / renderer integration

2026-10-04, follows a6e6e88. Phases OPEN, no33.3/push or Cobalt pass.

## Implementation and boundary

tools/build_cxx_containers_sdk.py builds GCC15.2.0's original prime-rehash
implementation and genuine generated vector/unordered_map headers with existing
MyOS no-exception new/delete. Libraries/licenses come from the sealed GCC
archive; no host libstdc++, handwritten STL or exception/unwind support.
Native imports use the installed runtime initialized by the loader. A configure
archive must NOT be embedded in a native ELF: its runtime has fixed private
TLS/data addresses. Only dependency-free sealed musl ceil/floor objects are
extracted from that archive, plus actual SDK string primitives.

Added fatal std::__throw_out_of_range and exact generated-header
std::__glibcxx_assert_fail ABI. Length/range violations trap; assertions report
the condition and call real abort/SIGABRT. None silently return success.
Upstream headers are system headers, preserving -Werror for target fixtures.
The math C header now follows upstream C++ headers so <cmath>'s include_next
finds it. Source preparation includes omitted GCC unwind-pe/demangler headers
and sources, not a host unwinder or a claim of working exceptions.

probe_renderer_target.py --cxx-configuration enables these actual headers and
container objects in the checked target provider, with a vector/hash link
boundary requiring no undefined symbols/DT_NEEDED. Its default stays unchanged.
Selected OSMesa/softpipe compile feasibility remains NOT a native EGL/GLES backend.

## Native evidence and diagnosis

Initial44678: native-twclqzam BIOS and native-bcnbg_b3 UEFI container checks PASS,
four successful launches plus length/range fault recovery per firmware. Genuine
vector growth/erase/copy/move/shrink request/destruction, 64-byte alignment,
513-key deliberately colliding hash insertion/duplicate/find/erase/copy/move/
swap/reserve/rehash and load bounds; eight workers x8 iterations/exact resources.
The chain later failed affected UEFI lifetime pressure, NOT these container tests.

Diagnosis: 4096-byte request plus allocation metadata uses two pages. Failure
can leave one page available for smaller destructor registration. The fixture
now exhausts minimum one-byte requests too; no runtime capability fabricated or
assertion removed. Corrected lifetime56071 exit0 12:53:32 BIOS+UEFI, allocation/
aligned/sized/nothrow/OOM/registration/finalizers/cancel/relaunch/resources PASS.

81649's expanded BIOS checks gave three positive runs and correct length/range
faults, then the expectation rejected assertion SIGABRT because it incorrectly
expected UD2 vector6. Actual output proves GCC precondition message + signal6.
Fixed expectation distinguishes real abort from traps. Final62162 exit0
13:00:28: BIOS sdk-26wo1sms/UEFI sdk-t2cyyrmm, all three fatal paths/exact
resources followed by actual eight-worker container recovery. The final fixture
uses the SAME SDK archive builder as renderer integration, not duplicate providers.
Renderer20532 terminal configure0/compile1 13:00:03, target-config-3c959yqk:
5 failed units/13 errors (previous11 units/11 errors). Native backend still absent.
Failures: ASTC LUT C++ units need ctime; cache DB sys/file.h; ddebug mkdir;
HUD _IOLBF. Actual vector/hash/math compilation proceeded; no full link/pass.

Reproduce tests/test_cxx_containers.py --configuration
build/cxx-target-evaluation/configure-_1j4xqjo --renderer-probe
build/renderer-evaluation/target-config-yi0713xx [--uefi]. --faults-only reuses
unchanged positive evidence but tests all three fatal paths plus recovery.
GCC configure7633 exit0 12:36:36, _1j4xqjo; whole make39341 exit2 12:37:05:
math/support-source groups clear, stdio/ctime/locale/errno still incomplete.
Full93613 remains valid: kernel/runtime architecture unchanged; normal four
hashes unchanged12:59:08, PHASE332_OS_INFO.md. All jobs terminal, no image owner.
Git fetch12:59 confirms origin/main3699a87 is an ancestor of local HEAD; no push.
Imports163/242 coverage67.4%,
actual Cobalt0/5. Next: direct renderer failures, real context/present and
Cobalt HTML/JS/DOM/input/verified HTTPS >=60s with close/relaunch acceptance.
