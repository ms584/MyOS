# Phase 33.2 renderer feasibility checkpoint

2026-10-01, runtime baseline 8003d20. Native backend NOT chosen or passed.
Cobalt has not executed. Phase 33.1/33.2 remain OPEN; no Phase 33.3 work.

## Pinned boundary, not two graphics imports

The pinned Starboard headers require EGL 1.5 and contain 44 EGL table slots
(352 bytes) and 246 GLES slots (1968 bytes), pointer alignment 8 on x64.
`tools/measure_renderer.py` verifies their hashes against
PHASE331_ABI_SOURCES.json and inventories ordered names/offsets. These are ABI
requirements, not implemented entry points or proof that every slot is used.
Table inventory does not replace context/surface/thread/extension semantics.

Source: Cobalt 3befd332486952f6603ae3baf2a8713878d6e200,
[EGL](https://github.com/youtube/cobalt/blob/3befd332486952f6603ae3baf2a8713878d6e200/starboard/egl.h),
[GLES](https://github.com/youtube/cobalt/blob/3befd332486952f6603ae3baf2a8713878d6e200/starboard/gles.h).

## Measured softpipe candidate (HOST ONLY)

Mesa 25.0.7 was downloaded from the official archive, not installed system-wide.
Archive SHA256: 592272df3cf01e85e7db300c449df5061092574d099da275d19e97ef0510f8a6,
matching the [release checksum](https://docs.mesa3d.org/relnotes/25.0.7.html).
This older pinned release is a build spike, not a production/security selection.
Source/licenses remain in build/renderer-evaluation/mesa-25.0.7; no vendoring
or distribution of this candidate has been approved by a native test.

The initial Linux EGL configuration required libdrm despite using softpipe,
no X11/Wayland/GBM, and no Vulkan/LLVM/video codecs. This is the Linux DRI/EGL
platform requirement, not proof that a new MyOS software backend needs DRM.
Mesa's [EGL design](https://docs.mesa3d.org/egl.html) separates its dispatcher
from platform/driver hooks. MyOS does not currently supply Linux DRM ioctls.

The isolated OSMesa core built successfully at 03:06 +07. It avoids the EGL
DRI dependency and exercises the rasterizer, but OSMesa exposes desktop GL
contexts, not the required native Starboard EGL/GLES window contract.
The host probe at 03:07/03:12 used the built library (not system Mesa):

- GL renderer: softpipe; desktop GL 3.3 compatibility; GLSL 3.30.
- Compiled/linked GLSL 120 shaders; textured quad produced four exact RGBA
  quadrant samples. A known glyph mask exercised sampled alpha/blending;
  all 64 sampled glyph cells and output-buffer guards passed.
- No Cobalt, DOM/font shaping, input, native present or HTTPS was tested.
- File: 17854872 bytes. LOAD memory sum: 16952601; address span: 16956776.
- DT_NEEDED: libstdc++.so.6, libm.so.6, libgcc_s.so.1, libc.so.6.
- PT_TLS: 8 file bytes/32 memory bytes, alignment 8. Relocations include
  R_X86_64_TPOFF64 (2), TLSDESC (1), plus supported non-TLS classes.
- Host-only 64x64 probe: 0.04 seconds elapsed, 77452 KiB peak RSS. These are
  WSL measurements, NOT native engine RAM/startup/performance requirements.

The Linux binary is not loadable unchanged by MyOS: kernel/elf.c rejects PT_TLS;
dependency loading, symbol versioning and those TLS relocations also require
real contracts. Emulated TLS already tested for Cobalt is not native ELF TLS.
Linux C11/pthread types are not Cobalt's opaque wrapper types either.
Do not accept host library success as a native context/present gate.

## Reproduction and tooling

All commands were run through tools/run_logged.py. Isolated source/build/tool
directories are under build/renderer-evaluation. Python tooling: Meson 1.8.1,
Ninja 1.11.1.4, Mako 1.3.10, packaging 25.0, PyYAML 6.0.3, setuptools 80.9.0,
MarkupSafe 3.0.3. Bison/flex/m4 were extracted from Ubuntu debs into toolroot;
no packages were installed globally. GCC/G++ 15.2.0, binutils 2.46.

Mesa 25's pthread C11 emulation conflicts with glibc 2.43 stdlib's once_flag.
tools/renderer_mesa_host.patch selects host C11 threads and bridges the two
monotonic-condition calls with size/alignment assertions. Glibc's actual
[mtx_lock source](https://github.com/bminor/glibc/blob/master/sysdeps/pthread/mtx_lock.c)
uses pthread_mutex_t storage. This patch is explicitly host-only, not a MyOS
thread ABI adaptation; it must not silently become the target configuration.

Core configuration (prefix PATH with venv/bin and toolroot/usr/bin; set
BISON_PKGDATADIR=toolroot/usr/share/bison and M4=toolroot/usr/bin/m4):

```sh
meson setup build/renderer-evaluation/mesa-osmesa-host \
  build/renderer-evaluation/mesa-25.0.7 --buildtype=release \
  -Dc_std=c11 -Dcpp_std=c++17 -Dgallium-drivers=softpipe \
  -Dvulkan-drivers=[] -Dplatforms=[] -Dglx=disabled -Degl=disabled \
  -Dosmesa=true -Dgles1=disabled -Dgles2=enabled -Dllvm=disabled \
  -Dgbm=disabled -Dgallium-va=disabled -Dgallium-vdpau=disabled \
  -Dvideo-codecs=[] -Dglvnd=disabled -Dbuild-tests=false \
  -Dshader-cache=disabled -Dzlib=disabled -Dzstd=disabled \
  -Dexpat=disabled -Dxmlconfig=disabled -Dlibunwind=disabled -Dvalgrind=disabled
ninja -C build/renderer-evaluation/mesa-osmesa-host -j2
python3 tools/measure_renderer.py \
  --library build/renderer-evaluation/mesa-osmesa-host/src/gallium/targets/osmesa/libOSMesa.so.8.0.0 \
  --headers build/engine-evaluation/contracts \
  --source-archive build/renderer-evaluation/mesa-25.0.7.tar.xz \
  --output build/renderer-evaluation/report.json
python3 tests/test_renderer_audit.py
```

Compile tools/renderer_host_probe.c with the pinned Mesa include directory,
the built OSMesa library directory and an absolute matching rpath, -lOSMesa,
-std=c11 -Wall -Wextra -Werror -O2. Run with GALLIUM_DRIVER=softpipe and
LIBGL_ALWAYS_SOFTWARE=1. The probe prints HOST ONLY and its measured renderer.
Static inventory tests pass comments/order/offsets, ambiguity rejection and
ELF TLS/truncation reporting. Neither tool can mark native_gate_proven true.

## Next native decision gates

1. Choose a target toolchain/ABI strategy and measure its real closure. Either
   supply tested native TLS/dependencies or compile against MyOS-compatible
   libc/pthread/emutls interfaces. No Linux loader/syscall substitution.
2. Prototype a native software winsys/context backend using Mesa's state
   tracker, or measure ANGLE/SwiftShader's two-port closure before choosing it.
   OSMesa desktop GL alone is not a Starboard EGL/GLES implementation.
3. Implement owned native surfaces/framebuffer present, context thread ownership,
   shared textures, swap/error/extension behavior and all required table slots.
4. Native context/present/texture/text, cleanup/relaunch and measured RAM/CPU
   must pass before backend selection. Then finish window/event/input plumbing
   and run Cobalt's actual HTML/JS/DOM/input/verified HTTPS >=60s gate.

Production sources/image were unchanged by this spike. The runtime baseline's
full system/UEFI regressions remain valid; no duplicate full run is claimed.
