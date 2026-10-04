# Phase33.2: real linear/sRGB surfaces and persistent draw/read storage

2026-10-04 (+07), baseline0e5d299. Phases33.1/33.2 OPEN; no33.3/push.
Imports163/242=67.4% supported subset; native Cobalt acceptance0/5.

## Implementation

platform-osmesa-surface.inc uses the actual pinned OSMesa drawable/flush
infrastructure and Gallium resource_create/reference, not host rendered pixels.
Each EGL surface owns a persistent PIPE_TEXTURE_RECT resource in actual
R8G8B8A8_UNORM or R8G8B8A8_SRGB storage. Framebuffer validation returns resource
references instead of allocating a new image for every context. This preserves
surface pixels through context changes, including unshared GL namespaces.
Allocation occurs during surface creation and failures unwind both pixel storage
and drawable. Final-core-reference cleanup destroys actual storage/drawable.

Original state tracker derives sRGB framebuffer capability from the real format.
KHR_gl_colorspace is advertised with original attribute validation/query; no
attribute-only success. MyOS frontend binds distinct draw/read drawables through
original st_api_make_current, preserving original surfaceless behavior too.
Borrowed current_buffer pointers are cleared after successful context switches
and unbind, before EGL binding references can release drawables. Contexts do
not own/free surface drawables. No OSMesa postprocessing is enabled here.

## Native semantic tests and evidence

Strict original-source patches/21 units compile PASS19:37:39 core-_44v3idy:
17 production units and four semantic test units. BIOS linear window
native-mfnu40df PASS20036 exit0 19:39:48, two launches and exact resources.
BIOS sRGB native-fmd_okq3 PASS59056 exit0 19:46:05. UEFI sRGB
native-uk7w_m7a and pbuffer native-r2xvr4l0 PASS78061 exit0 19:51:34;
two launches each and exact resources. This matrix used core-_44v3idy.
Final borrowed-pointer cleanup/assertion compiled21/21 at19:51:39:
core-up2fftbs. BIOS sRGB native-1j_y1dxv PASS71541 exit0 19:54:19,
two launches/exact resources. Final cleanup was not rerun on UEFI/pbuffer;
their earlier matrix remains evidence for unchanged surface/color paths.
All jobs terminal. Four normal image hashes unchanged19:56:42.
Logs: build/renderer-evaluation/native-egl-surface*.log. Individual fixture
directories now retain serial.log and per-launch present/text PPMs so subsequent
tests with the same QEMU name cannot overwrite this evidence.

Tests distinguish GL_LINEAR/GL_SRGB encoding and actual clear pixels:
linear(.5,.5,.5,.5) gives128,128,128,128; sRGB gives188,188,188,128.
Distinct draw/read surface clears cannot alter the read target. An unshared
context sees existing pixels and its changes persist when the prior context
returns. Public surface destruction while current keeps actual blue pixels alive
but subsequent EGL surface queries reject the deleted handle without output writes.
Invalid colorspace is rejected. After switching contexts, original
OSMesaGetColorBuffer on the inactive context rejects its cleared borrowed pointer.
Existing context/debug/surfaceless, image storage,
fences/eight waiters, presentation/input/teardown/resource checks remain.

--egl-srgb enables actual SRGB window storage. Independent gradient oracle uses
the standard sRGB curve on32 known linear sample values, with at most one-byte
shader/rasterization quantization tolerance across1024 pixels. Glyph atlas alpha
blending checks all512 encoded pixels, with independent exact188 QMP/guest
framebuffer checks (linear stays128). Primary red/green/blue/yellow checks remain.
Initial94434 failed19:42:18 because the original gradient oracle expected linear
bytes in an sRGB framebuffer; the failure is retained, not counted as a pass.
No production code was changed to make the framebuffer look linear for the test.

## Remaining contracts

Still EGL1.4: core1.5/OpenCL-absent semantics, exact pinned SABI tables and
actual production module coexistence remain. Broader concurrent ownership,
termination/reinitialization, bind-allocation rollback and exact zero-sized
pbuffer default-framebuffer dimensions need further audit. No certification,
OpenCL, native pixmap or actual Cobalt claim. Full9925 remains valid for unchanged
kernel/runtime architecture. Actual HTML/JS/DOM/input/verifiedHTTPS>=60s/relaunch
gate must still pass; do not close33.1/33.2 on these dependency tests.

Primary spec: https://registry.khronos.org/EGL/extensions/KHR/EGL_KHR_gl_colorspace.txt
Reproduce through tools/run_logged.py with tests/test_native_renderer.py,
--renderer-probe build/renderer-evaluation/target-config-snob6k4x
--configuration build/cxx-target-evaluation/configure-_1j4xqjo
--gles --egl-core build/egl-core-evaluation/core-up2fftbs;
add --egl-window for native windows, --egl-srgb for encoded windows,
and --uefi for UEFI. Default windows remain linear.
