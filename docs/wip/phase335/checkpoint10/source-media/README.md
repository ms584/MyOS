# Phase33.5 checkpoint10: source-core standard-media observation
Private n97q9_ey completed2026-10-11T03:59:34+07:00, terminal0.
QEMU10.2.1/KVM/CPUhost/512MiB/RTL8139/SB16/800x600, native mono11025Hz U8 capture.
Same source-core SHA72a802f90fddd071971bca252a00ffbc824c3ef48ecc19197e6b6c0665848505
and sealed VP9 provider as checkpoint9; fixture WebM VP8/Opus hash in evidence.json.
Guest verified HTTPS/range fetch, JS/MSE/demux/decode/render/audio; host only
serves fixture bytes and records display/PCM. Not Rickroll or YouTube acceptance.

Actual inspected PNGs show native video, paused time5.995502, resume6.005706,
seek15.020083 and later clock33.831942. Dynamic status changes viewport top157/182:
fixed-coordinate frame reading is invalid for transition images. Actual viewport
frame IDs90/91→105/136/166→239/270/...575 show resumed motion and forward seek.
Only one settled pause image was sampled: insufficient to prove entire pause
freeze. PCM36.928707s, U8 RMS55.198; this aggregate does not prove pause/resume
audio timing or music identity. No claim of actual listening.

Native metrics presented563, decoded572, dropped1, corrupted0, sync-samples550,
max-sync89956us, starvations0. Physical126671→126671, SHM0→0, aliases0→0;
heap786176→847616. Shell/mem responsive. This is a scoped observation, not full
33.3/33.4 pass or repeated-open stability.

Follow-up yx51f7lc currently running, PTY93130/log build/phase335-source-media-controls.log:
private observer now pairs each screencapture with actual WAV file size and
extends pause to10s. This changes evidence collection/test schedule only, not
runtime/player. Results pending. Remaining continuous/lifecycle/adverse tests,
normal online Rickroll, freshboot/resources/regression and owner confirmation
remain required. Phase33.5/33 and independent Phase32 open; no push.
