# Phase 33.2 measured renderer string / allocator SDK

2026-10-01 checkpoint following 00c9b6e. Phase 33.1/33.2 remain OPEN;
renderer/Cobalt have not executed, no Phase 33.3 or push.

## Changes

user/platform-sdk-string.c now implements memmove, memcmp, strchr, strncpy,
strstr and strdup, matching actual Mesa compile failures. Target declarations
are in user/sdk/string.h. Overlap moves select direction by integer addresses
on the flat x86-64 target; byte comparisons are unsigned, strchr includes the
terminator and converts the argument to a byte, strncpy pads or truncates at
the given bound, strstr handles an empty needle, and strdup copies into native
malloc ownership. These are real providers, not configure stubs or host libc.

user/sdk/stdlib.h now declares existing runtime calloc/aligned_alloc/
posix_memalign. No allocator/provider/kernel changes: alignment validation,
overflow, zero pages, explicit free and direct posix_memalign return codes use
the existing native runtime. This does NOT establish complete libc conformance;
zero-size posix_memalign currently reports ENOMEM and still needs an explicit
ABI/semantic decision/fix. No full C++ const-overload string header claim;
these are C ABI declarations callable from the tested C++ target.

## Native evidence (+07)

- Initial string/allocator test 98058 exit 0 15:00:27, sdk-xa0x26oo.
- Final tests/test_string_sdk.py including measured strdup dependency,
  session 58821 exit 0 15:04:02, build/format-evaluation/sdk-oaksy2zo;
  serial build/system-tests/string-sdk.log. Frozen normal BIOS, 64 MiB.
- C11 and C++17 fixtures both execute natively three times each, with real
  target imports and no DT_NEEDED. Exact kernel pages/heap reclaimed.
- Each fixture checks 37,281 overlap moves (0..128 bytes, both directions,
  same pointer), all 65,536 unsigned-byte comparison pairs, zero counts and
  memchr byte conversion; whole-array equality and sentinel guards checked.
- String search terminator/high bytes/empty needle/failed search/repeated
  prefix, strncpy padding/truncation/zero count/guards, bounded strnlen;
  strdup empty/high-byte contents/copy isolation/caller free.
- Nonzero posix_memalign alignment 8..65536 and aligned_alloc alignment/free,
  direct EINVAL/ENOMEM preserving caller pointer and errno, calloc overflow
  and fresh/reused zero-fill allocations tested. Zero-size contracts and
  protected-page read-boundary matrix are not claimed by this fixture.
- Integer SDK C/C++ macro/type regression PASS, then real sealed Mesa blob
  OOM/trim/strings/relaunch PASS: session 2027 exit 0 15:02:28,
  build/mesa-blob-evaluation/native-wkwk18wb (before strdup-only addition;
  all existing byte primitives and headers remain unchanged afterward).
- Renderer 88515 terminal exit 1 15:02:19, target-config-matyj2n9:
  configure 0/boundary pass; prior string and allocator declaration errors
  cleared, exposing strdup and OS-info/atexit platform checks. strdup was
  subsequently implemented and tested above; no graphics pass inferred.
- Final renderer 13012 terminal exit 1 15:06:14, target-config-h0vmsizj:
  configure 0/boundary pass; string/allocator declaration failures cleared.
  stat/temp/random/abort/scanning, OS-info/atexit and OS-socket ENOSYS remain.
  No host libraries or backend selected; no native renderer execution.

Only target SDK/provider/fixture changes; no new full/UEFI run or claimed
architectural regression. Normal kernel/runtime/BIOS/UEFI remain unchanged.
Next measured work: sys/stat.h and proper file metadata/temporary lifetime,
abort/random/scanning, target OS-info/atexit adaptation; then native EGL/GLES
and actual Cobalt HTML/JS/DOM/input/verified HTTPS interactive endurance gate.
