# Phase33.2 native glyph-atlas feasibility (2026-10-04)

Baseline d43c15d. This is a rendering dependency, NOT Cobalt/DOM/font layout
or phase acceptance. No Phase33.3, no push.

## What executes in MyOS

The existing licensed assets/myos-font.bin is linked as read-only glyph data
through user/renderer-font.asm. SHA256 is checked before linking:
38cea9db0e28a9458cad63a57050167cbb87c94f2f5805d097f7cfee871bda94.
The font is not regenerated; assets/FONT-LICENSE.txt remains authoritative.

The native worker expands the four 8x16 masks for MyOS into an RGBA atlas.
Actual Mesa GLSL sampling/VBO drawing/SRC_ALPHA blending renders it over
opaque black. Ink alpha128 deliberately exercises compositing, not a direct
opaque framebuffer copy. Separate alpha factors preserve opaque output alpha.
All512 RGBA pixels are checked against a differently indexed glyph-bit oracle
in MyOS. Ink-count bounds require both foreground and transparent background.
The original four-color image is presented first, then text at688,552.
QEMU frame checks independently compare all512 text RGB pixels with the
pinned static font masks. Host data inspection is an oracle only; glyph
expansion, shader work, blending and presentation execute in MyOS.

Original GLSL/textures/cache invalidation/TLS/input/teardown/two launches/exact
resource checks remain required. Production kernel/runtime/images unchanged.
This does NOT implement Unicode shaping, TrueType rasterization, font fallback,
Cobalt Skia text layout, EGL surfaces or the SbGetGlesInterface function table.

## Evidence (+07)

- BIOS GLES50163 terminal exit0 16:33:45, native-kepg1ao7: all512 text pixels,
  QMP text frames, original renderer checks/two launches/exact resources PASS.
  build/renderer-evaluation/native-gles-text.log.
- GLES UEFI61812 terminal exit0 16:35:10, native-d5cj8na9: two launches,
  all512 text pixels/QMP frames/input/teardown/exact resources PASS;
  native-gles-text-uefi.log. Desktop GL63417 exit0 16:36:53, native-cpoxl897,
  same checks/two launches PASS; native-desktop-text.log. No live image owner.
- Four normal hashes unchanged16:38:14. No runtime/kernel architecture change.
- Full9925 remains valid for unchanged production architecture; normal hashes
  in PHASE331_ALLOCATOR.md. Current fixture changes require affected checks,
  not a new unrelated architectural full suite.

Reproduce via tools/run_logged.py:
python3 tests/test_native_renderer.py --renderer-probe
build/renderer-evaluation/target-config-snob6k4x --configuration
build/cxx-target-evaluation/configure-_1j4xqjo [--gles] [--uefi]

Next: implement actual EGL ownership,
surfaces/thread/error contracts and full pinned GLES table, production module
loading/SABI/resource/font integration, then Cobalt HTML/JS/DOM/input/HTTPS
interactive >=60s and close/relaunch. Imports163/242=67.4%, Cobalt0/5; phases OPEN.
