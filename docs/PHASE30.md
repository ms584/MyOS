# Phase 30 — Audio playback (QEMU baseline)

MyOS now drives QEMU's Sound Blaster 16 ISA device at I/O 0x220, IRQ 5,
DMA channel 1. The kernel plays **unsigned 8-bit mono PCM at 11025 Hz**.
A 16 KiB DMA ring has two 8 KiB halves; each device interrupt refills the
completed half from a bounded 64 KiB software queue. An empty queue produces
silence, increments the underrun counter and accepts later audio without a
reboot. This is a QEMU baseline, not proof of support for other sound cards or
physical machines. The driver runs in kernel mode; it is not a Ring 3 app.

## Run and hear or capture it

In WSL, from `~/myos`, build the normal image and start QEMU with the SB16:

```bash
python3 tools/run_logged.py -- bash build.sh
qemu-system-x86_64 -m 64M \
  -drive format=raw,file=build/myos-current.img,if=floppy -boot order=a \
  -audiodev wav,id=snd,path=build/phase30-demo.wav -device sb16,audiodev=snd
```

At `myos>`, try:

```text
audio status
audio tone 440 500
audio volume 25
audio tone 660 500
audio mute on
audio tone 440 500
audio mute off
audio stop
```

Close QEMU to finalize `build/phase30-demo.wav`, then play that file on the
host. The WAV backend captures the emulated output, including silent gaps.
If a WSL audio server is configured, `-audiodev pa,id=snd` can be used
instead of `wav` for live output. No audio device is added by default;
`audio status` then reports `no SB16 device` and ordinary boot remains
usable.

## Play a WAV file

MyOS accepts RIFF/WAVE PCM with one channel, 11025 samples/s and 8 bits
per sample. Its file and playback queue limits cap each WAV at 64 KiB.
Use a **new** disk image to copy a compatible file into `/home`:

```bash
python3 tools/make_audio_disk.py build/audio-data.img sample.wav
qemu-system-x86_64 -m 64M \
  -drive format=raw,file=build/myos-current.img,if=floppy -boot order=a \
  -drive format=raw,file=build/audio-data.img,if=ide,index=0 \
  -audiodev wav,id=snd,path=build/phase30-file.wav -device sb16,audiodev=snd
```

Then use `audio play /home/sample.wav` in the Shell. The import tool refuses
to overwrite an existing image; it does not alter a user's saved data disk.
Unsupported formats and malformed files are rejected. Volume is 0–100 and
mute affects newly refilled DMA halves, so a change may take up to roughly
1.5 seconds to reach the captured output. `audio stop` halts DMA and clears
queued samples.

## Verification and limits

```bash
python3 tools/run_logged.py -- python3 tests/test_audio.py
python3 tools/run_logged.py -- python3 tests/test_system.py
python3 tools/run_logged.py -- python3 tools/release.py build
python3 tools/run_logged.py -- python3 tests/test_audio_packages.py
```

The QEMU tests check absent-device handling, output captured to a real WAV,
audible signal level, silent tail after mute/stop, IRQ progress, recovery
after an empty queue, playback of an imported WAV from MyOS storage, and
audio from packaged BIOS and UEFI images.
Commands are recorded in `build/activity.log`. The format is deliberately
narrow; sample-rate conversion, stereo, 16-bit PCM, compressed codecs,
mixing multiple clients, audio through the browser, and physical hardware
remain future work. Phase 31 will build media timing on this audio baseline.

QEMU documents the [SB16 device and audio backend](https://github.com/qemu/qemu/blob/master/docs/qdev-device-use.txt);
its [device implementation](https://qemu.googlesource.com/qemu/+/9628ad8a6efbf097046f84898a2409ac357b22a3/hw/audio/sb16.c)
is the primary reference for the emulated DMA commands and interrupt
acknowledgment.
