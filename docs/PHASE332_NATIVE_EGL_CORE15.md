# Phase33.2: genuine EGL core1.5 entry points, no OpenCL provider

2026-10-04 (+07), baselinef11758f. Phase33.1/33.2 OPEN; no33.3/push.
Imports163/242 (67.4% fixture-supported subset), actual Cobalt0/5.

## Changes and boundaries

All previously measured version gates remain: real Gallium fences/server waits,
image storage, requested contexts/debug, sRGB, surfaceless and original procedure
lookup. Original Mesa ties its version calculation to KHR_cl_event2. The MyOS
adapter has no OpenCL provider or valid OpenCL events; core section3.8.1 explicitly
permits unsupported sync types to return BAD_PARAMETER. MyOS recognizes that
absence without advertising KHR_cl_event2 or fabricating a successful CL fence.
The version calculation remains conditional on all other original feature gates.
It now derives1.5 for the implemented MyOS/surfaceless driver, not other platforms.
No conformance/certification, OpenCL or complete Cobalt renderer claim follows.

Original _eglCreateSync used the CL extension as a proxy for both core EGLAttrib
and KHR64 calls. The scoped patch permits core calls when version>=15 while
keeping unavailable KHR64 rejected. Existing caller-specific error policy separates
core from KHR64; no weakening of the extension query or unsupported type checks.

Source audit also found eglCreateImage's legacy converter truncates64-bit names.
MyOS core image validation rejects out-of-range names with BAD_PARAMETER before
they can alias valid EGL_IMAGE_PRESERVED. Other platform/converter attribute
matrices, image numeric value ranges and generic configuration contracts still
need audit. This is not proof of every API/attribute combination.
Also audit eglQueryContext(CLIENT_VERSION): the original core returns the driver
base's requested version, while OSMesa may create a higher compatible version.
The driver must expose the actual supported context version rather than assume
that matching requested values prove the query contract (spec section3.7.4).

## Native tests

user/egl-core15-test.c runs inside MyOS, not on host EGL. It checks:
- Derived1.5 string; CL extension absent; CL requests with missing or high-bit
  fake event rejected as unsupported. KHR64 remains unavailable.
- Core debug/robust-false context attributes and actual GL debug state.
- Core fence with no current context rejected; invalid/wide sync attributes
  rejected. Real cyan FBO writes queued before fence, client/server waits,
  exact RGBA pixels,64-bit output slots and preserved error outputs.
- Real core EGLImage from texture; wide name cannot alias IMAGE_PRESERVED;
  image/sync deletion and stale handle errors; actual object cleanup.
- Existing image/surface/debug/fence/eight-waiter/window/glyph/resource tests
  and two complete opens remain in the native fixture.

Initial core-yuo5z7oo22/22 compile PASS20:02:23. BIOSsRGB jnab46_e
PASS57800 exit0 20:05:08, two launches/exact resources. This preceded the
wide-image-name guard/assertion and new required harness marker.
Final core-ogu957sr22/22 strict compile PASS20:10:18.
Final BIOSsRGB native-4c8gy3yq, UEFIsRGB native-asqas4sd and pbuffer
native-4rrsjzfq PASS44277 terminal exit0 20:17:38, two launches/exact resources
each; all include the wide-image guard and required core15 marker. No live owner.
Four normal image hashes unchanged20:17:08. Fetch20:17:08 confirms origin/main
db934b7; no push or merge. Query-version/other audits above remain open.
Strict patch failures final/final2/final3 retained; corrected context/order,
fuzz remains0. Logs: build/renderer-evaluation/egl-core15-target*.log and
native-egl-core15*.log; fixtures preserve serial and QMP PPMs.

Full9925 remains valid for unchanged kernel/runtime. Normal production images
are not modified by these isolated libraries/fixtures. Required next: complete
pinned44-entry EGL and GLES tables with ABI validation and genuine native calls,
provider/module coexistence in the actual loader, remaining79 imports/weakTLS,
then Cobalt HTML/JS/DOM/input/verifiedHTTPS>=60s and close/relaunch acceptance.
Native fixture input is after rendering teardown, not concurrent engine input.

Specification: https://registry.khronos.org/EGL/specs/eglspec.1.5.pdf section3.8.1.
Reproduce via tools/run_logged.py and tests/test_native_renderer.py using
--renderer-probe build/renderer-evaluation/target-config-snob6k4x
--configuration build/cxx-target-evaluation/configure-_1j4xqjo --gles
--egl-core build/egl-core-evaluation/core-ogu957sr; add --egl-window --egl-srgb
for native encoded windows and --uefi for UEFI. Default remains linear.
