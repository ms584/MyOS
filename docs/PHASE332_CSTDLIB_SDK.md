# Phase 33.2 sealed native Cstdlib dependency

2026-10-04, follows3699a87. This is a real C++ container/renderer dependency,
not Cobalt or a complete C++ library acceptance. No33.3/push/phase closure.

## Implementation and ABI

tools/build_cstdlib_sdk.py verifies SHA256 of existing musl1.2.6 archive:
d585fd3b613c66151fc3249e8ed44f77020cb5e6c1e635a616d3f9f82460512a.
Original div/ldiv/lldiv and UTF-8 mblen/mbtowc/mbstowcs/wcstombs, restartable
bulk converters/wcrtomb and existing mbrtowc/mbsinit/internal table are built
without host headers/libraries, retaining COPYRIGHT. Div struct declarations
are checked against the sealed header, including quotient/remainder order and
int/long/long-long types; C/C++ compile and native tests prove8/16/16-byte LP64.
Generated internal/locale headers only supply compiler visibility and the target
fixed UTF-8 profile. MB_CUR_MAX4 is not a claim of locale switching or C locale
compatibility. No replacement STL containers or copied glibc configurations.

user/platform-cstdlib.c reports the absent command processor truthfully:
system(NULL)=0, preserving errno; any command returns-1/ENOSYS and is NOT run.
This is explicit unavailability, not successful shell execution. The new archive
is SDK-linked into consumers, not embedded in kernel/runtime or newly counted
Cobalt exports. Integrate it into the checked renderer/C++ provider archive
before relying on its functions in linker configuration probes.

## Native evidence (+07)

tests/test_cstdlib_sdk.py --configuration
build/cxx-target-evaluation/configure-jf2fl8q3 [--uefi]. Actual C++ fixture uses
upstream <cstdlib>/std::div overloads, not a handwritten C++ wrapper substitute.
First attempts failed on source-relative internal.h include routes, then missing
SDK string primitives in the fixture. Fixed source staging/linked real existing
user/platform-sdk-string.c; no loader relaxation or unsupported success imports.
Initial89822 exit0 12:11:29 BIOS sdk-9aojw1eb/UEFI sdk-1tw58c1i,12 launches.
Final34118 exit0 12:14:54: BIOS sdk-_dtnozlr/UEFI sdk-ro45t3od,12 launches again,
with native unmapped second-page edges. Both C and C++ test:

- Quotient truncation/remainder signs over63 numerators/14 nonzero divisors,
  INT64 extrema where division is defined; zero/overflow UB is not disguised.
- ASCII/Thai/euro/emoji, partial explicit-state decoding, query-only lengths,
  exact destination limits/NUL behavior, restartable source-pointer progress,
  overlong/surrogate/out-of-range/truncated UTF-8 and invalid wide scalars.
- errno preservation/error reporting, reset requests, no false system success.
- Guard-page source/destination edges, including truncated input limited by n
  and full buffers without room for terminators; eight workers x20 repetitions.
- Repeated launch/exact pages/heap/file cleanup; no DT_NEEDED/host-library link.

The same34118 chain ran affected test_scan_sdk.py BIOS+UEFI successfully:
209 MPFR vectors/four rounding modes, numeric conversion, scanf/UTF-8/guard
edges, eight workers, allocation OOM/recovery and exact resources. SDK integer
154 macros/types C/C++/both char signs PASS hosted+freestanding12:11:45;
upstream-new/pthread renderer boundary PASS. No architecture/runtime change:
reuse full93613, normal kernel/runtime/BIOS/UEFI hashes unchanged12:15:35,
see PHASE332_OS_INFO.md. All jobs terminal.

## Still open / next work

Whole C++ make after changes exit2 12:15:04, log:
build/cxx-target-evaluation/configure-jf2fl8q3/build-after-cstdlib.log.
This Cstdlib declaration error group is cleared; strcoll/strxfrm, owned stdio
fpos/fgetpos/fsetpos/freopen/fscanf, time/cstdint down-level header routes,
locale/math and omitted libgcc unwind/libiberty support sources still fail.
No container library build, native std::vector/unordered_map/std::mutex, Mesa
rendering or actual Cobalt HTML/JS/DOM/input/verified HTTPS gate has passed.
Continue real implementation/testing for these dependencies; compile-only
configuration macros must never replace native semantics. Imports unchanged
163/242 coverage67.4%; actual Cobalt0/5; Phase33.1/33.2 remain OPEN.
