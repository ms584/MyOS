# Phase 33.2 owned unbuffered file/console stream SDK

2026-10-01 checkpoint following 06738ef. Phase 33.1/33.2 remain OPEN.
This makes the measured renderer file-stream dependencies real in MyOS;
it is not native graphics, Cobalt execution or a complete libc/stdio claim.

## Implementation

- user/platform-stream.c supplies owned FILE lifecycle, fopen/fdopen/fclose,
  fread, fflush (including NULL), seek/tell, error/EOF handling, fileno,
  unbuffered setvbuf, byte/string output and printf/fprintf/vprintf.
- Regular files use existing runtime descriptors and inherit their access,
  append, bounds, asset, cancellation and kernel-handle behavior. fdopen takes
  ownership on success; rejects incompatible access and exclusive creation;
  sets append/cloexec when requested. Failure releases FILE but retains the
  caller's descriptor; fopen allocation failure closes its new descriptor.
- A registry serializes fflush(NULL)/close; each stream has a real pinned
  target pthread mutex. printf records/puts are locked across each operation.
  Callers must coordinate FILE lifetime: concurrent fclose/use is invalid.
- stdout/stderr use the guest console syscall, split at its 256-byte limit,
  checking partial/error progress. They share the guest console, not descriptor
  redirection. stdin reports ENOTSUP rather than fabricated input or EOF.
- The sealed formatter uses this callback adapter; F_NOWR sets EBADF.
  Internal FILE layout is private and NOT verified for Cobalt-owned FILE.

## Native evidence (+07)

- Formatter regression exit 0 14:32:50, sdk-xir0r5hd: existing formatting,
  UTF-8/callback/overflow behavior and three relaunches still pass.
- Final tests/test_stream_sdk.py session 88411 exit 0 14:40:56,
  build/format-evaluation/sdk-fcalv8la; serial build/system-tests/stream-sdk.log.
  Frozen normal BIOS image, 64 MiB; no DT_NEEDED in either fixture.
- Binary/partial-element reads, append despite seek, EOF/clearerr/error,
  write-to-read stream EBADF, size overflow, seek/tell, exclusive creation,
  fdopen mismatch preserving descriptor, append/cloexec and fclose ownership.
- Eight workers emit 128 distinct complete records into a shared FILE; no
  interleaving/duplicates. Eight kernel file handles can reopen after exhaustion.
- Exhaust native malloc to its smallest allocation; failed fopen preserves
  ENOMEM and closes its fresh descriptor. All eight files reopen afterward.
- Console exact 600-byte payload proves chunking; stderr formatting verified.
- Three Escape cancellations with worker blocked reading pipe plus an open
  asset FILE, followed by four ordinary launches; kernel pages reclaimed and
  heap-before equals heap-after each time, eight file handles still usable.
- Earlier expanded test failed compiling undeclared ENOENT; added target
  runtime value 2. A cancel trial creating a persistent RAMFS file reported
  +4096 heap. fs/ramfs.c creates a page per file and retains it until unlink;
  this is expected data lifetime, not process allocation. Final cancellation
  fixture uses an existing asset so exact reclamation checks are meaningful.
  These failed trials are not counted PASS.

## Renderer evidence / remaining work

tools/probe_renderer_target.py --compile session 13859 terminal exit 1
14:38:56, target-config-kx6iuc3g. Configure 0 and boundary checks pass;
real compile remains 1. File-stream errors cleared, exposing remaining
mkstemp/unlink, sys/stat.h, abort, posix_memalign/calloc declarations, rand,
strstr/strncpy/strchr/memmove/memcmp, open_memstream and scanning needs.
The measured errors determine the next work, not an import-count target.

Only unbuffered setvbuf is supported. No open_memstream, scanning, locale,
stdin, console redirection, full POSIX pipe-seek errors or pinned Cobalt FILE
ABI proof. Runtime file bounds/persistence limits remain PHASE331_FILES.md.
Force-cancel reclaims process mappings/handles, not C/C++ stream finalizers.
No kernel/runtime changes; full/UEFI earlier baseline remains valid but no
new full/UEFI result is claimed. Normal BIOS/UEFI images are untouched.
No Phase 33.3 and no push. Next: real memory streams/libc/stat dependencies,
then target EGL/GLES and actual Cobalt HTML/JS/DOM/input/verified HTTPS gate.
