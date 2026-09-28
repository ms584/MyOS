# Phase 33 engine assessment and Phase 33.1 entry plan

Assessed 2026-09-28 against MyOS commit
`8f5dd85e9d8b982802dfe9dbb026b971faa956c4`.
Status: static engine assessment complete; native engine feasibility unproven.
Phase 33.1 is in progress: the first asset/static-ELF slice and import register
are recorded in [PHASE331.md](PHASE331.md). Engine smoke is not implemented.
Phase 32/33 remain **NOT PASSED**.

## Decision

Use **Cobalt 27 LTS, Evergreen 7.3.2 x64 / Starboard 18** as the primary
native-port feasibility target. Evaluate a Lite-style deployment without an
automatic updater. This selects a concrete binary/ABI to investigate; it is
not a claim that Cobalt boots on MyOS or that YouTube accepts this platform.
Keep the MyOS kernel. Build tools may run in WSL; browser, network, decoding,
audio and rendering must execute in MyOS for acceptance.

| Candidate | Evidence and fit | Decision |
|---|---|---|
| Cobalt 27 LTS Evergreen | Published x64 core with a defined Starboard/ELF-loader boundary. Media-oriented integration exposes player and audio interfaces. Requires substantial POSIX, graphics and media implementation. | Primary bounded port investigation. Pin the release; do not chase main. |
| WPE WebKit | Modern embedded WebKit; official FAQ currently describes Linux support. Current 2.54 integration uses WPEPlatform, with a separate native platform/media port needed for MyOS. | Backup if Cobalt deployment or compatibility is infeasible; not a cheaper drop-in. |
| NetSurf | Existing libwapcaplet work helps only one dependency. NetSurf has Duktape JavaScript, but its binding documentation lists incomplete DOM/CSSOM coverage. No demonstrated YouTube media stack here. | Retain lightweight browsing separately; do not choose it for this acceptance gate. |

Sources: [Cobalt release](https://github.com/youtube/cobalt/releases/tag/27.lts.3),
[WPE FAQ](https://wpewebkit.org/about/faq.html),
[WPE architecture](https://wpewebkit.org/about/architecture.html),
[NetSurf JS bindings](https://github.com/netsurf-browser/netsurf/blob/master/docs/jsbinding.md).
Candidate ranking is our engineering assessment, not an upstream guarantee.

## Measured package, not guessed RAM requirements

Downloaded the official normal x64 release CRX and inspected its ELF using
`readelf`. No Cobalt binary was executed, no YouTube media was downloaded,
and no upstream source build was performed during this assessment.

- Release tag: `27.lts.3`; source tag commit:
  `3befd332486952f6603ae3baf2a8713878d6e200`.
- Asset: `cobalt_evergreen_7.3.2_x64_sbversion-18_release_20260905002529.crx`.
- Download: 45,200,514 bytes (43.1 MiB).
- All archive entries uncompressed: 104,929,222 bytes (100.1 MiB).
- `lib/libcobalt.so`: 103,391,688 bytes (98.6 MiB).
- ELF LOAD virtual extent before page rounding: 104,004,256 bytes (99.2 MiB).
- 243 named undefined dynamic symbols: 242 GLOBAL and one WEAK.
  The null ELF symbol is excluded. These are imports, **not 243 new syscalls**.
- Relocations observed: RELATIVE 117653, JUMP_SLOT 626, GLOB_DAT 17,
  and R_X86_64_64 20. Dynamic constructors and GNU RELRO also exist.
- No DT_NEEDED entry was observed; that does not mean platform-independent:
  imports must resolve through the loader's platform ABI.

[PHASE33_COBALT_AUDIT.json](PHASE33_COBALT_AUDIT.json) preserves exact names,
download URL, hashes, source/baseline pins and relocation counts. Source tag
identity is recorded separately from downloaded binary identity; reproducible
equivalence between the source tag and Google's binary has not been proven.
Ignored inspection files are under `build/engine-evaluation/`.

The roughly 99 MiB mapped image excludes JavaScript heap, thread stacks,
media buffers, decoder state, graphics surfaces and the kernel. It is neither
peak resident memory nor a RAM minimum. The current 64 MiB demo configuration
cannot eagerly map this full image. MyOS currently manages at most 256 MiB of
physical pages: merely passing QEMU more RAM will not fix that limit.
Do not publish 512 MiB or 1 GiB as a measured requirement yet.

For budgeting, three RGBA 640x360 frames cost 2,764,800 bytes, before decoder
reference frames; 2 seconds of 48 kHz stereo float PCM cost 768,000 bytes.
These are illustrative buffer calculations, not the release's actual demand.
Measure mapped/resident peak, stacks, media queues, underruns and frame timing
at each native milestone; then derive recommended and minimum guest RAM.
Keep engine assets on a separate read-only data volume initially. They cannot
fit in the existing tiny kernel image or 64 KiB RAMFS file format.

## Exact API families and local gaps

The import inventory is the lower bound; function-pointer interfaces returned
by Starboard and transitive runtime behavior add requirements. Match Starboard
18 wrapper layouts, errno, pointer widths and calling conventions; do not bind
an identically named MyOS function blindly. Not every libc function needs a
kernel syscall. Optional features must report unsupported honestly.

| Required boundary / observed imports | MyOS baseline evidence | Necessary work |
|---|---|---|
| ELF64 loader, relocations, constructors | `kernel/app.c` loads MYOSAPP1 flat payloads; `include/process.h` caps code at 16 pages | Native user loader + large mapped files; reject corrupt layouts/relocations. Use the upstream Evergreen loader design as reference. |
| `mmap`, `munmap`, `mprotect`, `madvise`, allocation | 32 user heap pages; kernel heap selects 4 or 16 MiB in `memory/memory.c` | Per-process VM areas, allocation lifecycle, executable-memory policy and page protections; distinguish virtual reservations from committed RAM. |
| `pthread_*`, semaphores, clocks, signals | Scheduler/process demonstrations exist, but no matching user ABI in `include/syscall.h` | Thread identity/TLS, blocking waits, timed wakeups, thread cleanup, signal semantics actually used by the selected build. |
| Floating-point/SIMD execution | `cpu/interrupts.asm` saves general registers, no extended-state save/restore | Detect required CPU features and preserve enabled FP/SIMD and TLS state across preemption, faults and process exit. Test MXCSR too. |
| `socket`, `connect`, `recv`, `send`, `poll`, `epoll_*`, `eventfd` | `include/net.h` exposes one serialized kernel TCP stream; 13 user syscalls have no sockets | Concurrent socket handles, nonblocking progress, readiness/cancellation, user ABI. Existing verified HTTPS alone is not this interface. |
| `openat`, `pread`, `pwrite`, `lseek`, `fstat`, storage | `include/fs.h`: 65536-byte files; persistence uses bounded snapshots | Large seekable engine/content storage and user file offsets; preserve existing data format until an explicit migration design. |
| `SbGetEglInterface`, `SbGetGlesInterface`, window/decode targets | Framebuffer GUI exists in kernel; no user EGL/GLES service | Choose and validate a native renderer/backend; assess software rendering first, measure throughput. Framebuffer copying alone cannot implement GLES. |
| `SbPlayerCreate`, `SbPlayerWriteSamples`, `SbPlayerSeek`, playback rate | Only custom MYV1/RLE and PCM playback | Standard compressed-video/audio decoding, timestamps, buffering, flush/seek, end-of-stream and error handling. Select codecs from actual supported stream negotiation; do not advertise codecs without a decoder. |
| `SbAudioSink*`, sample frequency/format queries | SB16 baseline: 11025 Hz 8-bit mono | Native audio queue, sample conversion/resampling as needed, clock synchronization, latency/underrun accounting. |
| System paths/properties, fonts, locale, storage, capabilities | Basic kernel services; no complete Starboard platform | Package content/ICU/fonts/trust data, define capability reports and resource budgets. Audit certification-related and DRM imports; do not fabricate keys or claim DRM support. |

Pinned source references:
[exported symbols / ABI wrappers](https://github.com/youtube/cobalt/blob/3befd332486952f6603ae3baf2a8713878d6e200/starboard/elf_loader/exported_symbols.cc),
[Evergreen architecture](https://github.com/youtube/cobalt/blob/3befd332486952f6603ae3baf2a8713878d6e200/starboard/doc/evergreen/cobalt_evergreen_overview.md),
[Lite deployment](https://github.com/youtube/cobalt/blob/3befd332486952f6603ae3baf2a8713878d6e200/starboard/doc/evergreen/cobalt_evergreen_lite.md).
Some upstream prose still mentions old build tooling; pinned build files and
actual exports take precedence when implementing this release.

## Phase 33.1: native runtime feasibility, in this order

1. **Contract before implementation.** Classify every measured import as libc,
   MyOS service, platform callback, optional unsupported feature, or unresolved.
   Inspect the pinned SABI wrappers and header contracts. Define testable errors
   and layouts; resolve renderer and decoder library candidates with license,
   CPU and dependency checks. An unresolved essential interface blocks engine
   integration; a stub returning success is never a pass.
2. **Executable and storage slice.** Add a separate ELF/user-runtime path and
   large read-only asset source. First run a small ELF fixture, then map and
   relocate the pinned engine with an explicit missing-symbol report. Verify
   bounds, permissions, constructor handling and cleanup. Do not execute the
   core against dummy service implementations or embed it in the kernel.
3. **Runtime slice.** Implement required VM/thread/TLS/time/wait semantics and
   FP/SIMD context preservation with preemption tests. Replace the browser's
   5-second `app_run()` lifetime restriction with a cancellable long-lived app
   lifecycle; retain bounded demo behavior where appropriate. Verify >60-second
   responsive execution, forced close and resource reclamation.
4. **Native platform smoke.** Build the MyOS Starboard layer, run applicable
   upstream compatibility tests, then start the pinned engine in MyOS and render
   a local HTML/JavaScript page accepting input. Record failures and memory
   peaks. Supply real required platform services; unsupported optional features
   must fail according to their documented contract.

Phase 33.1 passes only after this native engine smoke and relevant regressions;
an ELF fixture alone passes only step 2's fixture subtest. The per-symbol register and first static ELF/storage slice are now available;
the remaining dynamic loader and native-runtime work is tracked in PHASE331.md.
Do not expand networking/media/kernel memory limits indiscriminately first.
For architectural code changes run `python3 tests/test_system.py`, plus new
focused checks, and restore the normal image. No regression run is claimed for
this documentation/static-audit change.

## Remaining feasibility risks and full acceptance

Native engine startup does not establish YouTube compatibility. We still need
to prove the supported YouTube frontend works on this custom platform without
unavailable certification credentials; rendering speed on QEMU is unknown.
If that frontend is incompatible, record evidence and revisit the WPE option
before expanding MyOS around an unusable engine. Source-build/toolchain fit,
optional weak-symbol behavior, graphics choice, codec choice and actual peak
RAM remain explicit open items, not hidden promises.

After 33.1: native concurrent HTTPS and real page execution; native compressed
media playback and synchronized output; then actual Rickroll online for at
least 30 seconds with audible music, pause/resume/seek and responsive close.
Follow [PHASE33.md](PHASE33.md) for evidence. Do not count a host player,
transcoded fixture, YouTube HTML download or successful Cobalt link as passing.
The existing roadmap numbers and deferred Phase 24/25 remain unchanged.
