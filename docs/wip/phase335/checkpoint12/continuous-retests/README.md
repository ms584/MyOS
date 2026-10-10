# Phase33.5 checkpoint12: retain continuous retest failures
Both source-built core runs completed successfully as observations, but their
continuous acceptance analyzer rejected them. No failure was converted to PASS.

VP9 provider1a29... kueg10ge:600 decoded,1 dropped,0 corrupted,presented601,
sync596/max89986us. Rejected by the zero-drop analyzer intended for the original
Phase33.3 VP8 declared profile. The VP9 candidate is not certified by that
analyzer and still needs its actual YouTube profile acceptance.

Original declared VP8 provider41901... 5fysd7x9/new source core:600 decoded,
zero dropped/corrupted; analyzer progressed to pixels, then rejected sampled
frame137. Actual PNG inspected: the binary strip is obscured/partially composited
while status/UI changes; fixed-pixel reader returned2 instead of continuing.
This is not verified playback failure or verified harmless observer failure;
no geometric or video result is invented. Preserve the image and raw run.
Both allocation histories have complete native512-tick coverage; facts in
failure-summary.json do not imply all RAM/audio/pixel gates passed.

Private fixture status box now has fixed50px height so the intended native
320x180 viewport remains at y182 for the exact existing reader. No runtime,
player, decoder, threshold or analyzer changed. Retest ACTIVE:
build/phase335-source-media-vp8-fixed-layout.log/PTY97538. This scoped fixture
does not establish that moving media bounds/compositing is correct; normal
YouTube movement/controls/reopen must still be inspected, and the captured
reflow case must be assessed separately before broader UI claims.

Normal-image YouTube observer now has explicit --source-core-manifest option.
It checks the original official-core audit first, validates source/core/patch/
log identity, and marks source-built provenance separately. Three focused
identity tests passed, including seven tamper/false-acceptance cases and
manifest path rejection. No new-core normal online run yet. Normal5 unchanged.
Phase33.5/33 and independent Phase32 remain open. No push.
