# Phase 33.2 native Starboard launcher

2026-10-01, continuation from local 36a5cea. Phase 33.1/33.2 remain OPEN.
This is a loader/lifecycle boundary, NOT Cobalt HTML/JS/DOM/HTTPS acceptance.
No Phase 33.3 work and no push.

## Implemented boundary

`elf starboard ASSET [URL]` explicitly starts a Starboard shared object even
when ELF e_entry is zero. Generic `elf run/live` still reject such an image;
there is no globally invented entry point or timeout removal.
Single-object eager relocation, missing-strong-import rejection, W^X, RELRO,
init/fini validation, foreground Escape cancellation and process reclamation
remain enforced. Missing imports cannot reach the getter, constructors or engine.

The dynamic symbol table, not section headers, supplies `SbEventHandle` and
`GetEvergreenSabiString`. Both must be unambiguous, defined strong GLOBAL
STT_FUNC, default/protected visibility, nonzero size, entirely file-backed RX.
Weak/data/hidden/undefined/absolute/out-of-image/oversized/duplicate exports
are rejected. Unrelated long C++ names do not need a 256-byte buffer.

The verified getter executes in Ring 3 BEFORE DT_INIT and INIT_ARRAY. Its
returned NUL-terminated descriptor must exactly match the pinned x64 SysV,
little-endian, API-18 Evergreen SABI JSON. Scalar size/alignment/signedness
are compile-time asserted. Mismatch exits 95 without constructors/finalizers;
bad code or string pointers fault only the owned process. Matching the SABI
descriptor does not prove any API implementation or certify the platform.

The bootstrap preserves launcher arguments across getter/constructors, then
calls the private native pump with the actual exported SbEventHandle. Start
receives owned argv: ASSET, optionally `--url=URL`, a NULL sentinel and the
URL as StartData.link. Bounded name/URL bytes live in the process's RX bootstrap
page until teardown; they are immutable and must not be modified by the app.
The existing public SbRunStarboardMain fixture boundary remains available.
Normal stop drives Blur/Conceal/Freeze/Stop and reverse FINI_ARRAY/DT_FINI;
fault/forced cancellation never fabricate graceful events or finalizers.

## Actual stack and runtime bounds

Generic ELF main stack remains four NX pages (16 KiB). The Starboard launcher
allocates 512 NX pages (2 MiB), downward from the unchanged stack top, with an
unmapped page beneath. This is a selected budget, NOT a measured Cobalt minimum.
It does not relax ELF span, VM, low-memory kernel, BIOS stack or DMA boundaries.

SYS_MAIN_STACK_INFO (48) reports the actual main base/length/guard; validates
reserved arguments and the entire writable 32-byte output before any copy.
`pthread_getattr_np(main)` and `getrlimit(RLIMIT_STACK)` now use this information,
not a hardcoded main base or the default worker stack. Since the main stack
cannot grow, its current/maximum resource limits equal its mapped size.
Worker stack attributes remain their actual separate allocations. Legacy app
stack metadata reports its one page, without claiming an unmapped guard.

Production runtime uses the measured small-PIC, fixed-address codegen with
static direct binding. The linker permits one RX LOAD and fails on mutable
data/GOT/TLS state or overlap with private TLS at +64 KiB. There are no dynamic
relocations or RWX LOADs. Main/worker state remains in owned RW allocations.
Runtime/UI/events/lifecycle fixtures natively validated the codegen before
launcher development; compile-only host measurement was not substituted.

## Evidence and remaining gate

Native launcher fixture PASS 06:39:26 +07: pinned upstream event/input/window
declarations and binary SABI descriptor; e_entry=0 and sectionless startup;
eleven malformed export cases; getter before constructors; argv/link; recursive
256 KiB stack use; actual main attributes/limits and atomic output rejection;
unmapped guard and NX faults; real PS/2 input; graceful finalizers; getter
fault/mismatch, Escape cancellation, repeated launch and exact page/heap return.
Reproduction: `python3 tests/test_starboard_launcher.py` (also `--uefi`).
The all/apps system suite includes this fixture. A follow-up case also rejects
reserved visibility bits. Test URL punctuation uses
real QEMU PS/2 modifier/key events, never direct shell/function injection.

Expanded scoped regression PASS 06:47:58; full tests/test_system.py PASS by
07:21, restored normal image. UEFI DMA/frames/PCM/network and native launcher/
events/UI PASS, sequence exit zero 07:22:32. Supplemental BIOS launcher PASS
07:27:28 with all twelve malformed exports and exact guard/NX/getter page-fault
error bits/addresses. Normal kernel 253464 bytes, image 1474560, runtime 29840,
BSS end 0x7d400; unchanged safety limits. Details: PROJECT_STATE.md/HANDOFF.md.
Actual pinned Cobalt remains fail-closed on 82 strong imports plus weak TLS;
it has NOT executed. Native EGL/GLES/dependency closure, remaining required
services and the actual HTML/JS/DOM/input/verified HTTPS >=60s interactive,
close/relaunch gate are still required. Never close the phases on this fixture.
