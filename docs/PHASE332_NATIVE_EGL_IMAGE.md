# Phase33.2: native EGL image resource sharing

2026-10-04 (+07), baseline db934b7. Phases33.1/33.2 OPEN; no33.3/push.
Imports163/242=67.4% supported subset; actual Cobalt acceptance0/5.

## Implementation and boundary

platform-osmesa-image.inc is compiled inside the actual pinned/MyOS-configured
OSMesa frontend TU, not a second renderer or a host Linux image service.
Original texture/renderbuffer lookup and completeness checks resolve actual
Gallium resources. st_finalize_texture materializes uploaded texture storage
before the first draw. Image ownership uses pipe_resource_reference; imported
GL siblings acquire their own resource references, not copied pixel buffers.
The frontend installs real get_egl_image/validate_egl_image hooks before context
creation. Driver lookup uses the current EGL display's locked resource registry,
never blindly dereferencing an application-provided image handle.
EGL image resources keep display lifetime and free actual storage on the final
core reference, including a core pin surviving public handle destruction.

KHR image_base/2D/cube/3D/renderbuffer entry points are implemented; native
pixmaps, dma-buf, OpenCL and complete EGL1.5 are not claimed. Mesa's texture
import rejects a resource whose pipe target differs from the GL texture target:
cube faces/3D slices are tested through real renderbuffer imports instead.
This is an explicit backend boundary, not proof of arbitrary image conversion.

## Native evidence

Strict original-source patch/19 units compile PASS19:00:27 core-9_i665c0:
17 production units plus lifetime/image semantic tests.
BIOS window native-txd7rze3 PASS75806 exit0 19:02:06, two launches,
exact resource reclamation, original shader/texture/text/input/frame checks.
UEFI window native-c239uyti/pbuffer native-4tf764wh PASS84043 exit0
19:05:20, two launches each; serial image success marker twice in each.
Logs: build/renderer-evaluation/native-egl-image*.log.

Tests cover an unshared consumer context reading producer pixels, then writing
cyan into imported storage and reading cyan through the original producer.
Deleting source GL objects and the EGL image leaves the imported sibling usable.
All six cube faces, nonzero 3D slice, renderbuffer blue pixels, invalid context,
zero name, negative level, zoffset exactly Depth, stale handle/double destroy,
and actual last-core-pin destructor are checked. Eight fence waiters and original
EGL deferred-destruction/presentation regression remain in the same fixture.

Initial BIOS90554 failed18:58:30 when cube storage was imported as texture2D;
the actual Mesa target check rejects this. Tests now use the real renderbuffer
layer import and preserve the 2D cross-context write-back test. A preceding
invocation failed before QEMU because of an incorrect configuration path;
neither failed run is recorded as a pass. No production kernel/runtime change.

## Remaining work

Source-context death with surviving images, concurrent ownership/termination,
full attribute/mipmap/error matrix and backend target limits need further audit.
Pinned Starboard requires EGL1.5: context/colorspace/surfaceless/core contracts,
exact SABI tables and production module coexistence remain. Native Cobalt itself
has not executed; HTML/JS/DOM/input/verifiedHTTPS>=60s/relaunch still required.
Full9925 remains valid for unchanged kernel/runtime architecture; normal image
hashes unchanged19:05:55; all test jobs terminal, no image owner.
Do not count this dependency slice as phase closure.

Reproduce through tools/run_logged.py with tests/test_native_renderer.py,
--renderer-probe build/renderer-evaluation/target-config-snob6k4x
--configuration build/cxx-target-evaluation/configure-_1j4xqjo
--gles --egl-core build/egl-core-evaluation/core-9_i665c0;
add --egl-window for native window and --uefi for UEFI.
