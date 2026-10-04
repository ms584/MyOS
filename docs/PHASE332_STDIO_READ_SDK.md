# Phase 33.2 native stdio read/format dependencies

2026-10-04 checkpoint follows d147ec8. Phase33.1/33.2 remain OPEN.
Git fetch07:29 confirmed origin/main=d147ec8. No Phase33.3 or push.

## Implementation and supported contract
- build_format_sdk.py compiles unchanged sprintf.c/vsprintf.c from sealed
  musl1.2.6 alongside the real formatter. No host libc or DT_NEEDED libraries.
  vsprintf uses vsnprintf(INT_MAX), preserving real formatting/error behavior;
  the caller MUST supply enough writable memory. This is not a bounded API.
- getc calls the existing locked fgetc, returning an unsigned byte or EOF.
- fgets uses a private read_locked helper shared with fread; holds one stream
  mutex across the whole call, preventing interleaving of a single line.
  Reads at most count-1 bytes, includes newline, NUL-terminates on success.
  Partial final line at EOF succeeds; empty EOF returns NULL without touching
  output. Embedded NUL/255 are bytes, not EOF or early termination.
- SDK extension: count<=0 returns NULL/EINVAL without reading/writing; count1
  writes NUL and returns output without reading. Read error returns NULL and
  sets stream error; output after partial error is not promised as a string.
  Nonblocking partial EAGAIN is an error, not fabricated EOF/success.
- Retains unbuffered owned FILE, access/error/EOF/cancel semantics; no pinned
  Cobalt FILE ABI, stdin/redirection, flockfile, buffered modes or full POSIX.
  Concurrent fclose/use still requires caller lifetime coordination.

## Native validation (+07)
Reproduce through tools/run_logged.py in /home/xytron404/myos:

    python3 tests/test_format_sdk.py
    python3 tests/test_stream_sdk.py
    python3 tests/test_stream_sdk.py --uefi --cpp

Formatter session75731 passed BIOS sdk-u22lwj7g: sprintf/vsprintf canaries,
INT64_MIN/hex/float/long-double/positional/empty output plus original formatter
UTF8/overflow/callback/three-launch exact resource regression.
Final expanded stream12440 exited0 07:38:27.104122: BIOS C11 sdk-zu4r_ko4 and
UEFI C++17 sdk-urta56f3, each four launches and three Escape cancellations.
Counts -1/0/1, short buffers/canaries, NUL/255/newline/chunked lines/final
unterminated line/empty EOF/seek reset/write-only EBADF/partial pipe EAGAIN
and clearerr->EOF; eight readers consume128 complete unique shared records.
Existing fd ownership/append/CLOEXEC/OOM/console/8writers/resources remain PASS.
Cancel fixture now blocks inside fgets while holding the stream lock; whole
process teardown and subsequent launch recover exact heap/pages/handles.
Private frozen boot copies, no host C++ headers/libraries/exceptions/RTTI.
Initial formatter fixture compile failed empty-format warning, then failed an
invalid mixed positional/nonpositional format; fixed fixture, not production
semantics or disabled strict warnings. Failed attempts are NOT passes.

## Renderer and checkpoint limits
Targeted ninja compile exited0 07:37:21.384032 in target-config-u8ev3uvw/build:
vtn_opencl, prog_print, prog_statevars, hud_cpu, pp_mlaa, u_debug_describe,
u_simple_shaders. All seven previously sprintf/fgets-blocked units now compile.
Original compile.log/report.json (33 failed units/47 errors) are not rewritten;
no new whole count, linked/native renderer, EGL/GLES or Cobalt success claimed.
Remaining exit/abort, OS-info, file locks/buffered stdio and C++ are next.
Kernel/runtime/BIOS/UEFI SHA256 unchanged07:34:29 versus previous checkpoint.
No shared build images modified; SDK-only changes, no new full-suite claim.
Last full evidence remains prefix83161 + suffix89247, not uninterrupted PASS.
All test jobs terminal. Commands logged; never log reads of activity.log.
