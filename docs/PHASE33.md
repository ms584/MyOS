# Phase 33 — Rickroll end-to-end acceptance

**PASSED on the declared native MyOS/QEMU profile,2026-10-12.** Phase33.1–33.5
are complete for this profile. The owner explicitly accepted A/V sync<=500 ms
and separately confirmed Phase32 search→selection→playback with sound.

MyOS opened the real online Rickroll URL, displayed moving frames and output
same-session audio for>=30 s, paused/resumed both, sought forward/backward,
closed/reopened with responsive desktop/shell and repeated after a fresh boot.
Network,TLS,JavaScript,demux,decode,render and audio all execute inside MyOS.

[PHASE335_ACCEPTANCE.md](PHASE335_ACCEPTANCE.md) contains the requirement audit,
actual normal build hashes, engine/codecs, screen/audio/control evidence,
network/serial summaries, RAM/recovery and focused/full regression results.
Latest maxima:204.618/140.037/87.339 ms, all below the owner500 ms limit.
Historical100 ms failures remain archived and are not rewritten as passes.

The accepted configuration uses QEMU/KVM1024 MiB, SDL800x600, RTL8139 andSB16.
Loading/UI delay,dropped frames,rebuffering,low-resolution decode and retained
linked cache remain documented. This closes the online Rickroll gate; it does
not claim general Chrome capability or real hardware/USB readiness.

Use [PHASE335_USER_GUIDE_TH.md](PHASE335_USER_GUIDE_TH.md) to launch from Windows
and reproduce URL/search,playback,controls,reopen and fresh boot.
Phase32 was reviewed separately; see [PHASE32.md](PHASE32.md).
Earlier narrative and failures are preserved in checkpoint directories and
wip/phase335/checkpoint22/pointer-boundary/pre-closure-documents.json.gz.
