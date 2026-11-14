# MyOS — native desktop and online YouTube in QEMU

MyOS is an x86-64 operating system with BIOS/UEFI boot,framebuffer desktop,
PS/2 and optional virtual absolute input,preemptive scheduling,Ring3 ELF apps,
a bounded syscall/runtime interface,RAMFS,RTL8139 networking,verified HTTPS
and SB16 audio. It is developed and tested in QEMU before physical deployment.

**2026-10-12: Phase32,Phase33.5 and Phase33 accepted on the declared QEMU profile.**
Native Cobalt searches YouTube,selects an actual result and plays online Rickroll
with picture and sound inside MyOS. Pause,resume,forward/backward seek,
close/reopen,desktop response and fresh boot have recorded evidence.
The owner explicitly accepts A/V sync<=500 ms; final measured maxima are
204.618/140.037/87.339 ms. Historical100 ms failures remain recorded.

The complete native path is MyOS network/TLS→Cobalt JavaScript/media→WebM
VP9/Opus demux/decode→native video plane/SB16 audio. Host browsers,players,
YouTube decoders/transcoders and prepared MYV1 clips do not substitute for it.
The older bounded HTTPS text Browser remains a separate limited application.

## Start from Windows

Open Windows Terminal/PowerShell:

```powershell
wsl.exe -d Ubuntu
```

In Ubuntu on the tested development machine:

```bash
cd /home/xytron404/myos
python3 tools/run_logged.py -- python3 tools/run_phase335_manual.py build/phase335-pointer-boundary-manual --qmp --sdl-diagnostic --sdl-show-host-cursor --network-evidence
```

Inside MyOS at `myos>`:

```text
net up
mkdir /dev/shm
mouse absolute
elf cobalt cobalt provider https://www.youtube.com/watch?v=dQw4w9WgXcQ
```

K pauses/resumes,L seeks forward10 s,J backward10 s,Esc closes the browser.
The launcher uses QEMU/KVM1024 MiB,SDL800x600,RTL8139 andSB16/PulseAudio.
Keep one VM running. The native asset bundle is a verified local build,not
included as a large Git binary; a fresh machine must prepare pinned native
engine/assets/provider dependencies first.

The [Thai user guide](docs/PHASE335_USER_GUIDE_TH.md) covers every step from
opening Windows Terminal,search/selection,controls,reopen and fresh boot.
[Acceptance report](docs/PHASE335_ACCEPTANCE.md) records exact commands,versions,
codecs,build hashes,network/serial logs,paired screen/audio and limitations.

## Build and verify

Linux/WSL host tools: GCC,binutils,NASM,Python3,QEMU. The kernel is freestanding.
Native Cobalt/media dependencies are pinned separately in the Phase33 documents.
Development commands are logged for review in `build/activity.log`.

```bash
cd /home/xytron404/myos
python3 tools/run_logged.py -- bash build.sh
python3 tools/run_logged.py -- python3 tests/test_system.py
```

Full regression passed2026-10-12 at15:36:04 and restored the five normal
kernel/BIOS/UEFI/runtime artifacts byte-for-byte. Affected native media,memory,
BIOS/UEFI absolute pointer,edge/button recovery and underlay tests passed.
Normal kernel-only shell tests can use small validated profiles; native online
playback was tested at1024 MiB. Do not interpret this as minimum RAM.

## Status and limits

| Scope | Status |
|---|---|
| Phases0–23 | Existing QEMU boot,desktop,apps and release baselines retained |
| Phases26–31 | App runtime,network,verified HTTPS,text search,SB16 audio/local video baselines |
| Phase32 | Native YouTube search→selection→online video/sound,owner-confirmed |
| Phase33.1–33.5 | Native engine/runtime,standard media,YouTube and Rickroll QEMU acceptance complete |
| Phase24/Phase25 | Complete for selected ThinkBook offline hardware profile |

Loading may take minutes; UI/input can lag,frames drop and rebuffering remains.
Final Auto source640x360 was decoded to160x90 and scaled into a766x431 plane.
This does not promise Chrome parity,sustained30fps,all sites,accounts/DRM/Shorts,
or physical GPU/USB/network/audio driver support. Linked RAMFS cache is retained;
exact physical/SHM/alias recovery and stable non-file post-close heap are observed,
without claiming unlimited endurance or a measured exact warm-cache plateau.

One original1FPS PNG/WAV/timeline bundle and a5.18 MB approximate AVI review
copy remain local in ignored build. Raw network captures/signed URLs are not
published. Native clocks,not the AVI mux,measure A/V synchronization.
Ubuntu cleanup reclaimed8605818880 B; task-created Windows dependency cleanup
reclaimed71487576 logical B. Windows VHDX compaction/refund is separate maintenance.
Never delete ext4.vhdx. Development dependency size is not the boot image size.

[Roadmap](docs/ROADMAP.md),[current project state](docs/PROJECT_STATE.md) and
[handoff](docs/HANDOFF.md) describe the accepted profile and next scoped work.
No subsequent phase starts automatically from this acceptance.

## Physical hardware acceptance

Phase 24 and 25 passed on Lenovo ThinkBook 15 G3 ACL (21A4), Ryzen 5500U,
owner-reported 8 GB RAM, UEFI with Secure Boot disabled. Accepted scope:
800x600 desktop, PS/2 keyboard, USB HID mouse, desktop apps and USB BOT
persistence across power-cycle. Desktop apps run in kernel mode.
Physical network/audio and I2C touchpad are outside this offline profile.
YouTube acceptance is for QEMU; it does not imply online playback on this laptop.
The validated USB image is 64 MiB, not a minimum RAM specification.

See [Phase 24 completion](docs/PHASE24_COMPLETION.md),
[Phase 25 completion](docs/PHASE25_COMPLETION.md), and the
[hardware guide](docs/PHASE24_HANDOFF_TH.md).
Writing a raw image erases the target: follow the guide and verify the device.

## Source and publication

Clone https://github.com/ms584/MyOS.git and run the build commands from the
checkout root, replacing the original development machine path as needed.
Native Cobalt/assets require separate preparation; a kernel build alone does
not provide them. See the online user guide above. A clean-machine browser
build was not revalidated for this documentation update.

Generated images and local captures are excluded from Git. Existing tracked
acceptance evidence and test certificates are preserved; ignore rules do not
remove tracked files or erase history. Review captures for credentials and
signed media URLs before publishing new evidence.

Original MyOS material uses the [MyOS Non-Commercial Source-Available License](LICENSE).
Personal/educational non-commercial use and free sharing with attribution are allowed.
Sale/commercial exploitation requires permission; claiming original MyOS code as
your own is prohibited. Third-party licenses remain unchanged. See
[licensing scope](docs/PUBLICATION_LICENSE.md) and [publication audit](docs/PUBLICATION_AUDIT.md).