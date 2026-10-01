# Phase 33.2 target math dependency checkpoint

2026-10-01; follows 6e4bd4a, unchanged kernel/runtime 45e08d7.
Phase 33.1/33.2 remain OPEN. Cobalt has not executed and no graphics backend
is selected. This is a native-tested math prototype, not renderer closure.

## Math-only SDK / cross-probe integration — 12:38 +07

After 95580ce, build_math_sdk.py additionally emits libmyos-math.a and a narrow
SDK include tree: math.h, fenv.h, features.h, x86_64 bits/fenv.h and MyOS
myos/math-types.h. COPYRIGHT accompanies the package. Upstream header content
is retained except math.h's bits/alltypes.h include is redirected to math-only
evaluation typedefs; no musl pthread, time, file or other OS types are exported.
The math header declares upstream functions beyond the selected closure; those
are not claimed implemented. Missing providers still fail at link, not fake
success. float/double evaluation types are checked in C/C++ for the x64 profile;
no claim covers every exported long-double function or other target ABI.

The renderer cross-probe uses these headers and real math/string objects in its
configure provider archive. C/C++ sqrt/clock provider link/no host libraries,
opaque pthread sizes and nine forbidden library routes each passed 12:38:46.
This is not a final renderer link or backend/context/present acceptance.
tests/test_math_sdk.py now consumes the exported header tree first: native BIOS
44692 PASS 12:20:46, retaining 1895 vectors, FP-state workers and exact relaunch
resources. No algorithm/flag changes or new UEFI/full-suite claim.
Real Mesa blob.c consumer uses the same header tree and passed BIOS 12:37:38;
PHASE332_RENDERER.md records bounds/OOM and remaining actual compile failures.
Normal BIOS/UEFI hashes unchanged; the valid full kernel/runtime run is retained.

## Pinned sources and target build

musl 1.2.6 archive SHA256:
d585fd3b613c66151fc3249e8ed44f77020cb5e6c1e635a616d3f9f82460512a.
Downloaded from https://sources.buildroot.net/musl/musl-1.2.6.tar.gz and matched
[Buildroot's source hash](https://raw.githubusercontent.com/buildroot/buildroot/master/package/musl/musl.hash).
That metadata says its hash was calculated after checking the upstream PGP
signature; MyOS checked the matching hash, NOT independently verified PGP.
COPYRIGHT SHA256 b870108ec5e7790e9f9919064f1b9421d62d5f9b0e6c230c6adf7ea2da62e97b.
Upstream source/license notices remain intact in each private extracted tree.
MIT plus permissive third-party math notices are described in COPYRIGHT.

tools/build_math_sdk.py verifies the sealed archive, extracts a fresh private
source tree for every build and generates musl's type header using its own
Makefile. It compiles only selected generic math and x86_64 FP environment
sources, not Linux libc/loader/syscalls. Math roots are explicitly listed in
the tool; missing helpers/tables are closed from actual undefined symbols.
A missing non-math provider fails rather than falling back to host libraries.
64 sources link a 46904-byte standalone prototype with no undefined symbols,
DT_NEEDED or PT_TLS; GNU_STACK is RW/nonexecuting and RELRO is present.

Build uses -nostdinc, target SDK plus pinned source/compiler intrinsic headers,
undefines Linux platform macros and defines __myos__. No fast-math;
-frounding-math, -ffp-contract=off and standard excess precision are retained.
Scoped upstream warning exceptions: intentional endian precedence, volatile
force-evaluation temporaries and GCC's unknown STDC pragma. The argument reducer
retains a visible maybe-uninitialized warning (nonfatal only for that file).
Selected huge/random inputs passed; this does not prove all binary inputs safe
or certify complete libm accuracy. The unmodified x86_64 fenv assembly uses its
user-stack red zone; C sources use -mno-red-zone. No signal delivery/unmasked
FP trap-mode compatibility is claimed.

## Native numeric and thread evidence

tools/math_oracle.py uses host MPFR 4.2.2 only to generate deterministic test
constants at 256-bit precision. See the [MPFR manual](https://www.mpfr.org/mpfr-current/mpfr.html).
It self-checks its LP64 record ABI; seed 0xc0ba17 adds 64 finite bit patterns.
Production MyOS runs the actual compiled math code, not host calculations.

tests/test_math_sdk.py / user/elf-sdk-math.c run 1895 generated vectors for 26
operations. Exact comparisons apply to selected rounding/basic/sqrt/FMA cases;
selected transcendental results permit at most 2 ULP. IEEE class and signed-zero
checks are strict. Additional direct cases cover frexp/frexpf, ldexp/ldexpf,
scalbn/scalbnf, min/max NaN handling and sincosf. Invalid sqrt and log-of-zero
exception flags are tested. This is sampled evidence, NOT a formal exhaustive
proof or all-functions/all-rounding-modes certification.

Four workers set different rounding modes and exception flags, sleep/preempt
12 times each and check rint results and FP-state isolation; main's state is
preserved. Three native launches verify exact page/heap recovery on frozen
private images. No kernel/shared-image rebuild or redundant full run.

- Initial BIOS 81465 PASS 11:22:15 +07; UEFI 99018 PASS 11:23:03.
- Expanded/fresh-source BIOS 51058 PASS 11:25:38; UEFI 49769 PASS 11:26:11.
- Logs: build/system-tests/sdk-math.log and sdk-math-uefi.log.
- BIOS SHA256 0849bb23f54b1f9c0bdf10b7c89f881263c3209827d8d3eaa9466f10da61fa6a.
- UEFI SHA256 5a28e2ae2c8b6b205b3ad19fd87027d08249daa9f6efb78f352906c818fdcf3c.

## Reproduce and remaining work

Archive prerequisite: build/math-evaluation/musl-1.2.6.tar.gz, exact hash above.
No global packages/install. MPFR libmpfr.so.6 is a host test-only prerequisite.
Run python3 tools/run_logged.py -- python3 tests/test_math_sdk.py [--uefi].
Source trees/objects remain disposable build artifacts, not a selected full
renderer distribution. Final renderer linking, libc/C++ dependency closure,
native GLES/EGL/context/presentation and real Cobalt web acceptance remain.
No Cobalt import count increase, Phase 33.3 work or push.
