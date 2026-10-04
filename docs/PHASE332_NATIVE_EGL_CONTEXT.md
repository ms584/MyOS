# Phase33.2: genuine GLES context attributes and no-surface rendering

2026-10-04 (+07), baseline b8f8128. Phases33.1/33.2 OPEN; no33.3/push.
Imports163/242=67.4% supported subset; actual Cobalt acceptance0/5.

## Implementation

The MyOS EGL driver accepts original core-validated GLES2.0/3.0/3.1 requests
and passes actual requested versions to the genuine OSMesa constructor.
The single software config advertises ES2/ES3 renderability, never certification.
Requests beyond this pinned backend's measured GLES3.1 capability fail BAD_MATCH;
desktop/forward-compatible/robust/no-error modes are not fabricated.
Debug requests initialize actual Mesa GL_DEBUG_OUTPUT and context debug flags
before first binding, using original st_api_create_context debug initialization
functions. Debug callback dispatch is exercised in MyOS, not a string flag.

Surfaceless binding calls original st_api_make_current with NULL drawables:
there is no throw-away surface/backing pixel masquerading as no surface.
The original incomplete framebuffer rejects rendering until an application FBO
is bound. First surfaceless binding initializes viewport/scissor to zero and
marks viewport initialization complete: the first later surface must not reset
the application's viewport. Original Mesa's generic nonzero-dimension policy
alone did not meet this EGL requirement.

mesa_egl_current_deleted.patch changes only MyOS public current context/surface
queries: a still-bound, reference-held object returns its original handle after
public destruction. Registry lookup still rejects that handle in other EGL calls.
Generic resource handle validation is not relaxed. Actual last-reference cleanup
remains in effect. This follows EGL1.5 section3.7.4, not an invented live object.

## Evidence and discovered failures

Strict original-source patches/20 units compile PASS19:22:51 core-qfs_95xh:
17 production units and three native semantic test translation units.
BIOS window native-fets0c80 PASS41016 terminal exit0 19:24:43, two launches
and exact resources. UEFI native-6bp0_opw/pbuffer native-xtmebax1 PASS69227
terminal exit0 19:28:38, two launches each and exact resources. Each serial
log has two requested-version/debug callback/surfaceless FBO success markers.
Logs: build/renderer-evaluation/native-egl-context*.log.

Initial52175 failed19:14:43; diagnostic43020 failed19:16:30 and captured actual
GLES3.1/error0x500. GL_CONTEXT_FLAGS is a GLES3.2-only query in the original
Mesa get_hash_params.py, so the GLES3.1 test now checks INVALID_ENUM/unchanged
output and proves debug via actual enabled state and application callback.
78366 failed19:17:54 on viewport reset; fixed first-surface initialization.
57289 failed19:20:43 on deleted-current query; specification/source audit led
to the scoped public-query patch, not weakening the resource registry tests.

Native tests request debug3.1 and verify actual GL major/minor, first viewport/
scissor zeros, synchronous callback ID/type/severity/text/user pointer, undefined
default framebuffer error, exact four magenta FBO pixels, mixed missing-surface
rejection without losing current state, surface/no-surface transitions preserving
viewport, bound deferred destruction and unbinding. Invalid versions/flags/
forward-compatible/robust requests are checked. Existing shaders/textures/text,
image storage, eight fence waiters, native swaps/input and resource checks remain.

## Limits and next work

Still EGL1.4: colorspace and core1.5/OpenCL-absent contracts remain, followed by
pinned Starboard tables and actual loader/provider coexistence. Unequal draw/read
surfaces, binding-allocation failure rollback, termination/reinitialization and
broader concurrent ownership need audit. No GLES3.2/robustness/no-error promise.
Actual Cobalt has not executed; HTML/JS/DOM/input/verifiedHTTPS>=60s/relaunch
remains required. Kernel/runtime architecture unchanged: full9925 remains valid.
Normal four hashes unchanged19:29:18; all test jobs terminal, no image owner.

Primary specifications:
- https://registry.khronos.org/EGL/extensions/KHR/EGL_KHR_create_context.txt
- https://registry.khronos.org/EGL/extensions/KHR/EGL_KHR_surfaceless_context.txt
- https://registry.khronos.org/EGL/specs/eglspec.1.5.pdf (section3.7.4)

Reproduce through tools/run_logged.py with tests/test_native_renderer.py,
--renderer-probe build/renderer-evaluation/target-config-snob6k4x
--configuration build/cxx-target-evaluation/configure-_1j4xqjo
--gles --egl-core build/egl-core-evaluation/core-qfs_95xh;
add --egl-window for native window and --uefi for UEFI.
