# Phase 33.2: genuine Mesa EGL core and MyOS pbuffer/context driver

Date: 2026-10-04 (+07). Baseline bdab91c. Phases33.1/33.2 remain OPEN.
Imports163/242=67.4% tested subset; Cobalt acceptance0/5. No33.3 or push.

## Implementation and supported boundary

`tools/probe_egl_core_target.py` uses the sealed Mesa25.0.7 source and its
existing MyOS compiler/configuration, never host EGL/GL/libc. It compiles
13 original EGL main units plus original util/rwlock.c; --driver also includes
user/platform-egl-driver.c and original util/os_file.c (16 units total).
The compile report deliberately retains native_egl_passed=false: compiling
an archive does not prove that it executes. Native evidence is separate below.

The driver uses original EGL handles/error state/config parsing/reference
management, and genuine OSMesa API_OPENGLES2 contexts and pixel storage.
Only surfaceless display, ES2 requested contexts and RGBA8 pbuffers are exposed.
EGL core reports1.4, not1.5; configs claim no certified conformance. No image,
sync, colorspace or surfaceless-context extension flags are falsely enabled.
Pbuffers are bounded1024x1024; zero-sized query dimensions remain exact despite
minimum1x1 backing needed by OSMesa. EGL_LARGEST_PBUFFER clamps to config limits.
Separate draw/read surfaces and unsupported context attributes are rejected.
Contexts retain display/config lifetime; destroy of a bound resource defers
storage release until unbind. Texture pbuffers are rejected EGL_BAD_MATCH.
BindTexImage/ReleaseTexImage use original validation (binding no-texture fails,
release of an unbound image is a defined no-op); CopyBuffers rejects unsupported
native pixmaps. The core otherwise calls these callbacks without NULL guards.

## Evidence and reproduction

Compiler-only14/14: core-rgbzublh, egl-core-target-final.log,16:44:44.
Initial driver16/16: core-y9it32b6, egl-driver-target.log,17:45:24.
Initial BIOS native-azt9j2al PASS17:46:45; UEFI native-3a0fi2pr PASS17:52:31.
Logs build/renderer-evaluation/native-egl-pbuffer.log and
native-egl-pbuffer-uefi.log. Each run uses an isolated frozen normal image,
256MiB QEMU, two launches and exact resource reclamation. Linked renderer
16364464 bytes, no DT_NEEDED, actual installed runtime imports, not the
configure-only fake TLS runtime archive.

Checks cover original EGL initialization/config/context/pbuffer/current/error
paths, thread-local state, invalid display/config/resource handles and deferred
destruction. Genuine GLES/GLSL100 rejects desktop120, VBO/texture1024 RGBA oracle,
glyph alpha512 RGBA oracle, independent QMP RGB frames, key input and teardown
all pass. UI presentation here still uses MyOSUiPresent, NOT eglSwapBuffers
of a native window. Pbuffer swap is a flush, not a native framebuffer swap.

Guard follow-up16/16 compile PASS core-afep6e8u17:54:45.
Expanded BIOS native-iybdvjdi and UEFI native-fz90ijff PASS, plus affected
pure-GLES native-9f0wh9tu PASS. Combined session82634 terminal exit0 17:58:02;
logs native-egl-guards.log, native-egl-guards-uefi.log, native-gles-after-egl.log.
All use two launches/exact resources. Expanded checks additionally exercise
texture-pbuffer rejection, BindTexImage failure/unbound release no-op,
CopyBuffers rejection, over-limit pbuffer failure and largest-pbuffer clamp.

Reproduce via tools/run_logged.py, then:
```
python3 tools/probe_egl_core_target.py --renderer-probe build/renderer-evaluation/target-config-snob6k4x --driver
python3 tests/test_native_renderer.py --renderer-probe build/renderer-evaluation/target-config-snob6k4x --configuration build/cxx-target-evaluation/configure-_1j4xqjo --gles --egl-core build/egl-core-evaluation/core-afep6e8u
```
Add --uefi for UEFI. Probe paths are isolated evidence artifacts, not installed
production libraries. Fresh probe output paths must replace the example path.

## Remaining work / regression impact

No native EGL window/pixmap, SABI EGL/GLES table, loader module coexistence,
actual Cobalt rendering/text/HTML/JavaScript/DOM or HTTPS gate is established.
Mesa core explicitly rejects native windows on SURFACELESS: implement a genuine
MyOS native platform and owned window surfaces, framebuffer orientation/swap,
context/thread ownership and safe resize/close, not rename a surfaceless API.
Audit multi-thread ownership/terminate/reinitialize and failed binding rollback
before expanding claims. Essential1.5 contracts need real implementation before
advertising them. Integrate required exact pinned SABI function tables next.

Kernel/runtime production code is unchanged; previous full9925 PASS16:08:50
remains architecture evidence. Current tests affect isolated renderer artifacts
and fixture only, so scoped native regression is required, not a redundant full
kernel suite. Four normal hashes unchanged17:55:55; no fault image to restore.
See PHASE331_ALLOCATOR.md for those hashes and full-suite evidence.
