# Phase 33.2 renderer feasibility checkpoint

2026-10-01, runtime baseline 8003d20. Native backend NOT chosen or passed.
Cobalt has not executed. Phase 33.1/33.2 remain OPEN; no Phase 33.3 work.

Native TLS follow-up, 07:57 +07: GD/LD/IE/TLSDESC and PT_TLS now have native
single-module fixture evidence, including constructor templates and pthread
ownership: PHASE331_NATIVE_TLS.md. Resumed full coverage/UEFI passed 08:44:17;
the earlier unchanged passes were retained after a 40s boot timeout.
The older host-spike statement that PT_TLS is rejected describes that baseline.
The Linux candidate still lacks target libc/C11/C++ and dependency closure;
this change does NOT make that library loadable or select a native backend.

## MyOS target configuration and first real compile — 12:10 +07

Follow-up math/blob checkpoint (after 95580ce): the cross-probe now receives
math-only headers and all 64 actual math objects plus SDK string providers.
No full musl header tree or host libc/C++ is exposed. math.h and BLAKE3/blob
compile failures are closed; this does not mean a final renderer has linked.
Full target configure/compile 53882 finished 12:31:59 in
build/renderer-evaluation/target-config-8gnsdabk: setup 0, compile 1. Observed
remaining frontier: sys/types.h, fcntl.h, inttypes.h, stdio.h, localtime_r.
C/C++ math-header/sqrt-provider link, pinned types/no Linux ABI and forbidden
library routes passed 12:38:46 (compile/link only). PHASE332_MATH_SDK.md records
the new exported-header native numeric test, not a graphics/backend gate.

Actual pinned src/util/blob.c is also native-tested via tests/test_mesa_blob.py.
The SDK now declares its existing realloc provider and implements memchr.
Target patches reject addition overflow, saturate capacity doubling, preserve
buffer ownership on failed trimming realloc and avoid signed count comparison
after the reader's pointer-order check. BIOS 44517 passed 12:37:38: growth,
aligned typed/string reads, fixed-size guards, truncated input, SIZE_MAX reserve,
real allocator exhaustion including one-byte allocation, failed-trim ownership
and three launches/exact page/heap recovery. Earlier small-allocation pressure
was insufficient and failed; only the corrected terminal run is a PASS.
The final signed reader comparison was compiled by this native fixture after
the full cross-build run. No new UEFI/full regression: kernel/runtime/images
unchanged. Headers can be reused only after comparison against the sealed musl
archive and current MyOS math-types source; no repeated math build is needed.
Reproduce: python3 tools/run_logged.py -- python3 tests/test_mesa_blob.py
[--math-sdk build/math-evaluation/target-qrhf88ei]. The fixture is an actual
renderer utility consumer, not rendering, EGL/GLES, shader execution or Cobalt.

tools/probe_renderer_target.py now configures a freshly extracted, hash-verified
Mesa 25.0.7 source against MyOS rather than measuring the Linux binary again.
Only mesa_c11_target.patch, mesa_thread_util_target.patch and the new
mesa_build_target.patch are applied; the glibc host patch is never applied.
The compiler boundary disables implicit C/C++ host headers and default system
libraries, undefines Linux macros and links configure probes to a fresh archive
of actual platform-runtime.c/platform-tls.asm providers. No syscall stub is used.
No host target execution or automatic dependency download is permitted.

The selected *probe configuration*, not a selected production backend, remains
in-process softpipe OSMesa, EGL/GLX/GBM/Vulkan/LLVM/codecs disabled. The MyOS patch
skips libdl only for this explicitly restricted configuration: upstream u_dl.c
already returns unavailable on a non-POSIX/non-Windows target. It does not claim
dlopen/driver-plugin support. OpenMP detection is disabled because compiler flag
acceptance cannot prove a MyOS OpenMP runtime. Byte order uses actual compiler
target constants instead of glibc endian headers. Linux dependencies are not
renamed or supplied with fake success functions.

Meson setup passed. The real ninja compile now passes C11 sources and the first
BLAKE3 memset failure, but still fails on math.h, sys/types.h, fcntl.h,
inttypes.h and the os_time.h localtime_r declaration. This is an observed first
closure frontier, not an exhaustive list or evidence of a complete renderer.
The existing native-tested math prototype still needs header/library integration;
C++ containers/target standard-library configuration also remain unresolved.
Do not use host glibc-configured libstdc++ OS/gthread headers with Cobalt opaque
pthread types. EGL/GLES context/present and native Cobalt remain unproven.

Full configure/compile logs and report: build/renderer-evaluation/
target-config-ad2v00f_/. Pipeline 97415 finished 12:10:01: configure_exit=0,
compile_exit=1. tools/renderer_target_compiler.py rejects library/search/response
escape routes; tests/test_renderer_target_compiler.py passed at 12:12:42 with
the real runtime archive (link only, no DT_NEEDED/undefined, ABI sizes/macros).
New SDK memset uses actual byte writes. Affected native Mesa-util BIOS 13315
passed at 12:08:45 with integer-to-byte conversion, zero length, unaligned spans
and buffer guards, plus existing thread/cancel/relaunch/resources. UEFI was not
rerun for this SDK change. Shared normal images/kernel/runtime are unchanged;
the fresh full regression is retained, not rerun or mislabeled as a new result.

Reproduce: python3 tools/run_logged.py -- python3 tools/probe_renderer_target.py
--compile. The current command correctly exits nonzero for the unresolved source
closure; this must not be reported as a graphics PASS. No Phase 33.3 or push.

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

### Reduced-C++ host measurement, 08:17 +07

An independent build with `cpp_rtti=false`, `cpp_eh=none`, fortify disabled
and stack protector disabled completed 08:14:21. The unchanged desktop-GL
context/shader/texture/glyph/guard probe passed 08:17:49; `ldd` verified it used
this separate candidate library, not the system/earlier OSMesa build.
File/LOAD/span: 17776680/16883393/16895208 bytes. Strong undefined symbols
decreased 186 -> 176. RTTI vtables, exception personality/unwind continuation
and fortified wrappers disappeared, but all four DT_NEEDED libraries remain.
Still required: prime-rehash policy, std bad-alloc/length/system-error helpers,
new/delete (including sized delete), __cxa_atexit and guard acquire/release.
Flags are a measurement, NOT an implementation of these contracts or permission
to turn allocation/errors into fake success. Report: cpp-report.json under
build/renderer-evaluation; isolated build: mesa-osmesa-cpp-measure.
The source explicitly selects only API_OPENGL_CORE/COMPAT in OSMesaCreateContext;
GLES-enabled compilation does not make this probe a GLES/EGL context.
Standalone target SDK guard provider now has native contention/abort/cancel
evidence: PHASE332_CXX_GUARD.md. It is not yet linked into this renderer and
does not change the measured Linux binary's imports. Target libc/C11/C++ and
real native GLES frontend/presentation remain required.
No native backend selection or Cobalt acceptance follows from these results.

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

### Additional source/closure inspection, 07:03 +07

The pinned core exports memcpy/memset, unwind entry points and some C++
operators (_Znwm, _Znam and _ZdaPv). A targeted check at 07:05 found those
operators, but not Mesa's _ZdlPvm, __cxa_guard_acquire/release, __cxa_throw,
__gxx_personality_v0 or class_type_info vtable. The host Mesa library's
undefined symbols include GLIBCXX/CXXABI and versioned glibc fortified stdio
functions. Its full dependency closure cannot be replaced by a nominal name
match to Cobalt or by
binding the pinned musl-opaque pthread boundary to host glibc's C11 types.
No Clang/Clang++ executable was found on PATH; emulated-TLS compilation is
therefore a toolchain task, not an available/proven build setting.

Mesa 25.0.7 src/mapi/glapi/glapi.h reads thread-local dispatch/context directly;
src/mapi/u_current.c defines those variables, and src/util/u_thread.h maps
__THREAD_INITIAL_EXEC to thread_local (initial-exec on the applicable target).
This source does not provide a USE_ELF_TLS=0 shortcut to a complete threaded
MyOS backend. A source adaptation must preserve per-thread state; a shared
global context would be incorrect. Compiler emutls also requires auditing
generated dispatch/assembly, not just C/C++ relocations.

Current MyOS errno/pthread state uses fixed private pages independently of FS.
SYS_THREAD_CREATE accepts a validated caller-supplied FS base; the existing
pthread_create passes zero, not its errno page. Native x64 TLS must preserve
this private-page identity while installing a real TCB and module templates/
offsets, including main-thread and worker lifetime. Static inspection does
not prove this design.
These observations narrow the next target-toolchain/TLS/dependency slice;
they do not select a renderer or demonstrate a native context.

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
