# Phase 33.2 measured string/C-locale renderer dependencies

Checkpoint follows 7d9242c; 2026-10-01. Neither 33.1 nor 33.2 is closed.

## Implementation and boundary

- Native SDK supplies strncmp, strcpy/strcat/strncat, strchrnul/strrchr, strpbrk,
  strspn/strcspn, strtok/strtok_r, strndup and ASCII strcasecmp/strncasecmp.
  Comparisons use unsigned bytes; bounded operations do not read past their
  bound. Duplication uses real guest malloc/free and terminates its own copy.
- C-locale ctype classifies EOF and all unsigned-char inputs, including high
  bytes; ASCII case mapping leaves other bytes unchanged. No host locale.
- strtok has the shared cursor allowed by ISO C, not a concurrency promise.
  Independent/concurrent parsing uses explicit caller-owned strtok_r cursors.
- Mesa's compiler boundary explicitly enables _GNU_SOURCE for the verified
  musl math header's extension constants. This changes feature visibility,
  not OS identity, dynamic dependencies or runtime capability claims. Linux
  identity remains undefined; host include/library fallback remains forbidden.
  GNU/BSD string.h exposes strings.h as the sealed musl header does; strchrnul
  is real because this feature profile disables Mesa's local fallback.
- No Cobalt import binding, C++ const-overload/full cstring, scanning,
  conversion, multibyte locale or complete libc claim follows from this SDK.

## Native evidence

Expanded tests/test_string_sdk.py compiles both C11 and C++17 without host
headers/libraries, then runs each three times in MyOS with exact reclaimed
heap/pages. Initial BIOS28240/UEFI22549 pass; final expanded GNU declaration/
strchrnul revision88757 BIOS exit0 18:01:42 sdk-8vibohmx, UEFI exit0
18:02:11 sdk-sqdbrjso. Assertions include:

- Existing 37281 overlapping moves, 65536 unsigned comparisons, allocator
  alignment/overflow/zero-fill/ownership and padding/truncation regressions.
- Every 256x256 bounded/case-folded byte pair and every EOF/byte ctype input.
- Search/spans, empty/changing delimiters, bounded unterminated source copies,
  terminating concatenation with guards and successful errno preservation.
- Eight native workers each perform 128 interleaved independent token parses.
- Sealed header M_PI/M_LOG2E binary64 bits match explicit expected values.

Formatter regression35253 exit0 17:55:19, sdk-uvito6_0: real sealed numeric/
UTF8/callback/bounds/locking/fwrite tests and relaunch/resources remain green.
Kernel/runtime/BIOS/UEFI unchanged (hashes 17:54:11 match PHASE331_UNLINK.md).
SDK-only impact does not invalidate the completed aggregate full regression;
no new uninterrupted full-run PASS is claimed.

## Renderer evidence / next dependency batch

Whole target compile24751 exit1 17:58:43, target-config-riek2d10: configure/
boundary PASS, 72 failed units/115 error lines versus prior100/212. Math
constants/string/ctype dependencies clear; NOT an engine completion percent.
Later GNU strings.h/strchrnul correction was checked with native fixtures
and a single incremental real Mesa u_debug compilation18:01:49: strcasecmp
error cleared, only strtoll/sscanf errors remain in that unit. Old whole-probe
report is retained unchanged, not relabelled after the later header edit.
No native graphics/backend/Cobalt execution follows from compile progress.
Next: genuine conversion/scanning, process exit, setjmp, OS services and
target C++ dependencies, followed by native EGL/GLES feasibility and all five
Cobalt HTML/JS/DOM/input/HTTPS/endurance gates. No Phase33.3 or push.
