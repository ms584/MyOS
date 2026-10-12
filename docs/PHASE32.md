# Phase 32 — YouTube search,selection and playback

**PASSED on the declared native MyOS/QEMU profile,2026-10-12.** This decision
uses an actual search-selected session rather than inferring search from a
direct Rickroll URL. The owner explicitly confirmed search→selection→playback
with sound in this chat on2026-10-12.

Native YouTube homepage search typed `Rick Astley Never Gonna Give You Up`,
selected the official result,used native Skip ad and reached `dQw4w9WgXcQ`.
39 changing movie samples and39.787 s native PCM were recorded; native Stats
confirmed VP9(243)/Opus(251). See
[wip/phase335/checkpoint22/normal-online/search-select-play-result.json](wip/phase335/checkpoint22/normal-online/search-select-play-result.json).
Its original pending human flags remain historical; the final owner confirmation
is in checkpoint22/pointer-boundary/owner-phase32-confirmation.json.

That search session uses the unchanged tested Cobalt core/media provider.
The later mouse boundary change is covered by BIOS/UEFI actual-coordinate,
button/recovery/underlay and full regression,plus normal online/fresh playback.
Controls,resources and audible owner acceptance are audited in
[PHASE335_ACCEPTANCE.md](PHASE335_ACCEPTANCE.md).

Fetch,TLS,JS,demux,decode,render and audio execute inside MyOS. The legacy64 KiB
text Browser is a separate limited application; the native Cobalt route provides
this YouTube capability. Host tools do not replace media processing.
Loading/UI delay,dropped frames and rebuffering remain. General Chrome parity,
all websites,accounts/DRM/Shorts and physical hardware are unverified.
Reproduce with [PHASE335_USER_GUIDE_TH.md](PHASE335_USER_GUIDE_TH.md).
Historical transport-only probes and preacceptance limits remain archived in
checkpoint22/pointer-boundary/pre-closure-documents.json.gz.
