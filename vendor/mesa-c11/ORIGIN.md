# Pinned Mesa C11 source subset

These three files are pristine copies from Mesa 25.0.7, not the host-patched
Mesa experiment and not approval of Mesa as the native rendering backend.

Upstream archive: https://archive.mesa3d.org/mesa-25.0.7.tar.xz
Archive SHA256: 592272df3cf01e85e7db300c449df5061092574d099da275d19e97ef0510f8a6

Original archive paths and SHA256:

- src/c11/threads.h: 052bd00a9be1cdf1e196cca65782f84e5aa91070f50b0d11f22d5a5c1111f8fc
- src/c11/time.h: 2206f6ba6f339b9574f20c26ad21473f0094cbef4df6ef20c3824c47422039bb
- src/c11/impl/threads_posix.c: 362fb5312e61aafb008075e66a9bff81ad74b2d1c6da3340cdcbd50bc92583b0

Copyright notices remain in the sources. threads.h and threads_posix.c use
BSL-1.0 (full terms in threads.h); time.h uses MIT. Full upstream license files
are retained alongside this file. The target modifications are kept separately
in tools/mesa_c11_target.patch and applied only to disposable build copies.
tests/test_c11_sdk.py verifies the pristine source hashes before compiling.

MyOS selects HAVE_PTHREAD and HAVE_STRUCT_TIMESPEC, with its own SDK ABI
headers. HAVE_THRD_CREATE and tools/renderer_mesa_host.patch MUST NOT be used
as a substitute for this target port. Compile with -nostdinc/-ffreestanding;
only GCC's compiler intrinsic headers may be added. No Linux libc/C11 objects.
