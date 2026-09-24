# Phase 21 — BIOS HDD distribution image

Phase 21 packages the complete Desktop 1.0 baseline as a **16 MiB raw BIOS
hard-disk image**. The boot sector is at LBA 0, the 8 KiB second stage at LBA
1–16, and the kernel begins at LBA 17. Desktop code, the three built-in apps,
the RAMFS seed files and their other assets are embedded in the kernel; no
separate host files are needed at boot. The new loader uses BIOS EDD (`INT 13h`
extensions) and LBA reads rather than assuming floppy CHS geometry. It checks
for EDD support and reports `E` on a read failure. The original floppy image
remains available for development and regression tests.

## Build and boot the exact image

From the repository in WSL Ubuntu:

```bash
cd ~/myos
python3 tools/run_logged.py -- python3 tools/package_bios.py
qemu-system-x86_64 -m 64M \
  -drive format=raw,file=build/myos-bios.img,if=ide,index=0 \
  -boot order=c
```

At `myos>`, enter `desktop`. Click Settings, Calculator or Snake in the top
bar; keys 4–6 also open them. Press Q or Esc to return to the Shell. The image
can be copied as `build/myos-bios.img` to another machine running QEMU with
legacy BIOS firmware. The package command rebuilds the normal kernel and
replaces **only this generated OS image**; it does not create or overwrite a
user data image. A QEMU launch uses the packaged image directly, not the
development floppy. `tail -f build/activity.log` in another WSL terminal
shows the build and test commands.

For persistent settings, high scores and Shell files, create a *separate*
MyOS data disk once and attach it as the primary IDE slave:

```bash
python3 tools/create_data_disk.py build/myos-data.img
qemu-system-x86_64 -m 64M \
  -drive format=raw,file=build/myos-bios.img,if=ide,index=0 \
  -drive format=raw,file=build/myos-data.img,if=ide,index=1 \
  -boot order=c
```

The data-disk creator refuses to overwrite an existing image. Reuse the same
`build/myos-data.img` on subsequent boots. The kernel looks for the marked
data disk on the primary IDE master or slave, so the boot disk and data disk
stay separate. Without a marked disk, MyOS still runs with RAM-only changes.
An unknown IDE disk is never used as a MyOS save target.

## Verification

```bash
cd ~/myos
python3 tools/run_logged.py -- python3 tests/test_bios_package.py
python3 tools/run_logged.py -- python3 tests/test_system.py
```

The package test boots the **actual 16 MiB image** in QEMU, captures the
desktop framebuffer, opens all three apps, checks Shell access, verifies the
OS image stays byte-for-byte unchanged, then boots with a separate data disk
and checks Settings/high-score restoration after reboot. The kernel read
crosses the old floppy CHS track boundary. The full suite protects earlier
floppy, memory, GUI, storage and fallback behavior; it restores the normal
development image afterward. Logs and screenshots are in `build/system-tests/`.

This is an unpartitioned, raw **BIOS HDD image**, not an MBR/GPT filesystem,
USB installer or UEFI image. It has been accepted in QEMU with SeaBIOS only;
real hardware is Phase 25. The kernel is still loaded below 1 MiB and limited
to 256 KiB; the 16 MiB image size does not increase kernel or RAMFS capacity.
The apps remain kernel-resident and are not isolated processes. The separate
data disk uses MyOS's custom snapshots, not a general filesystem. The web,
networking, audio and video milestones remain future work.
