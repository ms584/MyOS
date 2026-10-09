2026-10-10 05:26 +07 — checkpoint10/11; Phase33.4 ACTIVE/INCOMPLETE.
Branch main; baseline ea94696. Local commits only; owner pushes. No33.5/33/Rickroll closure.
Preserve owner untracked promotion logs/status/audits.

Verified prerequisites: native33.1/33.2 replay PASS2026-10-09 20:41;
original33.3 600frames/37.4s/40.062s PCM PASS20:41:56. Latest native
Cobalt HTTPS/JS/MSE standard recheck also passes below. No host production
fetch/TLS/browser/player/decoder/transcoder or converting proxy.

Actual public checkpoint10 (ea94696/provider-0w_9xkyk,3fvd7jf3):
45s movie motion and same-session native SB16 PCM; all four players actual
hardware-clock maximum <=89997us. Native144p256x144@30 VP9 SDR, Opus/WebM;
output160x90 in800x600, QEMU1GiB. Two pause/resume/seek and close/reopen,
130s actual NIC loss/restoration/seek recovery; both desktops/shell responsive,
exact257743 pages, SHM/aliases0. Peak owned mapping525533184B (not RSS).
Evidence: wip/phase334/checkpoint10/online-rebuffer-controls/.
Native search/result selection and actual WebM MIME independently observed:
checkpoint10/search-stream-format/. This older search run starts audio before
first SetBounds; disclose limitation, do not accept that startup.

Latest source90dfe4b6d6f45097bad559d8aecc0bb818aafd8d2e796e81b300643d545e8943
provider-sey3hj06 ba9620ca077d7942775d9ec9fcca0e8561e39c3989f17ae024e625d5122c2c85
holds PCM until first native video submission on creation/seek. Delayed-bounds
and video-gap fixture PASS: consumed0 at first frame, max39979us, exact resources.
Cobalt standard1vrufhf0 PASS05:22:53:38.533s motion14.975fps/39.953s PCM,
600decoded/drop0/corrupt0/starve0,actual sync39979us/hardware failures0,
resident exact. Evidence: checkpoint11/first-video-native and cobalt-standard.
Latest source actual public direct+search/reopen controls/recovery is PENDING.

Controlled native Cobalt XHR timeout10s/14s one-shot stall,restart/resources PASS;
native64MiB malformed VP8/VP9/Opus, unsupported codecs/dimensions, real ENOMEM,
two teardown/recreate waves and exact resources PASS. Checkpoint10/cobalt-timeout,
player-faults. Controlled403 checkpoint9 PASS; none proves live signed-URL expiry.

Full architecture regression checkpoint6 terminal0 at02:03:53;
normal UEFI/final launcher/pair BIOS+UEFI terminal0/all5 exact02:05:33.
Kernel/MRT/images remain exact; default VP8 binary remains41901a137421bf62...
New optional provider is separately focused/standard tested; public pending.

Live signed-URL expiry remains unverified. Auto-review rejected replay of a
private stored signed Googlevideo URL because credential/token reuse externally
requires explicit owner permission. No replay harness/action executed.
Do authorized current public test first; then request scoped permission if needed.
Read PHASE334.md, PHASE334_EXPERIMENT_GUIDE.md and HANDOFF.md for details.
