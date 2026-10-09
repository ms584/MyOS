# Phase33.3 standard media experiment guide

Run in WSL Ubuntu from /home/xytron404/myos. This guide exercises native MyOS
media and collects QEMU framebuffer/SB16 evidence. It does not test YouTube.
[Scope and pins](PHASE333.md), [measured checkpoint6](wip/phase333/checkpoint6/README.md).

## Profile and prerequisites
160x90 VP8 at15fps, WebM+Opus40s fixture, native11025Hz mono SB16 output.
QEMU512MiB, KVM CPUhost and RTL8139. Original limits:>=30s moving video+sound,
presented-video vs consumed-audio timestamp error<=100ms after preroll,
incremental media RAM<=32MiB. Renderer/core startup memory is the controlled
baseline; media code mapped before baseline is added separately.
The pinned Cobalt core, accepted renderer provider/SDK and sealed fonts/resources
must already be prepared as described in PHASE33_NATIVE_WEB_ACCEPTANCE.md.
Resource reports must match PHASE33_COBALT_AUDIT.json; the runner verifies them.
Recorded resource/font paths are used below; substitute fresh emitted paths.

Do not run concurrent media acceptance VMs. The first concurrent asset run
exceeded100ms; its failure is retained. Do not rebuild shared root images while
the full system suite uses them. Observation runners freeze their own copies.

## Build and freeze normal images
Use tools/run_logged.py for every development command:
~~~sh
python3 tools/run_logged.py -- python3 tools/fetch_media_sources.py
python3 tools/run_logged.py -- python3 tools/build_media_codecs.py
python3 tools/run_logged.py -- python3 tools/build_media_provider.py --native-underlay
python3 tools/run_logged.py -- bash build.sh
python3 tools/run_logged.py -- python3 tools/build_uefi.py
python3 tools/run_logged.py -- mkdir -p build/media-evaluation/manual-normal
python3 tools/run_logged.py -- cp build/myos-current.img build/myos-uefi.img build/kernel.elf build/kernel.bin build/platform-runtime.mrt build/media-evaluation/manual-normal/
~~~
The builder prints a new provider directory. Use it below.
Recorded production provider:build/media-evaluation/provider-i8xbwfmz.
Kernel ELF must match the frozen image when reading counters.

## Local standard asset first
~~~sh
python3 tools/run_logged.py -- python3 tests/test_media_native_player.py --provider build/media-evaluation/provider-i8xbwfmz --boot-image build/media-evaluation/manual-normal/myos-current.img --runtime build/media-evaluation/manual-normal/platform-runtime.mrt --kvm
~~~
This executes native file reads, bounded WebM demux, VP8/Opus decoding,
decode-target rendering, SB16, pause/resume, seek15s, EOS, restart and stop.
It asserts captured tones>=30s, changing textures, <=100ms sync,
sample ownership and exact resources. It is not the engine proof.

## Controlled verified HTTPS and real engine
Configure the actual emitted paths:
~~~sh
export MYOS_RENDERER_PROVIDER_PATH=/home/xytron404/myos/build/media-evaluation/provider-i8xbwfmz
export MYOS_MEDIA_BOOT_IMAGE=/home/xytron404/myos/build/media-evaluation/manual-normal/myos-current.img
export MYOS_MEDIA_RUNTIME_PATH=/home/xytron404/myos/build/media-evaluation/manual-normal/platform-runtime.mrt
export MYOS_MEDIA_KERNEL_ELF=/home/xytron404/myos/build/media-evaluation/manual-normal/kernel.elf
export MYOS_MEDIA_NATIVE_PCM_CAPTURE=1
export MYOS_MEDIA_RAM_DIAGNOSTIC=1
~~~
These select frozen inputs and passive capture. RAM diagnostics read counters
using QMP; they do not modify memory. Native PCM capture records actual11025Hz
monoU8 hardware output without host resampling/decoding.

Run one uninterrupted40s session:
~~~sh
MYOS_MEDIA_CONTINUOUS=1 python3 tools/run_logged.py -- python3 tests/test_cobalt_media_observation.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007
~~~
The host serves original WebM bytes over controlled HTTPS. MyOS/Cobalt verifies
the fixture CA through ordinary trust-store assets, runs JS/MSE, fetches8KiB
ranges, demuxes/decodes, renders and plays audio inside the guest.
No verifier bypass, host browser/player/decoder/transcoder is used.
Allow renderer startup; the recorded cold run took about110s before playback.
The controlled-start B input establishes the renderer-only baseline first.

The runner prints EVIDENCE_BUNDLE. Substitute its directory:
~~~sh
python3 tools/run_logged.py -- python3 tools/analyze_media_session.py build/native-verified-tls-yzub9bzc --provider build/media-evaluation/provider-i8xbwfmz/provider.so --renderer build/renderer-provider-evaluation/provider-84sokh6g/provider.so
~~~
The analyzer reads captured pixels/PCM/counters, not compressed media. It checks
600 decoded frames, moving binary frame identities, tone windows, sync,
complete PMM history coverage and conservative media RAM<=33554432B.
Observation exit0 alone is not acceptance. Whole-phase acceptance also needs
controls, adverse cases, full regression and normal-image restoration.

Run actual controls and three launches:
~~~sh
MYOS_MEDIA_LIFECYCLE=1 MYOS_MEDIA_REPEATS=3 python3 tools/run_logged.py -- python3 tests/test_cobalt_media_observation.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007
~~~
The native page receives P pause, R resume, S seek15s, N fresh MediaSource restart
after EOS and X stop. The runner sends actual MyOS input. Escape closes Cobalt.
Evidence shows frozen audio-consumed/IRQ counters while paused, advancing after
resume, new decoded frame IDs after seek, cleared video/halted audio after stop.
WAV captures active device time and omits stopped-DMA intervals; its sample
index is not uninterrupted host wall time. Screenshot status text can change
the video's position; compare the actual viewport, not fixed coordinates.

## Adverse cases and resource checks
Run each separately with the frozen environment:
~~~sh
MYOS_MEDIA_FAULT=stall python3 tools/run_logged.py -- python3 tests/test_cobalt_media_observation.py /home/xytron404/cd-QJYMwG/build/cobalt-resources-dwxalc2n /home/xytron404/platform-fonts-01W7Ri/build/platform-fonts-20261007
~~~
Replace stall with truncate or corrupt. Stall delays one range8s. Truncate
closes before the declared response length. Corrupt replaces WebM header bytes.
Errors must be detected natively; recovery uses N then X through MyOS.
Use MYOS_MEDIA_FAULT=close or oom with MYOS_MEDIA_REPEATS=2 and
MYOS_MEDIA_HEAP_DIAGNOSTIC=1 for warm close/OOM/relaunch ownership checks.
OOM allocates/touches native JS ArrayBuffers during playback; verify actual
pressure, responsive Escape/shell mem and reclaimed resources.
Latest OOM catches RangeError but playback later stalls with allocation/network
errors; cancellation/relaunch pass. Continued playback under OOM is not promised.

Raw heaps/TLS keys stay in build. Publish sanitized inode ownership summaries,
serial/JSON, PNGs/WAV and hashes. Linked profile files are persistent filesystem
state, distinct from outstanding player buffers. Warm close/OOM repeat heap
is exact in the recorded latest tests, as are PMM/SHM/aliases.

Focused failures and allocation-history checks:
~~~sh
python3 tools/run_logged.py -- python3 tests/test_media_player_faults.py build/media-evaluation/provider-i8xbwfmz
python3 tools/run_logged.py -- python3 tests/test_pmm_pressure_history.py --image-dir build/media-evaluation/manual-normal
python3 tools/run_logged.py -- python3 tests/test_pmm_pressure_history.py --image-dir build/media-evaluation/manual-normal --uefi
~~~
The fault runner inherits MYOS_MEDIA_BOOT_IMAGE/MYOS_MEDIA_RUNTIME_PATH above.

## Finish the architectural checkpoint
~~~sh
python3 tools/run_logged.py -- python3 tests/test_system.py
python3 tools/run_logged.py -- python3 tools/build_uefi.py
~~~
Wait for the suite's terminal PASS and normal framebuffer restoration.
The UEFI builder restores normal BIOS too. Record image/kernel/MRT hashes,
update PROJECT_STATE/HANDOFF and commit locally. Owner pushes manually.
Phase33.4/YouTube and parent Rickroll acceptance remain separate.
