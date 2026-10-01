# Phase 33.2 sealed memory scanning / conversion SDK

Checkpoint follows d96c8ca; 2026-10-01. No Phase33.1/33.2 closure or33.3.

## Genuine implementation boundary

tools/build_scan_sdk.py uses the hash-verified musl1.2.6 archive retained by
build_math_sdk.py (SHA d585fd3b613c66151fc3249e8ed44f77020cb5e6c1e635a616d3f9f82460512a).
Original integer/floating scanners, strto* converters, atoi/atol/atoll/atof,
abs/labs/llabs/bsearch, memory vsscanf/sscanf and UTF8 state table/mbrtowc/
mbsinit are compiled without host headers/libraries/syscalls. Pure long-double
helpers close their actual math dependencies. COPYRIGHT and compile diagnostics
are retained in build/scan-evaluation/sdk-*/. No production MPFR dependency.

Private memory-scanner FILE has only scanner/string-reader fields. It is NOT
the public SDK stream FILE or Cobalt's FILE ABI. Its refill executes the actual
musl string reader; local scanner state needs no pthread/global FILE lock.
vfscanf is renamed hidden __myos_memory_vfscanf, with the public FILE alias
removed. Unused wctype include is removed; allocation cleanup pointers are
initialized, retaining Werror. UTF8 internal.h receives explicit visibility;
private MB_CUR_MAX=4 replaces foreign libc locale/thread lookup. No locale
switching, public fscanf/scanf/file scanner or complete libc claim.

SDK stdlib/stdio/wchar declarations expose actual providers. Renderer archive
includes them; this is not an increase in proven Cobalt import bindings.

## Native semantics and regression

tests/test_scan_sdk.py builds C11 and C++17 guest ELF libraries, no DT_NEEDED
host dependency. Each runs three times with exact reclaimed heap/pages.

- Bases2..36 x96 generated64-bit values: unsigned/signed/negative results,
  end pointers, successful errno, base0/prefixes, invalid/no digits and overflow.
- 209 MPFR4096-bit decimal reference cases for both float/double: normal,
  subnormal, signed zero, extremes and halfway values. Four rounding modes,
  long-double low mantissa bit, NaN/Inf, exponent rollback and range errors.
- Real unchanged sealed Mesa25.0.7 strtod.c wrapper (HAVE_STRTOF/native C11
  type profile) uses guest providers and matches both sets of MPFR vectors.
- Nineteen readonly strings terminate immediately before a PROT_NONE page:
  empty/incomplete forms and31..600byte runs, including reader refill boundary.
  Conversion outputs/end/errno and bounded sscanf match ordinary buffers.
- scanf sizes/widths/scansets/suppression/counts/matching/EOF and Mesa-style
  0x%08x/hash plus %hhx formats, allocated300byte strings, real malloc/realloc
  failure injection/rollback/free/recovery. No production injected success.
- UTF8 byte-at-a-time and complete Thai/emoji; rejects overlong/surrogate/
  out-of-range/isolated continuation bytes and restores initial state.
- Eight native workers parse using private state and preserve thread errno.

Initial BIOS75541 PASS18:16:58; UEFI+formatter85049 PASS18:19:15/18:19:26.
Guard-expanded74841 BIOS/UEFI PASS18:22:48/18:23:13. Final real-Mesa74650
BIOS sdk-wvfhia0c + UEFI sdk-_88c4x_j both PASS; enclosing chain exit0
18:29:46. Full renderer24500 and all scoped test jobs are terminal.
Earlier builder/test setup failures (visibility/helper/include/type-profile)
were fixed, not counted PASS. Normal kernel/runtime/BIOS/UEFI hashes unchanged
18:20:09; SDK-only impact reuses valid aggregate full regression, not a new run.

## Renderer / remaining gate

Whole24500 exit1 18:24:49, target-config-e87xjgfy: configure/boundary PASS;
53 failed units/80 errors versus prior72/115. Conversion/scanning/abs/bsearch
compile errors cleared. No native backend/context/presentation/Cobalt proof.
Remaining: exit/abort, alloca, setjmp, file/OS services, stdio details and genuine
target C++ dependencies; then EGL/GLES and all native Cobalt acceptance gates.
No host libc/GL/browser substitute, completion percent, Phase33.3 or push.
