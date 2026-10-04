# Phase33.2: exact pinned GLES table and native table dispatch

2026-10-04 (+07), baseline1289e65. Phases33.1/33.2 OPEN; no33.3/push.
Imports163/242=67.4% fixture-supported subset; actual Cobalt acceptance0/5.

## Implementation

tools/generate_gles_interface.py generates build artifacts from the actual
hash-verified pinned SbGlesInterface declaration, preserving all246 fields.
This is246 GLES table slots, not242 Cobalt imports. Each generated typed
assignment and every field offset/table size are checked by the target compiler.
Pointer/size scalar ABI checks use the actual compiler/Khronos declarations.
Upstream EGL/GLES/log/system headers remain unchanged. Generation is not a
handwritten function-name registry, generic signature cast or success stub.

245 entries reference real Mesa GL functions directly. glGetInternalformativ
exists in original Mesa formatquery.c and dispatch XML (GLES3.0), but its standalone
symbol is absent from the pinned glapi archive. That single entry is obtained
through actual eglGetProcAddress using original C11 call_once. A separate compiler
type-compatibility assertion checks its precise signature before the lookup cast.
Getter returns NULL if actual lookup fails, not a fabricated implementation.
The table is module-owned writable data during one-time initialization, published
as const after call_once; it is not claimed to be a read-only RELRO table.

Native GLES+EGL fixture includes generated dispatch macros after real GL headers;
its GL calls for version/errors, shader compilation/linking, VBO/texture draws,
gradient1024 pixels, glyph atlas/alpha512 pixels, fence-associated writes and
presentation checks now call SbGetGlesInterface()->actual field. Desktop/non-EGL
profiles retain their original paths. Getter identity/all246 nonNULL slots are
checked; eight fence-wait workers also retrieve the getter after renderer teardown.
No dispatcher injection from host, kernel-rendered substitute or forced pixels.

Native internal-format query checks real GL_NUM_SAMPLE_COUNTS on RGBA8, then
invalid pname rejects with GL_INVALID_ENUM and unchanged output77. Actual query
is exercised, not merely a nonNULL proc pointer. Existing pinned44 EGL table/core
context/image/fence/draw-read/sRGB tests remain. Slot/type/link checks do not prove
every API's complete semantics, nor Cobalt renderer input/HTML/DOM behavior.

## Evidence and reproduction

Initial24/24 compile PASS20:38:06 core-wrrdg7ns. Native25288 terminal exit1
20:39:11 native-xobuvl21: loader correctly rejected one missing strong symbol,
glGetInternalformativ, before execution. That is NOT a native pass. Actual source/
XML audit confirmed implementation; fixed genuine lookup, not unresolved fallback.
Final core-0ky0zq6u24/24 strict compile PASS20:44:26.
Final BIOSsRGB native-21xnvj1n and UEFIsRGB native-kc_zt45s PASS43457
terminal exit0 20:50:27, two launches/exact resources each. All required table/
actual-query markers present; no live owner. Four normal hashes unchanged20:51:45.
Dynamic-symbol inspection20:51:45 confirms both getters are GLOBAL DEFAULT FUNC
exports, not only internal static calls. Actual provider mapping remains pending.
Logs: build/renderer-evaluation/gles-interface-target*.log and
native-gles-interface*.log; per-fixture serial/QMP PPMs remain preserved.
Kernel/runtime unchanged; full9925 architecture evidence remains applicable.

Reproduce via tools/run_logged.py: tests/test_native_renderer.py
--renderer-probe build/renderer-evaluation/target-config-snob6k4x
--configuration build/cxx-target-evaluation/configure-_1j4xqjo --gles
--egl-core build/egl-core-evaluation/core-0ky0zq6u --egl-window --egl-srgb;
add --uefi for UEFI. Older core archives do not contain these generated headers/
tables; historical reproduction requires its corresponding source checkpoint.

## Required next integration

SbGetEglInterface/SbGetGlesInterface exist in this native fixture, not the kernel
runtime allowlist. Production remains single-object eager linking; DT_NEEDED and
external TLS modules rejected. Add actual provider mapping/coexistence/export
resolution/relocations/lifecycle/TLS ownership with adversarial/fault/OOM tests,
not hardcoded user addresses or import-name success. Test kernel262072/cap262144
has72 bytes headroom; kernel/BSS low-memory layout and image cap remain unchanged.
Then remaining79 strong imports/weakTLS and actual Cobalt HTML/JS/DOM/input/
verifiedHTTPS>=60s, close/relaunch gate. Imports have not increased merely because
two fixture getters hide290 table entries. No phase closure/certification claim.
