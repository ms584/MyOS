# Phase 33.2 native UTC calendar / C++ ctime dependency

2026-10-04, followsa79d97c. Not renderer/Cobalt or full C++ acceptance.

## Real implementation and limits

build_time_sdk.py verifies sealed musl1.2.6 SHA256
d585fd3b613c66151fc3249e8ed44f77020cb5e6c1e635a616d3f9f82460512a.
Original Gregorian year/month/tm/seconds converters and strftime algorithms
are retained, with private symbol/include/locale-plumbing adaptation only.
The fixed English time-name/format table and constants come from that same
archive, not a fabricated public locale.h/langinfo API. Widen day subtraction
before subtracting one, avoiding signed-int UB for INT_MIN day normalization.
COPYRIGHT/source remain under each generated musl source directory.

platform-calendar.c adds true UTC mktime normalization, traditional shared
gmtime/localtime/asctime/ctime buffers and reentrant variants. UTC only, no TZ
database/DST/locale switching. Out-of-range mktime preserves input; asctime
rejects unrepresentable fields/year outside0..9999 with EOVERFLOW and unchanged
output, rather than overflowing the caller's26-byte buffer. Traditional static
interfaces are NOT thread-safe; workers use _r and their own buffers.
difftime computes unsigned magnitude before double conversion, avoiding signed
overflow and preserving one-second differences near signed64-bit endpoints.
clock_t is target long/CLOCKS_PER_SEC1000000. CPU accounting remains unavailable:
clock returns-1/ENOSYS, NEVER a fabricated wall-clock CPU measurement. Pinned
Cobalt imports clock_gettime, not clock; native realtime/monotonic stay unchanged.
Calendar consumers link real format/scan/math SDK dependencies, not host libc.

## Native evidence (+07)

52566 exit0 13:15:24: BIOS sdk-0zfzblhk/UEFI sdk-9ud7bjot; C11 and actual GCC
<ctime> C++98/17, six launches/profile. 264 independent Python civil/ISO-week
oracle vectors (1600..9999, each month first/last day), negative epoch/leap/
century/2038 dates, week/year rollover, tm normalization and isdst reset,
signed64 difftime extremes, invalid-year output preservation, all standard
strftime directives plus supported padding/E/O extensions, every capacity,
errno/canaries/unmapped page boundaries and eight workers x40 reentrant rounds.
No DT_NEEDED; repeated launch/exact page/heap/file resource recovery.

23097 exit0 13:21:06: affected Mesa clock/wait tests now BIOS AND UEFI: real
monotonic/realtime/deadlines, unnotified32-bit wait, condition/carry/calendar,
eight blocked-worker cancellation/relaunch/resources. Stream tests both PASS:
unbuffered file/console, read/seek/EOF/partial-EAGAIN, getc/atomic fgets, eight
worker serialization, descriptor limits, OOM rollback and cancellation.
New _IOFBF0/_IOLBF1 constants match stdio ABI; their modes still correctly return
ENOTSUP without changing stream position or fabricating buffering support.

## Actual renderer result / next work

mesa_optional_target.patch includes real sys/stat.h for existing native mkdir.
For MyOS's ALREADY disabled shader-cache configuration only, Meson no longer
compiles unused cache database objects requiring flock. Enabled cache/non-MyOS
profiles retain those objects; no fake sys/file.h or successful flock API.
64295 terminal configure0/compile1 13:24:22, target-config-snob6k4x: two failed
ASTC LUT C++ units/10 errors (previous5 units/13 errors). Ctime/cache/mkdir/
_IOLBF groups clear. Remaining generated GCC error_constants needs canonical
errno declarations; then investigate genuine mutex/once/provider link closure.
No full C++ library/native backend/context/present/Cobalt gate yet.

Normal kernel/runtime/BIOS/UEFI four hashes unchanged13:25:07:
PHASE332_OS_INFO.md. Architecture unchanged, reuse full93613. All jobs terminal.
Reproduce test_calendar_sdk.py --configuration
build/cxx-target-evaluation/configure-_1j4xqjo [--uefi]; test_mesa_time.py
[--uefi], test_stream_sdk.py [--uefi]. Imports163/242 coverage67.4%, actual
Cobalt0/5, phases OPEN; no33.3/push. Continue to real native web endurance.
