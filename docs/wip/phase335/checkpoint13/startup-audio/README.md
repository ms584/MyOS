# Phase33.5 checkpoint13: startup audio, not acceptance

2026-10-11. Production player and normal images remain unchanged. Phase33.5,
Phase33 and Phase32 remain open; owner has not tried/listened.

Source Cobalt VP8 fixed-layout continuous run siu14qbj completed04:16:47.
Unchanged analyzer failed04:17:02: a 12.880ms held-sample gap begins at1.008798s.
600 decoded, zero dropped/corrupted, max sync22697us do not override this failure.
Real native SB16 underruns increase during startup; provider source-starvations0
is a different counter. The fixed viewport isolates decode measurement; it does
not establish that the earlier partial-composite/reflow observation is harmless.

Rejected candidate4096 changed PCM_CONFIGURE DMA-half length, not a preroll.
Only1024/8192 are supported. SbPlayerCreate returnsnull, fixture status5;
no playback pass. Failure is retained. No kernel API was relaxed.

First-frame candidate holds PCM until actual first video submission for VP8,
matching the existing native VP9 ordering. DMA half remains1024. Native asset
focused test kpk8nzkw completed04:29:41 exit0: pause/resume/forward seek/EOS,
same-process restart/stop, max sync9956/9987us, zero drop/corruption, all2380
samples released, physical/SHM/alias/heap cleanup exact. This is a standard
fixture, not Cobalt/HTTPS/YouTube or audible Rickroll acceptance. RAM8585216B
is sampled player increment, not a true peak. No audio listening is claimed.

Candidate source SHA2567a923b193cb4c540ffe21973de038bc765cab3591cfedcb4c17fff26cd81487d.
Provider SHA2567cd804458f8a1da6834877b03d8f3325618f58a74348c2f0a764cc3a379e0c00.
New source-core continuous retest wr6bjpii started11:46:27, unchanged analyzer
and memory budget apply. Result is pending; no promotion/acceptance inferred.

Logs, captured PCM, serial, fixture evidence, candidate patch and sealed build
reports are retained here. Private TLS keys/assets are excluded.
