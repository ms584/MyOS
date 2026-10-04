# Phase33.2: EGL final-reference cleanup and pinned1.5 blocker

2026-10-04 (+07), baseline1214dc4. Phases33.1/33.2 OPEN, no33.3/push.
Imports163/242=67.4% supported subset; actual native Cobalt acceptance0/5.

## Evidence-driven lifetime correction

Original Mesa `_egl_relax_begin` increments resource references before dropping
the display lock; `_egl_relax_end` reacquires the lock and drops those pins.
Original `_eglPutResource` only returns whether the count reached zero; it does
not invoke a driver destructor. The previous MyOS driver freed only when its
DestroyContext/DestroySurface entry point dropped the last reference. A concurrent
API call could retain a core pin after destroy, so the core's later final release
would leak backing storage/context/display references. This was a source-level
finding, not a reproduced runtime leak in the earlier basic fixture.

`tools/mesa_egl_lifetime.patch` adds a MyOS-only OnFinalRelease callback to the
internal resource header and invokes it exactly when the atomic reference count
reaches zero. Driver context/surface destructors drop references only; the callback
performs actual OSMesa destruction/backing free/display release, including when
the final release occurs inside core code. Bound resources still defer cleanup
until unbind. All modified core and driver units rebuild together; no public
EGL/Starboard ABI or kernel/runtime layout is changed.

`user/egl-lifetime-test.c` is white-box native fixture support, not a platform
binding or replacement renderer. It acquires the same core pin, destroys the
public handle, confirms no early cleanup, drops the final pin and confirms one
actual driver destructor for a context and pbuffer. Callback observation wraps
and invokes the genuine destructor; no mocked storage or successful stub.
This deterministic reference-transition test does not prove all concurrent
ownership/terminate/reinitialize/failed-binding interleavings.

Strict source patch/17 units compile PASS18:22:31 core-_oys8k6i:
16 production units plus one fixture-support object. Archive objects are selected
on reference; normal use does not require the test entry point.
Native window BIOS qd7nly83/UEFI tbutzzx0 and pbuffer h_5qtanu PASS;
session39964 terminal exit0 18:26:43, two launches each/exact resources,
both context/surface last-pin destructor markers, original pixel/text/QMP/input
and deferred-bound destruction checks. Logs:
build/renderer-evaluation/native-egl-lifetime.log,
native-egl-lifetime-uefi.log, native-egl-pbuffer-after-lifetime.log.
No kernel/normal-image mutation; previous full9925 remains architecture evidence.
Four normal hashes unchanged18:25:29. Fetch18:26:42 confirms origin/main452faaa,
local HEAD5 ahead/0 behind before this checkpoint; no merge/push performed.

## Required1.5/SABI work, not merely two import names

Pinned build/engine-evaluation/contracts/egl.h explicitly requires EGL1.5.
The current backend truthfully reports1.4; its native window success is NOT a
complete SbGetEglInterface contract. Do not publish a complete interface or bump
version just to let Cobalt proceed. The pinned GLES header requires2.0 and makes
3.0 call availability the caller's responsibility.

Next: actual Gallium/softpipe fences (original st_flush/fence_finish/reference)
and EGL create/wait/query/destroy contracts; cross-thread waits must not depend
on a current GL context. Original softpipe completes synchronously, so an
already-signaled real fence is valid, but never invent successful completion.
Original EGL core gates the EGLAttrib CreateSync entry point on KHR_cl_event2;
audit OpenCL-absent behavior against the actual1.5 specification, not assumptions.
Also implement genuine EGL images/client texture/renderbuffer integration,
required context/colorspace/surfaceless contracts and exact pinned SABI tables;
then production module coexistence and actual Cobalt HTML/JS/DOM/input/HTTPS.

References: pinned egl.h/gles.h; original Mesa EGL/OSMesa/state_tracker/softpipe;
[EGL1.5 specification](https://registry.khronos.org/EGL/specs/eglspec.1.5.pdf),
[KHR fence sync](https://registry.khronos.org/EGL/extensions/KHR/EGL_KHR_fence_sync.txt),
[KHR wait sync](https://registry.khronos.org/EGL/extensions/KHR/EGL_KHR_wait_sync.txt).
