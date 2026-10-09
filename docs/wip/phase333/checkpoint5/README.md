# Phase33.3 checkpoint5: expanded thread handles and measured heap

Phase33.3 remains OPEN. No Phase33.4/YouTube claim.

## Concrete lifecycle fault
The old MRT thread_control rejected handles outside LOW anonymous VM,
although kernel mmap also allocates thread stacks in MID/HIGH.
The shared helper is used by pthread_join, detach and getattr.
A register/stack diagnostic placed the crash in Cobalt PlayerStatusCB,
called by the native player's Destroyed callback after a failed join.
Cobalt released callback context while its worker was still running.
The fix admits only the kernel's three anonymous VM ranges and additionally
requires a resident first page before reading the thread-control magic.
Runtime/TLS gaps and unmapped/PROT_NONE guards remain rejected.

The old-image focused fixture reproduced the shared helper's getattr rejection.
Matching fixed-image and production fixtures passed real MID/HIGH worker
join, detach, stack attrs, expired/guard handles and two exact-resource relaunches.
The test is now included in the architectural full suite.

## Engine evidence
Matched fixed candidate and production normal image/MRT hashes are identical:
normal-identity.json. Native VP8/Opus, verified TLS, MSE and SB16 were used.
Three launches each completed pause/resume/seek15, EOS40, fresh MediaSource
restart in the same Cobalt process, and stop. Six native players reported
547/138,547/141,545/132 presentations; maximum sync53.234ms, original limit100ms.
No user fault/panic occurred. PMM/SHM/aliases reclaimed on every launch.
Kernel heap grew61440 profile bytes cold,4096 second, then exact540416 third.
These are retained profile files, previously audited; not codec/frame leaks.
The WAV and three PNGs come from this same guest session.
QEMU captured the actual11025Hz mono U8 device format, with no host decoder.
This format change did NOT eliminate held PCM samples (selected window52.88ms).
Measured hardware underruns increased273/269/292, unlike source-starvations0.
Sound continuity/pitch acceptance remains OPEN; do not treat audible tones alone
or zero source starvation as proof of correct hardware playback.

## RAM
Allocator tracks rounded live allocations and a runtime heap high-water.
Its existing heap walk independently verifies exact live-counter equality.
Deliberate boot OOM is tested, reclaimed, then the runtime peak starts.
Engine controlled-start0.5s PMM samples plus conservative incremental heap
high-water and verified372736B media provider delta total28024848/20652048/
18800672B against33554432B. Exact per-launch samples:engine-measurements.json.
PMM is sampled; it can miss short peaks. Final allocation-bound acceptance
is still pending. No budget increase or resolution/frame-rate reduction.

## Adverse HTTPS and regression
One actual range response at byte131072 was delayed8s; engine recovered,
continued its media clock, accepted stop, and returned all PMM/SHM/aliases.
Cold heap growth61440B is the profile-file allocation noted above.
https-stall.json and serial hold the observed events; max sync18.738ms.
Truncate/corrupt/close scenarios are implemented but results are still pending.

Full regression5 PASS terminal0 at14:58:06+07, normal framebuffer restored.
This covered allocator accounting with the OLD MRT; log is losslessly gzipped.
After promotion of the MRT fix, full regression6 is ACTIVE, not yet PASS.
Root build image is being used/rebuilt by that suite; do not call it restored.
Final UEFI, full regression, audio continuity, RAM bound, guest playback OOM,
stream corruption/truncation/close and normal-image restoration remain gates.
