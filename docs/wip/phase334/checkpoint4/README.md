# Phase33.4 checkpoint4 — diagnostic progress, NOT PASSED

2026-10-09 +07, MyOS/WSL repository; no push, no33.5/Rickroll closure.
Prerequisites were replayed and passed before YouTube work.
The prior40-worker full suite passed; checkpoint3/regression records that
result and exact normal restoration. New PMM1024 full suite is running
from23:34:56, build/phase334-checkpoint4-full.log; not yet claimed PASS.

pmm1024-focused: actual E820-limited1GiB, low/high BIOS/UEFI memory tests,
high-frame writes/aliases/reference lifetime/reclaim, normal image identity.
quality144-512-failed: manual native Quality144p, only3s metrics then V8
allocation failure/int3 fault; no30s acceptance.
online1g-sync-recovery-failed: real online YouTube BBB in MyOS,640x360
decode/160x90 presentation, visible Quality144p selection, same-session
11025Hz monoU8 audio, pause/resume/seek,close/reopen. Native PCM WAV
838.257s includes silence,488 audible one-second bins. No host fetch/decode.
144p source dimensions not printed in this session; do not infer them.
Sync621368us/367026us/253931us violates100ms candidate budget.
Guest NIC down150.082s/up500.083s, buffer depleted at120s media time;
no recovery after restore/seek before590.058s close. Shell returned,exact
PMM257743/SHM0/aliases0. Cold heap retention grew; long repeated-reopen
bound unverified. Search/select,expiry,error reporting remain open.

sources: exact online source snapshots hash-match provider report.
native-focused: bounded codec/controls/EOS/restart/exact cleanup fixtures;
these are not YouTube acceptance. Latest latency diagnostic fixture passed
23:32:34; later first-decoded-image logging correction compiled only.
Scoped timing/RAM logs expose scalar counters only; owned-page bytes are
not RSS. Broad RAM capture was rejected earlier and was never performed.
Published logs redact signed URL query/data payloads; raw serial private
under build. SHA256SUMS covers committed evidence. Original owner promotion
files are untouched. Full regression and further live diagnosis continue.
