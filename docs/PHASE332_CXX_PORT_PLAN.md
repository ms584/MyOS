# Phase 33.2 real target C++ dependency plan

2026-10-04, after e12f51b; source preparation/configure diagnostics only.
No C++ library built, installed, renderer-linked or native-tested by this plan.
Memory/loader full93613 exit0 10:55:33; normal BIOS/UEFI restored and ownership
released. Evidence/hashes: PHASE332_OS_INFO.md.
Phase33.1/33.2 remain OPEN; no33.3 or push.

## Pinned source and observed constraints

Current compiler reports GCC15.2.0. The official gcc-15.2.0.tar.xz was downloaded
to build/cxx-target-evaluation/, without installing libraries or changing SDK.
Download89279 exit0 10:26:04+07. SHA512 matches the official release manifest:
```
89047a2e07bd9da265b507b516ed3635adb17491c7f4f67cf090f0bd5b3fc7f2ee6e4cc4008beef7ca884b6b71dffe2bb652b21f01a702e17b468cca2d10b2de
```
[Source archive](https://gcc.gnu.org/pub/gcc/releases/gcc-15.2.0/gcc-15.2.0.tar.xz),
[upstream checksum](https://gcc.gnu.org/pub/gcc/releases/gcc-15.2.0/sha512.sum).
Only libstdc++-v3, gthread source, configuration helpers and license files were
extracted to build/cxx-target-evaluation/upstream/gcc-15.2.0/. Total preparation
directory217MiB (archive included). No full compiler build or source vendoring.
Retain COPYING3/COPYING.RUNTIME and audit redistribution separately.

Observed canonical-target probe10:28:06 rejects x86_64-unknown-myos: unknown OS.
configure.host otherwise has generic OS/C-library defaults, not a MyOS port.
requires_hosted.h rejects <mutex> when _GLIBCXX_HOSTED is false. Current target
compiler uses -ffreestanding for both C/C++; blindly adding include paths would
not supply the required C++ concurrency environment. Do not redefine Linux,
borrow glibc c++config.h/os_defines.h or bypass the hosted-header guard.
Use a deliberate target C++ compilation/library profile with hosted facilities,
while retaining no-host-header/library boundaries; this is not a claim that
all ISO C++ hosted facilities or exceptions already work in MyOS.

The pristine gthr-posix.h maps native types to pthread objects. Those must resolve
to MyOS's pinned mutex80/cond96/once64-byte ABI, NOT glibc types. Static condition
initializer and all referenced APIs must be audited; no weak-zero route may
silently disable locking. Existing C++ guard/allocation/DSO-finalizer providers
have native evidence, but are not a standard container library or renderer link.

## Ordered implementation/verification

1. Start from a fresh checksum-verified source copy. Add explicit MyOS target/
   generic OS configuration and real thread adapters. Keep original upstream
   algorithms/headers, not hand-written replacement vector/map/mutex classes.
2. Configure through the existing no-host compiler/link boundary using actual
   target SDK providers. Never execute MyOS syscall probe binaries on Linux;
   unsupported functions cannot be made successful with configure cache lies.
   Select no exceptions/RTTI as the measured Mesa profile, not as full C++ support.
3. Inventory configure-generated macros, include routes, exported/missing symbols,
   TLS/init/fini requirements. Reconcile upstream <new> with existing allocation
   providers; avoid duplicate CRT/guard/finalizer definitions. No host libstdc++,
   libc, libgcc_s, default startup files or unchecked library search paths.
4. Link actual upstream vector/unordered_map/rehash/mutex/lock_guard consumers.
   Native tests must prove growth/move/destruction, collisions/rehash, bounds and
   allocation failures, real concurrent exclusion/publication and timed waits
   where advertised; exact resources on success/fault/cancel/relaunch, BIOS/UEFI.
5. Compile and native-test real Mesa ASTC lookup-table and GLSL/container users.
   Their compile success is not shader/rendering success. Update the whole-build
   failure inventory only from a terminal complete keep-going compile.
6. Complete native EGL/GLES/context/present/texture/text integration and then run
   Cobalt HTML/JS/DOM/input/verified HTTPS >=60s with close/relaunch. Imports and
   utility fixtures never replace that web gate. Compressed media33.3 stays out.

Primary guidance: [GCC porting](https://gcc.gnu.org/onlinedocs/libstdc++/manual/internals.html),
[configuration](https://gcc.gnu.org/onlinedocs/libstdc++/manual/configure.html).
Required upstream/new-OS configuration is distinct from installed Linux headers
inspected for diagnosis; none of the latter were copied into the target SDK.
Remaining C-only renderer failures also include sys/file.h, mkdir declaration
and _IOLBF/actual line buffering; no unsupported success declarations allowed.

## Configure diagnostics (not a library acceptance)

tools/probe_cxx_target.py extracts sealed sources into a fresh configure-* output,
uses the existing no-host renderer compiler/provider boundary, and never runs
MyOS syscall binaries on Linux. tools/gcc_myos_target.patch recognizes MyOS and
selects actual compiler/linker/math/stdlib capability tests in crossconfig.m4
and its generated configure. No forced Linux features or /dev/random macros.
Initial24609 exit1 10:36:32+07, configure-v31fc4jf: unsupported host/target;
C99 stdio/stdlib/inttypes and locale/wctype headers also fail actual probes.
Second1589 exit0 10:45:39, configure-z2mdr78d, crossed the target branch but
upstream ignored generate-header failure: missing GCC BASE-VER/DATESTAMP.
This is NOT complete configuration despite its exit0 and initial report flag.
Probe now extracts those original metadata files and requires the generated
target bits/c++config.h, returning failure if upstream silently omits it.
Replacement27830 exit0 10:47:26, configure-p4slmkpj: generated target headers
exist; this proves configuration only, not their semantic runtime capabilities.
Actual source build60077 exit2 10:48:08, build.log in that directory. Failures
include SDK <new> shadowing upstream, freestanding-only language profile,
missing sched_yield declaration in gthread include route/static condition
initializer, locale/wide/stdio services and separately omitted support sources.
Generated _GLIBCXX_HAS_GTHREADS is unset: do not claim working C++ locking.
_GLIBCXX_HAVE_TLS=1 is only a compiler/link probe, NOT native TLS acceptance.
Before use, select validated emulated TLS or disable native TLS in this profile.
Introduced opt-in --cxx-library compiler profile: upstream -I headers precede
SDK -idirafter; no -ffreestanding for C++ (normal C++ language default), while
keeping -nostdinc/-nostdinc++, no Linux macros/startups/libraries. Default renderer
mode unchanged. Hosted-profile83926 configure exit0 10:51:28, configure-q02cu_qs;
its initial invalid -fhosted warning was subsequently removed (C++ defaults
hosted without that C-only switch). No library built with this profile yet.
Boundary test10:52:04 still FAIL: GCC hosted stdint.h includes_next a target
header which the SDK lacks. Add actual GCC-intrinsic/ABI-verified stdint route,
then rerun tests/test_renderer_target_compiler.py --cxx-headers against sealed
libsupc++ source before further compilation; do not substitute glibc headers.
Resolved10:56:26: SDK stdint.h delegates to selected GCC target-intrinsic
stdint-gcc.h, not host libc. tests/test_integer_sdk.py [--hosted] both PASS:
154 format macros, target types/LP64 limits, C/C++ and both char signs. Upstream
<new>/pinned pthread profile boundary now PASS, no DT_NEEDED, host library routes
rejected. Only header/link-boundary evidence; no library/container/native claim.
All source/configuration results above are NOT native/renderer/Cobalt PASS.

## Real gthread target adapter — 2026-10-04

GCC's original gthr-posix.h now has an explicit MyOS C++ branch: sched.h,
strong pthread bindings (no weak pthread_cancel proxy), real dynamic condition
initialization and existing native realtime mutex-deadline calls. No API invented
or silently implemented as unlocked success. Standalone source preparation
selects gthr-default.h -> gthr-posix.h as the target compiler build normally does.
Compiler native TLS is disabled: link support does not prove loader/TLS semantics.
45019 configure0 11:49:38 aieavow6 still had gthreads unset: the missing default
header prevented the genuine probe. 31069 build2 11:50:12. Corrected source
selection30635 configure0 11:52:17 jf2fl8q3: HAS_GTHREADS=1, HAVE_TLS undefined.
Whole source build12608 exit2 11:57:50; cstdlib/locale/stdio/time/support-source
gaps remain. No standard container library or renderer has been linked.

tests/test_gthread.py --configuration build/cxx-target-evaluation/configure-jf2fl8q3
[--uefi], actual user/elf-gthread.cpp imports upstream generated gthread headers.
Final22111 exit0 11:59:17: BIOS native-11g0qzk2 and UEFI native-czlcqy1x; four
successful launches + one blocked-worker Escape cancellation per firmware.
Each success: eight workers, 8000 exclusive counter updates with yield while
locked, condition publication/wakeup, once-only initialization, eight TSS
destructors, recursive lock/trylock/unlock, monotonic-measured realtime cond
deadline. Repeated launch and cancellation return exact pages/heap/file counts.
ELF has strong pthread references, no weak-zero/pthread_cancel or DT_NEEDED.
Fixture uses actual write/abort providers; upstream cstdlib remains failing and
is deliberately not hidden or claimed by this thread-only dependency test.
Both normal image hashes remain PHASE332_OS_INFO.md values. No runtime/kernel
architecture change: reuse full93613; scope is target source patch/fixture only.
Next: implement and test missing real C-library services needed by upstream
containers/stdlib, then actual vector/unordered_map/mutex users/Mesa rendering.

Follow-ups: PHASE332_CSTDLIB_SDK.md (0f3f768) and PHASE332_CSTRING_SDK.md.
Native Cstdlib/Cstring dependencies now tested; C++98 ABI/gthread also PASS.
Latest whole library build-after-cstring.log exit2 12:32:29 still needs real
stdio/ctime/locale/math and omitted support-source integration. No full C++,
native renderer or Cobalt acceptance. Do not substitute compile macros for APIs.

PHASE332_CXX_CONTAINERS.md: actual GCC vector/hash/prime-rehash SDK tested on
BIOS/UEFI, fatal range/assertion ABI and recovery. Integrated into renderer's
explicit --cxx-configuration profile. GCC _1j4xqjo configure PASS, full library
still fails stdio/ctime/locale/errno. Renderer3c959yqk compile1 (5 units/13 errors):
ctime/cache flock header/mkdir/_IOLBF remain. Full C++ and Cobalt still OPEN.
