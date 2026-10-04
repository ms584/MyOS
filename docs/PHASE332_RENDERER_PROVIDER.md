# Phase33.2: real separate renderer provider package and native staging

2026-10-04 (+07), baseline2ee96b4. Phases33.1/33.2 OPEN; no33.3/push.
Imports163/242=67.4% fixture-supported subset; Cobalt acceptance0/5.

## Separate provider, not a fixture entry-point substitute

tools/build_renderer_provider.py reuses exact native-tested Mesa/EGL/GLES/SDK
objects/archives from a successful fixture's logged linker closure. The log is
data, not executed commands; paths are bounded to build/, diagnostic wrappers
rejected, only explicit support objects/Mesa target/archives selected. Fixture
entry/font objects excluded. New link invokes ld directly, with no host CRT/libc,
no host runtime/loader and explicit getter roots. e_entry remains0.

Generated input manifest records SHA256 of every dependency, fixture and evidence
log. Package checks ELF64/x64/ET_DYN, entry0, bounded file-backed LOADs without
RWX, no interpreter/DT_NEEDED/TEXTREL, exact nonempty GLOBAL DEFAULT FUNC getters
within file-backed RX, allowed relocation types and all strong imports against
the current actual platform runtime export allowlist. These metadata checks do
not certify complete ABI semantics or prove multi-module execution.

provider-njs6506e built21:04:31:16,375,808 bytes on disk, LOAD memory15,532,099.
SbGetEglInterface offset548768/size12; SbGetGlesInterface548816/size63. These
are measured ELF offsets, not hardcoded runtime getter addresses. No missing
platform imports. Actual TLS: initialized8 bytes, total80, alignment8. Actual
relocations: RELATIVE/GLOB_DAT/JUMP_SLOT/DTPMOD64/DTPOFF64. Init array24 bytes,
fini array8. Provider therefore needs real module TLS and constructor/finalizer
ordering; deleting PT_TLS or ignoring these arrays is not a valid integration.
Existing per-image environment/exit/C++ registration must also be audited for
cross-module ownership/stop ordering. Report explicitly native_provider_executed=false.

## Native stage/discard tests and limits

tests/test_renderer_provider.py makes disposable asset/frozen boot images, stages
this actual provider through the MyOS linker twice, checks binding0-missing,
real RELRO/TLS relocation, discard and exact free physical page return each time.
Generic elf live on e_entry0 must remain fail-closed before startup. No constructors,
getters, renderer code or Cobalt are claimed executed by a staging test. Heap
counts are not exposed by shell mem; this test proves page return, not a separate
quantified kernel-heap delta. Prior executed renderer fixtures cover their own
page/heap/resource scope, not new multi-module lifecycle.

Initial BIOS64 stage/discard/repeat/reject PASS native-stage-um7w8hg1.
Initial sequence30924 exit1 21:11:22: UEFI64 native-stage-jaxywxg5 OOM,
baseline2792 free physical pages (~11MiB) below provider LOAD requirement.
This is NOT a successful UEFI staging result. Test now checks page return before
expected outcome, including unexpected OOM. Updated matrix56309 terminal exit0
21:17:56: UEFI64 OOM/reclaim native-stage-e2vwlzaf, UEFI128 stage/discard
native-stage-72wb3cma, BIOS16 OOM/reclaim native-stage-tmi7xcu0 all PASS.
Each repeats twice and rejects entry0 execution. No live image/test owner.
Four normal production hashes unchanged21:20:53. Staging is not provider execution.
Logs: build/renderer-evaluation/renderer-provider-package.log and
renderer-provider-stage*.log; package link/import/dynamic/TLS/SHA evidence in
build/renderer-provider-evaluation/provider-njs6506e/. Native directories retain
serial.log, assets and frozen image. Isolated packaging/test; kernel/runtime
unchanged, full9925 architecture evidence still applies.

## Real next integration (not a renamed completion gate)

Production linker still only one object, USER_BASE bias, module ID1, single TLS
template. Add actual core/provider mapping, collision/permission/rollback checks,
real export resolution and multi-module TLS/template/worker isolation; preserve
main/worker guards, W^X/RELRO, missing-import fail-closed startup and SABI preflight
before constructors. Include module init/fini ordering, faults/OOM/cancel/relaunch.
Test kernel262072/cap262144 (72-byte headroom) and kernel/BSS low-memory boundary
remain unchanged; do not remove caps to fit unverified architecture. Then native
Cobalt remaining79 imports/weakTLS and HTML/JS/DOM/concurrent input/verifiedHTTPS
>=60s/close+relaunch gate. Provider RAM observations are not Cobalt RAM minima.

Reproduce via tools/run_logged.py: tools/build_renderer_provider.py
--fixture build/native-renderer-evaluation/native-21xnvj1n
--evidence-log build/renderer-evaluation/native-gles-interface-final-bios.log;
tests/test_renderer_provider.py --provider build/renderer-provider-evaluation/provider-njs6506e
(--uefi --ram 128 for normal UEFI; --uefi --ram 64 --expect-oom or --ram 16
--expect-oom for measured OOM cases).
