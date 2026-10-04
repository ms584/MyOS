# Phase33.2: pinned44-entry EGL table and actual context version queries

2026-10-04 (+07), baselineb9f2fdc. Phase33.1/33.2 OPEN; no33.3/push.
Imports163/242=67.4% fixture-supported subset. Cobalt acceptance0/5.

## Actual context query fix

Original EGL core queries the driver's ClientMajorVersion field. The MyOS driver
left the requested minimum there even though actual OSMesa GLES contexts may
support a higher compatible version. Original st_context.c computes ctx->Version
during creation, before binding. MyOSMesaContextVersion now reads that actual
state; driver initializes both major/minor from it after successful allocation.
No version string rewrite, host query or temporary context binding is used.

Native test creates requested2.0,3.0,3.1 contexts, queries EGL before binding,
then compares with real GL_MAJOR_VERSION/GL_MINOR_VERSION after binding.
Pinned backend supports3.1 in all three cases. Existing undefined/unsupported
request rejection, debug and surfaceless tests remain; changed metadata must
not allow requesting unsupported versions. Source-based bug diagnosis, not
an assumption that matching requested metadata proves supported version.

## Exact table

user/platform-egl-interface.c implements SbGetEglInterface with all44 fields
in the actual pinned SbEglInterface declaration. Size/attribute/time/int ABI
assertions and compiler function-pointer checking apply. Four native intptr
parameters use typed wrappers to the actual EGL native pointer typedefs;
other entries directly reference original Mesa EGL functions. No success stubs,
NULL placeholders, synthetic EGL objects or permissive unsupported capabilities.
Static const table lives in the native fixture's protected ELF data.

Probe copies EGL/log/system headers only after verifying each pinned hash in
PHASE331_ABI_SOURCES.json. Generated configuration declarations describe actual
compiler scalar sizes and attributes; they do not change upstream headers or
generate runtime behavior. C-only explicit getter prototype satisfies the original
Mesa strict missing-prototype check without rewriting the pinned old-style header.

egl-core15-test.c now obtains the pinned table and calls core context/debug,
sync/client/server waits/query/image/delete/error paths through its function
pointers. Stable getter/default native-display identity and44 nonNULL slots are
also checked. Exact cyan FBO pixels and resource counts prove the exercised
operations;44 slot existence alone does NOT prove every API semantic contract.
Other existing window/glyph/context/image/fence regressions remain in the fixture.

## Evidence

Version-only core-7zrxg58b22/22 compile20:20:08. BIOSj9w5_jr9 and
UEFIk5b3h8g_ PASS59910 exit0 20:27:16, two launches/exact resources each.
Initial table core-sjrlcnls compile failed on required SB_NORETURN macro and
missing C prototype. Added compiler attribute/prototype, no ABI suppression.
Final core-lu672okl23/23 compile PASS20:26:55.
Native table BIOS4n9rxyg_/UEFImpf0jksv PASS37260 terminal exit0 20:33:34,
two launches/exact resources each, with required pinned44 marker and table calls.
No live image/test owner. Four normal hashes unchanged20:32:45.
Logs: build/renderer-evaluation/egl-interface-target*.log,
native-egl-interface*.log and native-egl-context-version*.log. Per-fixture
serial/QMP PPMs retained. Isolated libraries/fixtures; kernel/runtime unchanged,
full9925 architecture evidence remains applicable.

## Required integration / limits

This does not add SbGetEglInterface to the production loader allowlist or increase
Cobalt imports. Current linker is single-object eager, rejects DT_NEEDED and
resolves undefined names only against fixed platform runtime exports. Actual
provider coexistence/relocations/lifecycle/TLS/address ownership is still required;
copying function names into that list cannot map a Mesa provider by itself.
Next: complete exact GLES table, genuine native dispatch test, then provider/
loader integration and remaining79 strong imports/weakTLS before actual Cobalt.
Numeric image values, zero-sized pbuffer dimensions, termination/reinit and
concurrent ownership/bind-allocation rollback still need audit. Dependency
fixtures do not prove actual HTML/JS/DOM/concurrent input/verifiedHTTPS>=60s,
close/relaunch gate. No phase closure or certification claim.
