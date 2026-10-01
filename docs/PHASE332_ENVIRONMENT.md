# Phase 33.2 — process-local renderer environment

2026-10-01; checkpoint follows 6ea5840. Phases 33.1/33.2 remain OPEN.

user/platform-environment.c implements getenv/setenv/unsetenv in the target SDK,
not kernel exports or WSL forwarding. Each ELF image starts with an empty
environment; renderer configuration must explicitly populate it in MyOS.
Names/values are copied under the existing native pthread mutex. Active keys
are limited to 64, and copied/retired strings together consume a fixed 64 KiB
arena. Previously returned values remain valid until process teardown, even
after replacement/removal. Callers must not mutate those returned strings.
Concurrent writers serialize table changes; readers see a complete copied
value. Unknown getenv keys return NULL without changing errno. Invalid
set/unset names (empty or containing '=') and NULL set values fail EINVAL.
setenv overwrite=0 preserves an existing value. Capacity exhaustion fails
ENOMEM without modifying the table or the old value. Successful operations
preserve errno. unset frees an active slot but does not recover arena bytes;
this deliberately trades bounded storage for stable borrowed pointers. A long
mutation history may exhaust the arena even with few active keys. No putenv,
environ pointer, host inheritance, locale installation or shell config is added.

getgid/getegid return the same single guest identity (0) as runtime getuid/
geteuid. This does not provide permission enforcement, groups, set-id programs
or credential mutation. Actual Mesa __normal_user and secure_getenv wrappers
are tested, not substituted with Linux security policy. C11 assert.h now
maps static_assert to _Static_assert only for pre-C23 C; C++ keeps its keyword.
The renderer probe archive includes the real environment provider and its
boundary source now links group/environment calls explicitly.

## Evidence

- tests/test_environment_sdk.py BIOS session 14514 exited 0 at 13:44:11 +07,
  build/environment-evaluation/native-n8n45tmd/ and environment-sdk.log.
  Real sealed Mesa u_debug.h wrappers execute in MyOS. Tests: no deliberately
  seeded host variable, copied inputs, overwrite/no-overwrite/empty value,
  invalid names, unchanged errno on absent unset, stable retired pointers,
  eight synchronized workers reading/replacing values, 64-key exhaustion and
  slot reuse, arena exhaustion/preserved value, three empty-environment
  relaunches and exact page/heap resources. First compile omitted the same
  endian patch used by the renderer; fixed before any native PASS claim.
- tests/test_integer_sdk.py C11/C++17 header/static_assert regression PASS
  13:45:37, including all 154 format macros and both plain-char modes.
- Renderer 33565: configure 0/compile 1 at 13:45:34,
  build/renderer-evaluation/target-config-ilqu9vvw/. Environment/static_assert
  compile failures are cleared; remaining measured failures are stdio/FILE,
  sys/stat.h and declarations/providers for posix_memalign/calloc/rand/strcmp.
- C AND C++ actual environment/math-provider boundary PASS 13:47:15 with
  --environment. No undefined symbols/DT_NEEDED/Linux macros; nine forbidden
  library/search/response/OpenMP routes rejected in each language.

Kernel/runtime/normal BIOS and UEFI images unchanged; private frozen BIOS
only. No new UEFI/full-suite claim. No increase in pinned Cobalt import
coverage, selected graphics backend, native Cobalt execution, web acceptance
or Phase 33.3. Owner pushes manually. Next: real stdio/stat/libc closure and
target C++, then native EGL/GLES/context/presentation and Cobalt web gate.
