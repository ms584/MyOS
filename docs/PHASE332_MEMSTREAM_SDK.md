# Phase 33.2 dynamic memory streams and real Mesa wrapper

2026-10-01 checkpoint following 8dea155. Native graphics/Cobalt acceptance
remains unproven; Phase 33.1/33.2 stay OPEN, no Phase 33.3 or push.

## Semantics / implementation

user/platform-stream.c now supplies open_memstream on the owned FILE adapter.
It uses native malloc/realloc and target pthread locking, not foreign FILE or
libc thread/syscall state. Streams participate in the shared flush/close registry
only after full initialization. They are write-only, unbuffered and seekable.

Semantics follow the [POSIX Issue 8 contract](https://pubs.opengroup.org/onlinepubs/9799919799/functions/open_memstream.html):
flush/close publish the buffer pointer and min(current position, buffer length).
The buffer has a trailing zero at its actual length. Seek does not extend data;
a later write zero-fills the gap. SEEK_END is relative to the full buffer length,
including after seeking backwards. Negative positions and invalid origins fail;
positions beyond signed 64-bit range and writes crossing that limit fail before
allocation/output. Caller owns the published allocation after close and frees it.
Pointers/sizes must not be read concurrently with a writer or retained across
operations which can move the allocation. No descriptors exist for these streams;
fileno and reading report EBADF. This is not pinned Cobalt FILE layout proof.

Growth validates combined pending/direct write size before realloc, preserving
data/position/published output on ENOMEM. Write errors clear all three buffer
pointers so clearerr plus a later write can reinitialize the formatter state.
The same recovery correction applies to descriptor streams. fclose releases
FILE/cookie/lock, not the caller's buffer; force-cancel reclaims process mappings
without promising stream destructors or publication callbacks.

## Native evidence (+07)

tests/test_memstream_sdk.py verifies sealed Mesa 25.0.7 archive SHA256
592272df3cf01e85e7db300c449df5061092574d099da275d19e97ef0510f8a6 and
compiles pristine src/util/memstream.c/h with only target SDK/compiler headers.
Both Mesa wrapper and own adapter run in MyOS on a frozen normal BIOS image.
No DT_NEEDED/host libc. Session 72224 exit 0 14:51:37,
build/format-evaluation/sdk-z_s76v13; build/system-tests/memstream-sdk.log.

- Empty output, invalid argument rollback and write-only/no descriptor behavior.
- POSIX overwrite example, flush/close publication and caller free after close.
- Binary bytes, seek beyond end without extension, zero-filled write gap,
  backwards SEEK_END, invalid negative/origin rejection and INT64_MAX overflow.
- Actual allocator exhaustion down to malloc(1); failed growth preserves original
  buffer/size/position, failed creation preserves caller outputs; freeing pressure
  and clearerr permit successful write/close/free.
- Eight workers emit 128 distinct serialized records into one stream. Real Mesa
  u_memstream_open/get/flush/close produces checked native formatted output.
- Three relaunches return the expected status, reclaim pages and exactly restore
  heap. Initial session 44717 failed with null memcpy on OOM recovery; stale
  wend after clearing wpos was the cause. Fix tested by the same reproducer;
  failed trial is not PASS.
- Formatter and file/console regressions session 97536 exit 0 14:52:39.
  Existing thread/error/ownership/OOM/cancel/relaunch behavior remains valid.
- Expanded file-stream fixture cancellation 32852 exit 0 14:53:56,
  sdk-m9_is93s: three Escape cancellations each with an unclosed memory
  stream holding 64 KiB, open asset and blocked pipe reader. All process pages
  reclaimed, identical heap-before/after, four subsequent ordinary launches
  still open all eight file handles. No destructor/publication promise.
- Renderer 30351 terminal exit 1 14:53:47, target-config-usw_gujn:
  configure 0 and compiler boundary checks pass. Real compile remains 1;
  memstream failure cleared. stat/temp/string/scanning/allocator/random/abort
  errors remain. This is measured progress, not graphics acceptance.

Only SDK/provider/fixture changes; no new kernel/runtime/full/UEFI run or
acceptance claim. Normal BIOS/UEFI hashes must remain baseline. Remaining
measured renderer dependencies include stat, temporary files, string/scanning,
allocator declarations and random/abort. Full stdio/locale/input/redirection and
pinned Cobalt FILE ABI remain open. Next: close measured libc/stat dependencies,
native target EGL/GLES, then real Cobalt HTML/JS/DOM/input/verified HTTPS gate.
