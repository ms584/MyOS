# Phase 16 — Desktop and window management

Phase 16 adds a **kernel-resident** desktop demo with a launcher and three
placeholder windows. It establishes window focus, stacking, title-bar dragging,
close buttons and redraw. These placeholders prove the window system; they are
not Settings, Calculator or Snake. Those are planned for Phases 17–19.

## Try it in QEMU

```bash
cd ~/myos
bash build.sh
qemu-system-x86_64 -m 64M \
  -drive format=raw,file=build/myos-current.img,if=floppy -boot order=a
```

At `myos>`, type `desktop` and press Enter. Click a launcher tile labeled
**Demo A**, **Demo B** or **Demo C** to open a window. Clicking a window brings
it forward; drag its title bar to move it, or click its red **X** to close it.
Keyboard **1–3** also open/focus the corresponding window. Press **Q** or
**Esc** to return to the Shell. In QEMU, Ctrl+Alt+G releases captured mouse
input. `gfx` still opens the Phase 14–15 graphics test scene.

The desktop currently needs a framebuffer of at least 640×480. The default
QEMU mode is 800×600×32. In VGA text mode, `desktop` explains that a
framebuffer is required and the Shell remains usable.

## Design and current limits

`gui/desktop.c` is a separate window manager module. It keeps one full-screen
off-screen surface, stores windows in three bounded slots, and maintains a
bottom-to-top order list. Each opened window allocates a small metadata
structure; closing it frees the structure immediately. Exiting the desktop
frees all remaining windows and the surface. The cursor restores only its
previous small region when no other content changed. Opening, closing,
focusing and dragging redraw the composed scene, so overlapping windows
recover the correct underlying pixels. Coordinates are clamped to the screen
and leave room for the top and bottom bars.

The Shell, process scheduler and 100 Hz timer still run. Desktop code and its
windows run in kernel mode, without app process isolation. The launcher opens
only three placeholder windows. There are no widgets, Settings, Calculator,
Snake, saved window layouts or general application loader yet. QEMU is the
tested platform; this phase does not establish real-hardware or USB support.

## Automated verification

```bash
cd ~/myos
python3 tools/run_logged.py -- python3 tests/test_system.py --suite desktop
python3 tools/run_logged.py -- python3 tests/test_system.py
```

The focused suite drives actual QEMU PS/2 mouse and keyboard input. It opens
windows from the launcher, compares overlapping framebuffer pixels before
and after focus/close, drags a window and checks its old/new positions,
opens/closes windows repeatedly, verifies timer progress, re-enters the
desktop to check that its surface was freed, tests 16 MiB RAM and checks the
VGA Shell fallback. The full suite also regresses earlier phases and restores
the normal framebuffer build. `docs/phase16-desktop.png` shows the two-window
acceptance scene. The owner pushes commits manually.
