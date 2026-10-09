Updated:2026-10-09 15:20 +07. /home/xytron404/myos,main.
Baseline:091f300 local checkpoint5 worker-join/heap fix; no push.
Goal Phase33.3 ACTIVE; no push; preserve owner promotion logs/status/audits.

## Scope
Phase33.1/33.2 real native Cobalt HTTPS/JS/DOM/input prerequisite PASSED:
PHASE33_NATIVE_WEB_ACCEPTANCE.md. No imports-based acceptance.
Phase33.3 OPEN. No33.4/YouTube work/pass.
Pinned WebM/VP8/libvpx1.15.2/Opus1.5.2/licenses:PHASE333.md.
Asset160x90@15fps40s; QEMU512MiB/KVM/RTL8139/SB16,softpipe,jitlessJS.
Original gates:A/V<=100ms afterpreroll, incremental media<=32MiB.

## Checkpoint5 source and evidence
MRT thread_control rejected MID/HIGH stack handles; join could return before
Destroyed callback, then Cobalt context freed. Diagnostic caller stack identifies
PlayerStatusCB from native Destroyed callback. Restricted VM-range fix and
resident-page guard now in user/platform-runtime.c.
Old focused helper rejection and fixed MID/HIGH join/detach/getattr/guard/
relaunch exact resources PASS. Added tests/test_thread_vm_ranges.py tofullsuite.
memory/memory.c tracks exact rounded live heap +runtime highwater; existing
heap walk verifies counter equality; boot deliberate OOM followed by reset.
Three real engine controls/EOS40/fresh MSE restart/stop launches completed:
547/138,547/141,545/132 presentations;max sync53.234ms; noGP/panic.
All PMM/SHM/aliases reclaimed; heap+61440 cold profile,+4096 then exact540416.
Frozen candidate and production BIOS/kernel/MRT hashes IDENTICAL:
build/media-evaluation/thread-fix-normal-20261009/;checkpoint5/normal-identity.json.
Same-session PNGs/WAV,metrics/samples/safe serial:checkpoint5/.
Sampled accounted engine RAM28024848/20652048/18800672B under32MiB.
PMM sampled0.5s; short-peak allocation bound still pending,notRAMacceptance.
Native-format11025monoU8capture still heldsamples52.88ms;hardware underruns
273/269/292. Sound continuity/pitch gate OPEN; source-starvations0 insufficient.
Verified HTTPS8s range stall recovered,stop/responsiveness/PMM/SHM/aliases exact.
Cold profile heap+61440 asabove. Corrupt/truncate/close observations added.

## Active jobs and regression
Full regression5 PASS terminal0 14:58:06 normalBIOSrestored, OLD MRT+heapcounter.
Full regression6 ACTIVE since15:00,logbuild/media-evaluation/phase333-full-regression6.log.
PID/session must be checked live before waiting; do not restart ontimeout.
Root build outputs now belong tofull6; do not rebuild/mutate architecture source.
Truncate/corrupt native HTTPS errors detected, fresh stream recovery/stop passed;
PMM/SHM/aliases exact. Escape during playback also exact; partial cold profile
heap+40960B needs warm close/relaunch owner attribution.
Erroneous shell loop lost fault mode and is excluded; corrected Python loop DONE.
Playing OOM ArrayBuffer pressure ACTIVE session53496,cobalt-playing-oom1.log.
Native PCM666 long constant runs byte129, near zero; voiced periods near source.
Next investigate real starvation/single-packet callback delivery cadence.
Final UEFI/checks/normal-image restoration afterfull6 pending.

## Next gates
Diagnose audio held samples/hardware underruns; exact PMM highwater/bound.
Inspect actual guest during-playback OOM result, warm close/relaunch heap.
Finish focused+full regression/UEFI, restore normal images, update evidence/guide,
localcommit only. Nativefixturefaults do not substitute forenginefaults.
References:PHASE333.md,HANDOFF.md,wip/phase333/checkpoint5/README.md.
