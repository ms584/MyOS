Checkpoint7 2026-10-10 21:48+07: source27.lts.3 tooltip Aura-root patch applied; pinned dependencies/GN complete. Native source-core compile ACTIVE8workers (build/phase335-core-compile.log,PTY18106); prior6worker output reused. Private HTTPS/JS/standard-media/online source-core selectors prepared with hash checks, syntax only. No linked-core/runtime repair proof yet. Normal artifacts unchanged, owner listening/controls unconfirmed, Phase33.5/33 remain open. See wip/phase335/checkpoint7/tooltip-core-build/README.md.

Checkpoint6 confirmation2026-10-10 18:39:39+07: native HTTPS HTML-title fixture requested176×24 with primary window1, received ENOTSUP(-95), then FATAL/SIGABRT6. All16 core returnPCs match the captured online j_4b33rx failure exactly. Actual native JS ready/pointer callbacks recorded; pre-hover screenshot still splash, not proof of rendered HTML button. Exact physical/SHM/alias cleanup; heap786176→827136 disclosed. No production repair yet/no QEMU;33.5/33 remain open. See wip/phase335/checkpoint6/window-abort/actual-tooltip-result.json.

Checkpoint6 2026-10-10 18:20+07: private j_4b33rx controls run completed; third open FATAL/SIGABRT. Matching official symbols (returnPC−1) prove TooltipAura creation → PlatformWindowStarboard::Show. Current native UI rejects small windows and a second physical window; private Ring3 reproduction twice confirms ENOTSUP/EBUSY and exact cleanup. Actual tooltip dimensions are being measured separately. No production repair or acceptance yet; pinned Starboard permits fullscreen-only platforms, so engine/compositor adaptation must be correct rather than returning fake handles. See wip/phase335/checkpoint6/window-abort/README.md. Owner listening/signoff still absent;33.5/33 remain open.

# Phase33.5 experiment guide — no automatic acceptance

Run in WSL Ubuntu /home/xytron404/myos. Owner pushes manually. Read
PROJECT_STATE.md,PHASE335_ACCEPTANCE.md,PHASE33.md first. Every command uses
python3 tools/run_logged.py -- . No concurrent media VMs or shared-image rebuilds.

## Review evidence
wip/phase335/checkpoint1 holds public PNGs, original native PCM.gz, sanitized
serial/network metadata, timelines and screen-audio AVI.gz. Extract AVI.gz to
build for review. This is captured native output; its wall/audio mapping is
approximate, and failed sync runs must not be accepted. Never use these files
as media inputs to MyOS. Private encrypted PCAP/raw logs/assets remain under
build; do not publish signed URLs, cookies, headers, keys or payloads.

## Native automated trial
Use the tested provider recorded by the final checkpoint, not a guessed path.
Existing pinned resources:/home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n;
fonts:/home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007.
~~~sh
python3 tools/run_logged.py -- python3 tests/observe_phase335.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007 --provider build/media-evaluation/provider-3k2mykzc --identity docs/wip/phase334/checkpoint11/current-online/normal-identity.json --memory-mib 1024 --url 'https://www.youtube.com/watch?v=dQw4w9WgXcQ' --url 'https://www.youtube.com/watch?v=dQw4w9WgXcQ' --seconds 330 --actions build/phase335-actions-normal.json --desktop-check --ram --record --record-fps 1 --network-capture --inode-accounting
~~~
The final provider above is non-trace. Copy checkpoint1/phase335-actions-normal.json
into build before running. Ads/quality/Stats may change; inspect actual frames. Runner
packs native binaries/resources only; fetch,TLS,JS,demux,decode,render and sound
all execute in MyOS. Observation exit0 never means acceptance. Inspect frames
and timings before deciding whether scheduled input operated the intended UI.

## Owner live audiovisual/control check
For a completed native bundle printed by the runner:
~~~sh
python3 tools/run_logged.py -- python3 tools/run_phase335_manual.py build/phase334-public-g_wbd0r3
~~~
WSLg GTK/PulseAudio and non-root KVM are required. Available backends were
checked; actual live speaker output from this manual script is not yet verified.
QEMU GTK/PA only provides virtual display/sound hardware; it is not a host
browser/player/media decoder. The frozen boot is hash-equal to normal MyOS.

Inside MyOS type net up, mkdir /dev/shm, mem, then:
~~~text
elf cobalt cobalt provider https://www.youtube.com/watch?v=dQw4w9WgXcQ
~~~
This opens the native Ring3 Cobalt browser. The legacy desktop text-Browser
icon is a different app; its historical64KiB limit is not Cobalt's capability.
Select Settings→Quality→144p using MyOS mouse input. Confirm official title
and Stats video ID dQw4w9WgXcQ, actual codecs/resolution. Watch/listen>=30s.
K pause: picture and audible music stop; wait10s. K resume: both continue.
L forward10s, then J back10s: inspect timeline, new picture and new audio,
no stale output. ESC close; mem; desktop; open a window; Q returns shell.
Open the video again. Repeat>=3 opens/closes in one VM and sample mem/heap
boundary logs. Stop/restart QEMU for a fresh boot and repeat the full controls.
Record exact wall times, any buffering/drops, maximum RAM observation, cleanup,
audio quality and failures. No need to log account data or sign in.

Send your observed music/pause/resume/seek/close/fresh-boot results; no document
or script records your acceptance automatically. Close33.5/33 only when every
PHASE33.md gate and all unresolved technical tests actually pass.

Final tested manual bundle will be listed in the acceptance report after completion.
If an ad plays, wait or use the native Skip ad button; confirm Rickroll title/videoID
before counting30s. After offline Retry do not count another sponsored video as Rickroll.
Linked profile/cache retention is disclosed; compare heap and file capacity over warm
reopens. No current warm plateau or exact total RAM maximum is claimed.

Final fresh boot g_wbd0r3 two opens completed15:35:04+07; use its frozen
normal assets for manual trial. Commands remain logged; no push.
