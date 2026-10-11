# Actual VP8 late-bounds regression

Original sealed VP8 provider41901... failed native late SetBounds fixture
2026-10-11T12:10:21+07:00: after2s with no video bounds, audio clock1663090us;
no first visible frame. Fixture reports audio-before-video, status7,25 drops.
Cleanup still exact; failure is not an accepted control run.

First-frame candidate7cd804... passed the identical fixture12:12:41:
clock held until real bounds, first video submissions consumed0 on initial/
seek/restart, controls/EOS and same-process restart/stop work; max sync9986/
9987us, zero drops/corruption, samples2380 all released, exact physical/SHM/
alias/heap. This directly verifies the first-frame ordering change; it does
not explain all earlier intermittent cold-start continuous drops/audio gaps.

The existing native test now permits --late-bounds with either VP8 or VP9
underlay (previously VP9-only). All actual clock/frame/sample/control assertions
are preserved. No acceptance threshold, PCM DMA API or kernel contract changed.
These are standard native assets, not online YouTube/Rickroll or listening.
Production player is still unchanged pending focused adverse results.
