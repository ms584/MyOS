# Phase33.4 experiment guide
Status: IN PROGRESS, NOT PASSED. Run in WSL Ubuntu /home/xytron404/myos.
Every command below uses tools/run_logged.py. Owner pushes manually.
Read PROJECT_STATE.md and PHASE334.md before running; do not run concurrent
media acceptance VMs or mutate normal images while a suite uses them.

## Verify prerequisite identity
Current accepted baseline is PHASE333_ACCEPTANCE.md/checkpoint6.
Original baseline matched all5 artifacts. For later launcher/thread builds, use
the current checkpoint normal identity; do not compare altered source to old hashes.
Replay recorded web prerequisite (not a new engine run):
~~~sh
python3 tools/run_logged.py -- bash docs/wip/native-pmm512/integration-20261008/review_public_native_20261008.sh
python3 tools/run_logged.py -- python3 tools/analyze_media_session.py build/native-verified-tls-yzub9bzc --provider build/media-evaluation/provider-i8xbwfmz/provider.so --renderer build/renderer-provider-evaluation/provider-84sokh6g/provider.so
~~~
This source baseline uses QEMU512MiB,non-root KVM/CPUhost,RTL8139,SB16.
Known pinned core/resources/fonts directories appear below. Use their actual
hash-verified reports; do not substitute files or disable TLS verification.

## Original public-site observation
~~~sh
python3 tools/run_logged.py -- python3 tests/test_cobalt_youtube_observation.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 --url https://www.youtube.com/ --url 'https://www.youtube.com/watch?v=jNQXAC9IVRw' --url https://www.youtube.com/tv --seconds 180
~~~
Recorded: public home/search/menu,watch browser-can-not-play error,
TV Welcome/Watch as guest; zero PCM frames. jNQXAC9IVRw is19s and cannot
satisfy the>=30s gate even if it plays.
Runner creates a private build/phase334-public-* bundle, freezes normal
image/runtime and assets, records original QEMU args/hash/date/URLs/serial/PNG/WAV,
requires Escape/shell mem before relaunch, and verifies normal hashes after.
No host page/media fetch, extra fixture CA, media proxy or credentials.
Runner exit0 means observation completed, never Phase33.4 acceptance.

## Candidate VP9 build and original synthetic fixture
Native decoder archive is separate; default VP8 remains available.
~~~sh
python3 tools/run_logged.py -- python3 tools/build_media_codecs.py --vp9
python3 tools/run_logged.py -- bash -lc 'mkdir -p build/media-evaluation/fixture-vp9-encoder && cd build/media-evaluation/fixture-vp9-encoder && ../libvpx-1.15.2/configure --target=generic-gnu --disable-vp8 --enable-vp9 --disable-vp9-decoder --disable-multithread --disable-runtime-cpu-detect --disable-examples --disable-tools --disable-docs --disable-unit-tests --disable-shared --enable-pic && make -j4 libvpx.a'
python3 tools/run_logged.py -- python3 tools/generate_media_fixture.py --vp9
python3 tools/run_logged.py -- python3 tools/build_media_provider.py --native-underlay --vp9
~~~
Host fixture build is ENCODER ONLY: verify CONFIG_VP8_DECODER=0 and
CONFIG_VP9_DECODER=0. It encodes original raw moving cells/sine PCM, never
fetches or transforms YouTube video. Guest decoder libvpx1.15.2 uses
CONFIG_SIZE_LIMIT=1,640x360,profile0/8bit,no highbitdepth,no multithreading.
Guest presentation downsamples internally to at most160x90 underlay.
640x360/30fps is a candidate ceiling, not yet a measured accepted profile.
Opus1.5.2 and all source/archive/license pins stay in PHASE333_MEDIA_LOCK.json.
Builder prints a new provider directory. Substitute the actual emitted path:
~~~sh
python3 tools/run_logged.py -- python3 tests/test_media_native_player.py --provider build/media-evaluation/provider-xrjp9qef --vp9 --underlay --kvm
~~~
This focused test runs native read/demux/decode/render/audio, pause/resume,
seek15s/EOS/restart/stop, actual changed guest viewport/PCM and exact resources.
It requires native underlay sync<=100ms. Recorded PASS9.987ms.
Omitting --underlay selects texture mode; two recorded runs FAILED113.734/
111.163ms. Keep those failures; texture performance is not accepted.
Neither fixture route is YouTube or Phase33.4 acceptance.

## Diagnostic real watch and input
~~~sh
python3 tools/run_logged.py -- python3 tools/build_media_provider.py --native-underlay --vp9 --trace
python3 tools/run_logged.py -- python3 tests/test_cobalt_youtube_observation.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 --provider build/media-evaluation/provider-z73ft99k --url 'https://www.youtube.com/watch?v=aqz-KE-bpKQ' --seconds 300 --input 180:k --input 195:k --input 220:l --ram
~~~
Use actual emitted provider path. This diagnostic logs bounded MIME/support
queries and player events inside MyOS; key delivery alone does not prove controls.
Inspect actual pictures, native consumed audio/IRQ counters and timeline changes.
--ram observes PMM with brief QMP pauses; sampled PMM includes kernel/residents,
excludes reserved heap backing, is not exact media peak or per-process RSS.
Trace/sampling run is diagnostic; final timing acceptance needs appropriate
measurement and non-diagnostic provider. No fixed elapsed schedule is a pass.

TV guest feasibility:
~~~sh
python3 tools/run_logged.py -- python3 tests/test_cobalt_youtube_observation.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 --provider build/media-evaluation/provider-xcwnouzd --url 'https://www.youtube.com/tv#/watch?v=aqz-KE-bpKQ' --seconds 240 --input 170:ret --ram
~~~
Native Shift+3 types #. Recorded Enter passes Watch as guest to home;
this fragment did not open playback. No certification rejection is proved.
Signing/property APIs honestly remain unavailable; do not invent keys/scopes.

## Final gate remains pending
Real online YouTube>=30s moving video plus same-session audio; actual pause/
resume/seek; close/reopen responsive shell; search/select path; media host TLS/
redirect/range/concurrency; bounded RAM/buffering and network error recovery.
Publish original dated URL/commit/build/QEMU/profile/format/resolution/fps,
frame/audio/control/network/serial evidence. Keep private assets/raw heaps/
keys out of commits. Preserve unsuccessful experiments and truthful limitations.
Run focused and applicable full regression, restore normal BIOS/UEFI images,
update PROJECT_STATE/HANDOFF and commit locally. No push.
Phase33.5/Rickroll and parent Phase33 remain separate unaccepted gates.



## Scoped engine trace and worker checkpoint
Normal elf cobalt remains unchanged. elf cobalt-trace adds stderr logging/v1 only,
keeps jitless and verified HTTPS; no debugger, heap dump, policy/auth bypass.
New normal builds require their own five-artifact identity JSON.
~~~sh
python3 tools/run_logged.py -- bash build.sh
python3 tools/run_logged.py -- python3 tools/build_uefi.py
python3 tools/run_logged.py -- python3 tests/test_thread_capacity.py
python3 tools/run_logged.py -- python3 tests/test_thread_capacity.py --uefi
python3 tools/run_logged.py -- python3 tests/test_native_tls.py
python3 tools/run_logged.py -- python3 tests/test_native_tls.py --uefi
python3 tools/run_logged.py -- python3 tests/test_cobalt_youtube_observation.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 --provider build/media-evaluation/provider-uve_judw --url 'https://www.youtube.com/watch?v=aqz-KE-bpKQ' --seconds 300 --identity build/phase334-workers40-normal-identity.json --engine-trace
python3 tools/run_logged.py -- python3 tests/test_system.py
python3 tools/run_logged.py -- python3 tools/build_uefi.py
~~~
Use actual provider/identity emitted by checkpoint. Native trace includes real
URL queries: keep raw private under build and redact queries/data payloads before
committing public evidence. Never collect full guest RAM to diagnose pages.
Current VP9 full suffix only default SDR .01.01.01.01.00; no HDR/10bit/range claim.
Worker40 is bounded, saturation must return EAGAIN and resources must recover.


## Native quality/search input actions
Runner --actions PATH reads a local JSON list of unique id/type/at records.
type=click uses bounded PS/2 relative deltas and left button at x/y800x600,
type=text types at most64 ASCII characters through real key events,
type=key delivers supported native qcode. No DOM/JS injection, memory/heap
extraction or host page/network fetch. Native events and actual elapsed times
are copied into evidence. Edit action file while diagnostic run is active
to operate the observed guest UI; inspect native PNG after each step.
The small320x240 capability experiment exposes a frontend assumption:
rejecting640x360 probe makes this version omit dimensions and choose unsupported
streams. Manual native quality selection must be evaluated; never claim success
from capability responses or key delivery alone.

## Checkpoint4 candidate: actual1GiB diagnostic profile
New native PMM1024 ceiling is E820-constrained; QEMU --memory-mib1024
must be explicit. Defaults remain512. Focused low/high RAM BIOS/UEFI passed,
new full regression pending. Native process owned-page byte counters are
not RSS; --ram uses read-only PMM counters with brief pauses and is diagnostic.
Use --provider DIRECTORY (not provider.so). --actions JSON accepts native
click/text/key, per-launch close, and isolated virtual NIC link up/down.
No DOM scripting or guest heap capture. Current actions recorded under build;
quality is selected through visible YouTube Settings/Quality144p menu.
No engine --engine-trace for performance comparison; scoped media trace remains
diagnostic and cannot alone establish acceptance. Native1GiB trial changes
both memory envelope and stream-capability profile, so no isolated causality
claim. Codec guard/advertised maximum640x360@30; downscaled presentation160x90.

Checkpoint4 completion2026-10-10 00:17 +07: new PMM1024 full default
system suite passed00:15:27; normalUEFI/final launcher/pair BIOS+UEFI
passed, all5 normal identities exact, runner exit0. Evidence:
wip/phase334/checkpoint4/regression. Earlier running/pending statements
above are historical. YouTube performance/recovery acceptance still open.


### 2026-10-10 01:03 +07 — checkpoint 6; Phase 33.4 remains incomplete

VM-area 1024 eliminated the observed public-session reservation ENOMEM. Native YouTube
144p Stats proves 256x144@30 VP9/Opus; search/select/reopen and both post-close desktops worked.
A/V max 560083 us overall / 351299 us at 144p still FAILS. The bounded VP9 batch candidate
passes native fixture controls/EOS/cleanup; its public sync is pending. Actual idle NIC-loss
baseline was reproduced; ENETDOWN/waiter/reconnect fix passes native BIOS and UEFI.
Full regression is PENDING for VM-area 1024 + carrier fix. Earlier PMM full PASS does not cover it.
See `docs/PHASE334.md`, `docs/HANDOFF.md` and `docs/wip/phase334/checkpoint6/` for commands,
source identities, evidence and remaining online recovery/sync gates. No Phase 33.5/33/Rickroll acceptance.

Focused checkpoint-6 reproduction:
```bash
python3 tools/run_logged.py -- python3 tests/test_tcp_state.py
python3 tools/run_logged.py -- python3 tests/test_tcp_link_loss.py
python3 tools/run_logged.py -- python3 tests/test_tcp_link_loss.py --uefi
python3 tools/run_logged.py -- python3 tests/test_media_native_player.py --provider build/media-evaluation/provider-aml2hg6g --vp9 --underlay --kvm
```
`--expect-missing` is only for the archived pre-fix kernel; it reproduces the defect and
never claims a fixed transport pass. Use the matching normal BIOS/UEFI identities.
