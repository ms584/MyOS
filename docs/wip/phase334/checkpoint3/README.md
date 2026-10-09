# Phase33.4 checkpoint3: full VP9 negotiation and native worker pressure
2026-10-09 +07. IN PROGRESS, NOT PASSED. Local main after a370fe6.
No host YouTube playback/download/decode/proxy, fake credentials or CSP/TLS bypass.

## Evidence
- vp9-short-online: Big Buck Bunny300s, VP9 short capability supported but
  full codec rejected; zero PCM. Input k/k/l delivered, not control acceptance.
- engine-trace-before-full-codec: real native watch, opt-in stderr/VLOG proves
  VP9 full codec192x144/320x240 rejected; worker EAGAIN, CSP/cache/socket warnings.
  Zero PCM, exact shell/resource cleanup. Observation finished early after shell
  return; no playback acceptance.
- vp9-full-codec-focused: actual native VP9 underlay40s original synthetic fixture,
  full codec positive/negative checks, pause/resume/seek/EOS/restart/stop,
 526frames,9.987ms max sync,8,708,096B sampled increment, zero drops/corruption,
 2380 samples released, exact PMM/SHM/aliases/heap; real PNG and same-session PCM.
 This is a fixture, not YouTube.
- full-codec-online-workers32: actual YouTube BBB240s, now accepts full SDR VP9,
 chooses640x360@30 and attempts SbPlayerCreate. MEDIA create resource failure,
 concurrent pthread_create EAGAIN, zero PCM; screenshot spinner0:00/10:34.
 Controls key delivery/seek telemetry is not proof of native audio/video controls.

## Implementation
Explicit elf cobalt-trace only adds --enable-logging=stderr and --v=1 after
mandatory jitless/HTTPS flags. Normal launcher unchanged. BIOS/UEFI focused
owned readonly argv/NULL/host-route validation/lifecycle checks passed.
Native VP9 parser accepts optional complete default SDR suffix .01.01.01.01.00
under MYOS_MEDIA_VP9 only; rejects incomplete/HDR/10bit/full-range strings.
Binding source: https://www.webmproject.org/vp9/mp4/
Default VP8 provider rebuilt byte-identical41901a137421bf62e63585f8ea5339fd5b3addb50471b4129a5ea378b1514696.

A proposed full guest RAM diagnostic was rejected by automatic approval review
due to broad sensitive-data capture; it was never executed and removed.
Scoped native log tracing is the replacement. No raw heaps or signed URL query
values are published; raw serial remains private build, redacted serial/hash here.

Current next: bounded40 workers (within existing48-page runtime TLS reservation),
saturation/TLS/reuse/cleanup PASS BIOS+UEFI21:44:05, new online attempt, full default
test_system regression and normal BIOS/UEFI restoration. All final33.4 gates
remain pending. No33.5/Rickroll/parent phase pass or push.


2026-10-09 21:59 +07 update:40 workers fixes native create. Actual BBB YouTube
VP9 640x360@30 decoded/rendered in MyOS, PNG motion and native same-session
11025Hz monoU8 WAV2021508frames/183.357s. Pause counters fixed, resume advances,
seek changes ticket/position; Escape shell/resources exact. NOT PASSED:
post-resume sync outlier623121us exceeds100ms candidate gate.
All raw/PNG/PCM preserved in online640-sync-failed; no fake success.
Small320x240 bound focused PASS native-player-qjz8d23k. Public frontend omits
dimensions after width640/height360 probe rejection, selects unsupported output;
SbPlayerBridge invalid mode, zero PCM. online-small-bound-frontend-failed.
Next quality selection through real native PS/2 UI, no payload/capability bypass.
Full default regression started21:58:23, running; normal BIOS restore + UEFI and
final launcher checks queued. Phase33.4/33.5/Rickroll/parent remain NOT PASSED.
