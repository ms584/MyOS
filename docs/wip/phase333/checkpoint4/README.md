# Checkpoint4 - regression/UEFI progress; Phase33.3 still OPEN
Full tests/test_system.py exit0 at11:20:44, with all original assertions and
normal BIOS restoration. The ZIP preserves the complete original log.
Normal UEFI built and real guest native-underlay pixel/cleanup tests plus
PCM focused tests passed on UEFI. Hashes identify current normal artifacts.

Two non-trace provider observations faulted in the pinned Cobalt core at
RIP0x800343612e while loading MSE metadata; guest shell/resources recovered.
The latest isolated fault-register kernel observation instead played normally,
with542 presented frames,537 sync samples,max11148us,two dropped frames.
A traced provider run had538 presentations,530 samples,max16439us and zero
reported dropped/corrupted frames. These are provisional diagnostic data,
not final acceptance; normal non-trace stability is not yet established.
The old 'underruns=0' field was an unmeasured placeholder, not evidence.
Source now counts actual source-starvation episodes separately; hardware
underruns and PCM gap quality still require measurement.

Heap after cold engine exit is higher by61440bytes on completed playback,
or45056bytes on failed initialization. Private allocation diagnostics found
retained4KiB blocks; inode ownership/bounded warm relaunch is under investigation.
Raw heap/TLS private keys remain only in ignored build; never publish them.
Remaining:stable non-trace engine,EOS/restart/stream faults,full media RAM budget,
PCM quality,sync measured against captured frames and final cleanup.

Additional ownership evidence identifies the61440byte cold delta as15 linked
profile/cache file allocations with no open references. The extra36608byte
lazy asset index is separately identified. Warm replay bounds are still open.
The newer non-trace diagnostic run measured541 presentations,536 samples,
max10423us,two dropped frames,zero measured source-starvation episodes.
This source-starvation metric is distinct from hardware underruns.

## Normal replay and engine lifecycle, 2026-10-09 14:04 +07
Three consecutive normal non-trace launches completed in the same VM:
542/544/546 video presentations, maximum measured consumed-audio vs
presented-video PTS error 17316/15230/9986 microseconds; no guest fault.
PMM, SHM and aliases returned exactly on each launch. Cold profile/cache files
consume61440 bytes; a second launch adds4096 bytes; third heap540416 before/after
is exact. Heap snapshots map retained data to linked profile/cache inodes.
See checkpoint4/normal-three-launch/ for original PCM, selected screenshots,
serial/resource metrics and ownership summaries. Earlier GP cause is unresolved;
three successful runs are evidence of replay stability, not proof of a fix.

A normal non-trace engine lifecycle session reached DOM ended at40s after
real pause/resume/seek15s, then restarted a fresh MediaSource within the SAME
Cobalt process, played again from0s, and stopped. Two native player instances
reported550/145 presentations and maximum sync52175/36092us respectively.
All PMM/SHM/aliases reclaimed on process exit. See checkpoint4/engine-lifecycle/.
Generation guards reject old request/update callbacks; stop aborts in-flight
XHR, clears src/load and revokes the old blob URL. No host media decoding.

Raw PCM diagnostics reveal long held samples (up to72.8ms in selected steady
windows) and reduced tone crossings. This is an OPEN sound-quality issue;
source-starvations=0 does not prove DMA/capture continuity or correct pitch.
Do not declare the whole phase passed from these sync counters.

Controlled-start RAM measurement is now running: page waits for B after UI
ready and3s settling; sample matching-kernel PMM free_count every0.5s during
fetch/demux/decode/render/audio/EOS/restart. Original32MiB budget remains.
Conservative accounting will add the entire16MiB reserved kernel heap and
provider mapped PT_LOAD pages to observed PMM incremental peak; this covers
underlay allocations and preload cost but sampling is not an exact high-water
bound. Final budget acceptance must state these limits or add allocation bounds.
