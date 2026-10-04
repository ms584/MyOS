# Phase 33.2: native MyOS EGL window swap

Date2026-10-04 (+07), baseline437b701. Phases33.1/33.2 OPEN; no33.3/push.
Imports163/242=67.4% supported subset; actual Cobalt acceptance0/5.

## Implemented path

`tools/mesa_egl_myos_platform.patch` adds a MyOS-only native platform enum/name
to original Mesa EGL. `tools/probe_egl_core_target.py` copies EGL core into an
isolated output directory and applies the patch with --fuzz=0. No mutation of
the existing renderer source/archive or production kernel/runtime is needed.
Default native display selects MyOS; only EGL_DEFAULT_DISPLAY is implemented.
The original SURFACELESS platform still disallows native windows; it has not
been renamed or made to violate its contract. EGL core still reports1.4.

`user/platform-egl-driver.c` advertises EGL_WINDOW_BIT only on native MyOS.
Window handles are the pinned SbWindowGetPlatformHandle result, checked against
the actual kernel UI owner/generation and queried framebuffer dimensions.
Owned RGBA backing is overflow-checked; EGL resources never dereference a
native handle as a host pointer. The existing core rejects two live surfaces
for one window. Surface destruction frees backing, not the Starboard window.
OSMesaPixelStore(Y_UP=0) maps native window memory top-left while GLES coordinates
remain bottom-left; pbuffers retain their original Y_UP setting.
eglSwapBuffers requires the caller's current draw surface, revalidates window
handle/size, finishes real softpipe rendering, then submits owned pixels through
the existing native MyOS presentation ABI. Stale handles fail instead of copying
to a reused window. The boot framebuffer is fixed; dynamic resize is not claimed.

## Native evidence and impact

Initial BIOS52391 terminal exit0 18:06:25, native-urztpo4h, core-679x6qa0:
PASS two launches/exact resources, duplicate-window rejection, GLSL100/desktop120
rejection, real VBO/texture1024 RGBA oracle, text512 alpha RGBA oracle,
independent QMP frame checks including vertical orientation and upright glyphs,
key input, deferred EGL destruction and relaunch. Fixture16372808 bytes,
256MiB QEMU, zero missing strong imports/no DT_NEEDED/installed runtime.
Log build/renderer-evaluation/native-egl-window.log.
Unlike the earlier pbuffer fixture, window mode does NOT call MyOSUiPresent
directly: both color and glyph frames reach the framebuffer through EGL swaps.
Input currently occurs after the render worker's teardown; do not infer input
while rendering, engine callbacks or >60s responsiveness from this test.

Strict source application and16/16 compile PASS18:09:33 core-m4do_ey7,
egl-window-target-strict-verified.log. Earlier strict-context failures were
patch-context formatting, not native failure; fixed without permitting fuzz.
UEFI window native-2t04ggj1, pbuffer native-_ts99fct and pure-GLES
native-czpkuqbh PASS; session48983 terminal exit0 18:13:05, two launches each
and exact resources. Logs:
native-egl-window-uefi.log, native-egl-pbuffer-after-window.log,
native-gles-after-window.log. Four normal hashes unchanged18:12:21.

Reproduce through tools/run_logged.py:
```
python3 tests/test_native_renderer.py --renderer-probe build/renderer-evaluation/target-config-snob6k4x --configuration build/cxx-target-evaluation/configure-_1j4xqjo --gles --egl-core build/egl-core-evaluation/core-m4do_ey7 --egl-window
```
Add --uefi for UEFI. Isolated artifacts are not installed production libraries.

## Remaining native-engine blockers

Window/context multi-thread ownership, terminate/reinitialize and failed-binding
rollback need adversarial tests; single-buffer semantics and unsupported options
need further audit. No EGL1.5 images/sync/colorspace, native pixmaps, SABI EGL/GLES
function tables, second-module loader/backend integration, Cobalt HTML/JS/DOM
or verified HTTPS/endurance is established. Implement/test these actual contracts;
do not close phases from this renderer fixture or the two imported table names.
Production architecture unchanged; full9925 PASS remains valid (allocator doc).
Normal images unchanged; no fault-image restoration needed.
