# Pinned Mesa C11 source subset

These seven files are pristine copies from Mesa 25.0.7, not the host-patched
Mesa experiment and not approval of Mesa as the native rendering backend.

Upstream archive: https://archive.mesa3d.org/mesa-25.0.7.tar.xz
Archive SHA256: 592272df3cf01e85e7db300c449df5061092574d099da275d19e97ef0510f8a6

Original archive paths and SHA256:

- src/c11/threads.h: 052bd00a9be1cdf1e196cca65782f84e5aa91070f50b0d11f22d5a5c1111f8fc
- src/c11/time.h: 2206f6ba6f339b9574f20c26ad21473f0094cbef4df6ef20c3824c47422039bb
- src/c11/impl/threads_posix.c: 362fb5312e61aafb008075e66a9bff81ad74b2d1c6da3340cdcbd50bc92583b0
- src/util/u_thread.c: 9fbebc6f182851a67f493288dd0a6b5c331bf076d8e7c485a7082058e97235ae
- src/util/u_thread.h: ebd25a27827c27a6035617e29e29b7a4b40b04dcac4c5aaf704fd69343995265
- src/util/macros.h: cb7761079fd529cae22c7c5959573ddccdad7ebc335f362431557f7e0ef19e06
- src/util/detect_os.h: 143d669562560a4613e47beb0e7e06c4dc608e7db156600600a6dccd7e87e7b8

Copyright notices remain in the sources. threads.h and threads_posix.c use
BSL-1.0 (full terms in threads.h); time.h and the util subset use MIT. Full upstream license files
are retained alongside this file. The target modifications are kept separately
in tools/mesa_c11_target.patch and tools/mesa_thread_util_target.patch and applied
only to disposable build copies. tests/test_c11_sdk.py and
tests/test_mesa_thread_util.py verify pristine source hashes before compiling.

MyOS selects HAVE_PTHREAD and HAVE_STRUCT_TIMESPEC, with its own SDK ABI
headers. HAVE_THRD_CREATE and tools/renderer_mesa_host.patch MUST NOT be used
as a substitute for this target port. Compile with -nostdinc/-ffreestanding;
only GCC's compiler intrinsic headers may be added. No Linux libc/C11 objects.
