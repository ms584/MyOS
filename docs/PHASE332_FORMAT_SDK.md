# Phase 33.2 — sealed memory/callback formatting

2026-10-01; checkpoint follows bd4da4b. No complete stdio/renderer/Cobalt claim.

tools/build_format_sdk.py uses the already hash-verified musl 1.2.6 archive
through build_math_sdk.py. It builds real vfprintf/vsnprintf/snprintf/vasprintf/
asprintf/fwrite, the pure __towrite transition and three long-double helpers
(frexpl, __fpclassifyl, __signbitl). Central COPYRIGHT is retained in artifacts.
The foreign stdio exit hook is not compiled. Private stdio_impl.h supplies
callback state and actual native pthread locking, not musl/Linux thread state
or syscalls. Its FILE layout is internal to this SDK and is NOT a validated
layout for objects created directly by the pinned Cobalt core.

This provides memory formatting and caller-owned callback streams only.
No fopen/fclose/fread/fflush, stdout/stderr, input/scanning or memstream lifecycle
is implemented here. Those essential renderer dependencies remain explicit
build failures rather than fake success. fwrite multiplication overflow is
rejected with EOVERFLOW before locking/writing. A closure audit permits only
real malloc/free/errno/pthread lock/unlock dependencies. Renderer probes reuse
the existing verified math build rather than compiling it a second time.

The target adapter implements stateless UTF-8 wctomb (surrogates/out-of-range
fail EILSEQ), ASCII isdigit for parsing, and immutable strerror messages for a
documented partial errno table; other values return "Unknown error". No locale
switching, stateful encoding, full ctype/wchar or complete error-message table.
Real strnlen and unsigned-byte strcmp are added. Limits set UTF-8 MB_LEN_MAX=4
and positional-argument NL_ARGMAX=9. The former Mesa patch that skipped stdio.h
is removed now that the real SDK FILE type/header exists.

## Evidence and remaining work

- Final native BIOS tests/test_format_sdk.py exited 0 at 14:10:31 +07,
  build/format-evaluation/sdk-v5vftwxs/, format-sdk.log. Three launches with
  exact page/heap recovery: signed/unsigned 64-bit extrema, decimal float and
  long double, hex float, infinity/NaN/negative zero, positional arguments,
  count-only/truncated/one-byte buffers and guards, errno message, Thai/emoji
  UTF-8 and invalid surrogate, allocated output/free, strnlen/strcmp, real
  callback output/native locking and fwrite overflow before output mutation.
  These are focused samples, not exhaustive printf/float/locale conformance.
- Earlier attempts: compile warnings in upstream code; then loader refused two
  missing long-double helpers (fixed and added to closure audit); then a wrong
  expected fixture length (32 versus actual 29) was corrected using the literal
  size. None of those failed attempts is counted PASS.
- Selected upstream sign/fallthrough/unterminated-array/maybe-uninitialized
  warnings remain visible and nonfatal only for affected upstream sources.
  Own adapters still use -Werror; math's existing warning remains visible.
- Integer C11/C++17 regression PASS 14:04:28. Native Mesa thread utility
  regression 10874 PASS 14:06:01 after restoring stdio include: barriers/TLS/
  names/masks, fault, cancellation/relaunch/resources.
- Final renderer 46154: setup 0/compile 1 at 14:07:35,
  target-config-qpv095ob. FILE type issue is cleared. Remaining measured needs:
  fprintf/stderr/fopen/open_memstream, stat, mkstemp/unlink/ENOENT, abort,
  posix_memalign/calloc/rand, strstr/strncpy and further dependency closure.
- C AND C++ actual format/environment/math link boundary PASS 14:09:20:
  no undefined/DT_NEEDED/Linux macros; forbidden host-link routes rejected.

Kernel/runtime/normal BIOS/UEFI unchanged; private frozen BIOS tests only.
Existing full regression remains valid for unchanged architecture; no new
full-suite/UEFI claim. No pinned import-count increase or native graphics/web
gate evidence. Phases 33.1/33.2 OPEN, no Phase 33.3 or push. Next: real file/
standard/memory streams and remaining libc, target C++, native EGL/GLES and
Cobalt HTML/JS/DOM/input/verified HTTPS >=60 seconds with close/relaunch.
