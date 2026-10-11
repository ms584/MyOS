Current result: owner confirmed first and fresh separate SDL boots for Rickroll
song >30s and pause/resume/both seeks without stale sound. Search clicks still
select paragraph instead of target. New pointer fix is checkpoint20, unaccepted.
Fresh settled movie ROI(120,140,690,300) frozen36–43s; WAV growth0 in same interval.
See fresh-sdl-recording-audit.json. One FPS and flush alignment limitations apply.
These results belong to OLD checkpoint19 normal images, not the new cursor build.

# Phase33.5 checkpoint19 — visible SDL comparison, acceptance still open

The owner supplied a screenshot proving that starting QEMU from visible Windows Terminal displayed the normal MyOS shell. WSLg shared-memory errors did not occur in that GTK session, but the owner still observed spinning/stuttering and native serial showed18 rebuffer pauses/max outlier860037us. This disproves shared-memory error as the complete explanation of playback failure.

The same frozen normal build/core/provider with SDL/PulseAudio displayed actual online Rickroll. The owner explicitly reported both picture and audio smoother, then confirmed listening >30seconds and pause/resume/forward/backward audio behavior with no stale sound. Settled pause screenshots show identical movie pixels and resumed pixels change. Native new-frame anchors117120000→109520000us confirm backward seek; the first screenshot taken3seconds after input still showed an old overlay, so that screenshot alone is not a backward-seek pass.

This is one interactively checked session, not final phase acceptance. Current reopen screen/PCM capture and fresh-boot SDL repeat are pending. SDL is a launcher diagnostic switch, not a guest-code workaround; GTK default remains unchanged until the candidate is evaluated. No kernel/provider/core edits, no host YouTube fetch/demux/decode, no prepared MYV1, and no push.

Audio recording uses the actual QEMU PulseAudio device mirror `wavcapture`, supported by the installed QEMU monitor. Host monotonic timestamps and WAV file-byte samples provide approximate recording alignment, not a quantitative A/V sync measurement. QEMU documents this deprecated diagnostic command at https://www.qemu.org/docs/master/system/monitor . Capture is not a substitute for the owner's listening results.
