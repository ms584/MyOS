# Phase33.4 final acceptance audit — pending real expiry
Prepared 2026-10-10 07:45 +07. This checklist does not close the phase.

| Required scope | Authoritative evidence / pending decision |
| --- | --- |
| Native HTTPS, JS/DOM/input and standard sound/controls prerequisites | Native web acceptance replay and checkpoint11 standard-media/late-bounds proofs referenced by PROJECT_STATE.md. |
| Public YouTube direct URL and native search/select/watch | checkpoint11/current-online/evidence.json, actions.json and human-reviewed PNGs; aqz-KE-bpKQ, two native launches. |
| >=30s motion and sound in the same MyOS/QEMU session | checkpoint11/current-online/native-observation-analysis.json, PNGs, audio WAV.gz and serial; motion 50.447s and 30.283s. |
| Pause both, resume, seek, close/reopen, responsive shell/desktop | Same checkpoint11 actions/metrics, actual PCM_STATUS and unchanged pause pixels; exact post-close memory/SHM/aliases. |
| Native media interfaces, selected streams, bounded buffers and sync | Source/provider identities linked to checkpoint11; native VP9/Opus WebM SABR, measured actual hardware-clock error <=89970us. No unsupported format capability claim. |
| Public TLS, media hosts, concurrent requests and stream requests | Native frontend/public playback plus prerequisite certificate negatives; checkpoint12 native SABR POST records. No media-host mapping, TLS bypass or host media transport. Selected SABR transport is recorded rather than assumed to use GET ranges. |
| Loss, timeout, unsupported/corrupt format and return to MyOS | checkpoint11 current-online/current-timeout/current-faults/current-http403 and final-checks. |
| Authentic expired URL and recovery | PENDING: live native-verified-tls-lz6fldt5, session81701. Exact original URI and 1663-byte native body; pre-expiry peer HTTP200 verified. Repeat at real 12:53:09 +07; require peer expired response, native recovery motion/audio, exact close/desktop resources. CORS status0 alone is insufficient. |
| Date, URL, commit/build identity, QEMU command, format/resolution/fps/RAM, captures/logs | checkpoint11 archived identities/evidence/scope/SHA256SUMS; checkpoint12 diagnostic identity limitations disclosed. Final expiry capture still pending. |
| Focused checks, applicable full architecture regression, restored normal image | checkpoint11 final-checks plus checkpoint6 full architecture regression; production architecture unchanged. Recheck all five normal identities when live expiry exits. |
| Documentation, local commit, ready for33.5 | Existing checkpoint commits local only; final evidence/handoff commit pending. No push, no33.5/33/Rickroll acceptance. |

The scoped expiry auditor requires actual native body SHA, same URI POST,
pre-expiry HTTP200 then post-expiry HTTP403 with server Date, native UTC,
>=30s changing movie-region pixels and audible native PCM, <=100ms actual
hardware A/V error, exact cleanup and normal-image hashes. Human inspection
must confirm these are real YouTube frames; changing pixels alone cannot pass.
The auditor deliberately leaves phase_passed=false and cannot replace the
whole-phase audit or human review.
