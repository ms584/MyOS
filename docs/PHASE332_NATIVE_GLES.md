# Phase33.2 genuine GLES context feasibility (2026-10-04)

Baseline524c75c. No EGL/SABI/backend selection or Cobalt acceptance claim.
No Phase33.3 and no push.

## Native implementation

Mesa25.0.7's OSMesa frontend originally selects only desktop GL profiles.
tools/mesa_gles_target.patch adds a MyOS-only private profile constant0x10000
and routes it to Mesa's original API_OPENGLES2 state tracker. This is a private
offscreen frontend extension, NOT a claim that standard OSMesa provides EGL.
No version-string replacement, fake shader compiler or GL-to-GLES rename.
Existing softpipe, dispatch, shader compilation, API validation and drawing
remain the actual Mesa implementations; other platforms/profiles unchanged.

tests/test_native_renderer.py --gles selects that profile with requested2.0.
Actual Mesa reports OpenGL ES3.1; requested2.0 is not a forced upper limit.
The fixture verifies the ES version prefix, actual rejection of desktop
GLSL120, successful GLSL ES100 with fragment precision, blue clear, VBO and
textures, all1024 checker-gradient pixels/cache eviction/reupload, QMP frame,
real key input, TLS isolation, context teardown and two same-VM launches with
exact resources. This is not GLES conformance or all ES3.1 feature coverage.

## Evidence and pending work (+07)

- Profile build PASS: two actual frontend compile/archive steps in
  build/renderer-evaluation/gles-profile-build.log.
- First36305 terminated during a WSL E_UNEXPECTED interruption; no PASS.
  native-g7l8d454/log native-gles.log stops at context begin. Authoritative
  process inspection confirmed no test/QEMU remained before retry.
- Retry67924 exit0 16:19:51: BIOS native-r99292bp, all native checks/two
  launches/exact resources PASS. Log native-gles-retry.log.
- GLES UEFI native-3tgetrcy PASS in native-gles-uefi.log; two launches/exact
  resources. Pipeline80368 later exit1 after another WSL interruption during
  desktop BIOS native-nnc7xtwg; desktop UEFI not started. No native failure
  assertion observed; WSL dmesg shows systemd shutdown/unmount/remount, not
  evidence of a MyOS fault. No old test/QEMU remained at16:23:42.
- Desktop BIOS native-4f041htj PASS in native-desktop-after-gles-retry.log.
  Pipeline55606 later interrupted during UEFI native-h2j_46kt after shader/
  pixel checks, before teardown. No PASS claimed for that UEFI run.
- Standalone desktop UEFI91407 terminal exit0 16:28:04, native-_lm9a14j,
  two launches/all checks/exact resources PASS; log
  native-desktop-after-gles-uefi-retry.log. All jobs now terminal; no owner.
  Private frozen VMs; no kernel/runtime/image changes.
- Four normal hashes unchanged16:19:21 (PHASE331_ALLOCATOR.md).
  Mandatory full9925 remains valid for unchanged production architecture.

Reproduce through tools/run_logged.py:
python3 tests/test_native_renderer.py --renderer-probe
build/renderer-evaluation/target-config-snob6k4x --configuration
build/cxx-target-evaluation/configure-_1j4xqjo --gles [--uefi]

Next: native text,
real EGL ownership/surfaces/thread/error contracts, full pinned GLES table,
then production renderer loading and SABI integration. Current single-module
loader cannot load Mesa as a second dependency; module design remains open.
Kernel test image has72-byte headroom before adding embedded runtime code.
Imports163/242=67.4% subset only; actual Cobalt0/5. Keep33.1/33.2 OPEN.
