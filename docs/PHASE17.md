# Phase 17 — Widgets and Settings

Phase 17 adds reusable button, label and text-field drawing and a working
**Settings** window in the kernel-resident desktop. It changes the dark/light
theme, blue/green desktop background and a six-digit RGB accent color. Saved
values live in a validated RAMFS configuration file. If a recognized Phase 13
ATA data disk is attached, the RAMFS snapshot preserves them across reboot.

## Try it

For settings that survive reboot, create a dedicated data disk once and reuse
it on later boots:

```bash
cd ~/myos
python3 tools/create_data_disk.py
bash build.sh
qemu-system-x86_64 -m 64M \
  -drive format=raw,file=build/myos-current.img,if=floppy \
  -drive format=raw,file=build/myos-data.img,if=ide,index=0 \
  -boot order=a
```

The disk-creation tool refuses to overwrite an existing image. At `myos>`,
type `desktop`, then click the **Settings** launcher tile or press **4**.
Choose Theme **Dark/Light** and Background **Blue/Green**. The desktop previews
those selections immediately. The Accent field accepts exactly six
hexadecimal digits (`RRGGBB`); click it, erase the old value with Backspace,
and type the new value. Click **Save** or press Enter while editing. An invalid
color is rejected without replacing the saved settings. The status line says
whether the result was saved to disk, remains in RAM, or failed. At the Shell,
`settings` prints the committed values and persistence mode.

Close QEMU and boot again with the **same** data image to check that values
return. Do not run the creation command again for that image. Without a data
disk, the UI and Save still work, but the status says RAM-only and changes
disappear on reboot. The VGA text fallback retains the `settings` Shell
command; the graphical desktop requires a framebuffer.

The Accent field has focus until you click elsewhere, press Enter to save,
or press Esc to cancel editing. **Q** exits the desktop when the field is not
being edited. Demo A/B/C are still Phase 16 placeholder windows.

## Data format and limits

The configuration is `/etc/settings.cfg`, a 16-byte versioned binary record
containing theme, background, RGB accent and a checksum. On boot, Settings
checks file type, exact length, version, allowed values and checksum. Missing
or invalid files use safe defaults. The file is part of the Phase 13 dual-bank
RAMFS snapshot; it is not written as a separate raw disk format. A failed
disk save reports an error and does not guarantee that the latest values will
survive reboot. This checksum detects accidental corruption, not deliberate
tampering.

`gui/widgets.c` provides reusable hit testing and button, label and field
drawing for later apps. `gui/settings.c` owns configuration parsing and UI
state. The desktop and Settings run in kernel mode; they are not isolated
user applications. Other preferences, accessibility controls, hardware
settings and actual Calculator/Snake apps are future work.

## Verification

```bash
cd ~/myos
python3 tools/run_logged.py -- python3 tests/test_system.py --suite settings
python3 tools/run_logged.py -- python3 tests/test_system.py
```

The QEMU suite uses actual PS/2 mouse and keyboard input to exercise the
widgets, rejects malformed color text, checks preview pixels, verifies RAM
only reset after reboot, saves to a disposable ATA data image, restores from
it after reboot, rejects a malformed settings file, checks the 16 MiB RAM
case and preserves VGA fallback. The full suite regresses earlier phases and
restores the normal framebuffer build. `docs/phase17-settings.png` shows the
tested saved-to-disk state. The owner pushes commits manually.
