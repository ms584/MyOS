# Native C++ guard provider checkpoint

Phase 33.1/33.2 OPEN. No renderer or Cobalt execution is claimed.

The reduced-C++ Mesa measurement still imports __cxa_guard_acquire/release.
user/platform-cxx-guard.c provides the x64 Itanium one-time construction ABI
as a statically linkable target SDK object. It is NOT yet part of the fixed
RX platform runtime or the core import register; no import count increases.
The actual native renderer/dependency linkage is still to be implemented.

## Contract and ownership

The compiler's guard is eight writable, aligned bytes. Its first byte changes
only on successful release. Private upper bits encode the current runtime
pthread handle and lock; pointer width is checked, not silently truncated.
Acquire uses an acquire atomic operation, release publishes with release order,
and abort restores zero without claiming construction completed. Contenders
block on the existing compare-before-wait kernel primitive, retry after wake,
and release/abort wake every waiter. A lost wake becomes a failed comparison,
not indefinite sleep. No kernel code or shared build image was modified.

Recursive initialization is undefined C++ behavior; this provider traps in
the owning user process instead of deadlocking or returning fake success.
Release/abort by a nonowner similarly traps. Valid compiler-generated callers
must pair each successful acquire with release or abort. Whole-process cancel
reclaims blocked tasks and guard storage. Individual pthread cancellation,
exception unwinding, C++ TLS destructors and __cxa_atexit are NOT implemented
by this object. It must not be described as a complete C++ runtime.

Primary specification: [Itanium C++ ABI one-time construction](https://itanium-cxx-abi.github.io/cxx-abi/abi.html#once-ctor).

## Evidence, 2026-10-01 +07

- Host compiler-emitted local-static construction, contention/publication and
  explicit abort/retry passed 08:24:11. Link dependencies contain libc only,
  not a fallback libstdc++ guard implementation. Host adapter is test-only.
- Native Ring 3 first probe passed 08:26:20, three launches and exact resources.
- Expanded native probe passed 08:28:46: three waves of four blocked workers
  retry after abort, exactly one initializer publishes data, compiler local
  static initialized once across 200 calls, recursive-init vector 6 contained,
  cancel with four blocked workers, successful relaunch and exact page/heap
  reclamation. Both provider and fixture are in the actual loaded native DSO.
- Repository/path repeat PASS 08:34:22; latest-package UEFI PASS 08:42:33.
  Expanded host abort-with-waiters test also passed 08:33:57.
- Test image was copied with matching pre/post SHA256 and never rebuilt or
  modified; regression's shared image is not used by the isolated guest.
- Every native fixture rejects DT_NEEDED, avoiding host libc/C++ substitution.

Reproduce: python3 tests/test_cxx_guard.py after a current normal build. Optional
--uefi uses a cloned current UEFI package and has scoped evidence above. The
test clones that image into a unique build/cxx-guard-evaluation/native-* folder.
This standalone SDK object is not included in build.sh and does not invalidate
the running native TLS regression image. It still requires target renderer
linkage and broader SDK C11/libc/C++ services, followed by real native GLES and
Cobalt HTML/JS/DOM/input/verified HTTPS >=60s and close/relaunch acceptance.
No Phase 33.3 work. No push.
