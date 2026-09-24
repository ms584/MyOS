# Phase 14 — Graphics foundation

Phase 14 adds a validated framebuffer mode, an off-screen 32-bit RGB surface,
clipped pixel/rectangle/BIOS-font drawing and a redraw/present operation. It
is the drawing layer for later mouse, windows and applications. The Shell and
serial console still work. A VGA text boot reports that graphics needs a
framebuffer and stays in the text Shell.

## Try the graphics demo

Build and boot the normal QEMU image:

```bash
cd ~/myos
bash build.sh
qemu-system-x86_64 -m 64M \
  -drive format=raw,file=build/myos-current.img,if=floppy -boot order=a
```

At `myos>` type `gfx`. The screen shows a graphical test scene drawn in a
full-size off-screen surface and copied to the framebuffer. Press **R** to
change the header color and redraw. Press any other key to return to the Shell.
The surface is freed on exit and the text console is cleared for the prompt.

This phase provides graphics primitives, not a desktop, windows, Settings,
Calculator or Snake. Those begin in Phases 15–20. Applications currently do
not run in the graphics demo.

## API and limits

`include/graphics.h` defines `gfx_surface_create/destroy`, `gfx_clear`,
`gfx_pixel`, `gfx_rect`, `gfx_text` and `gfx_present`. Surfaces own their pixel
memory; all drawing is clipped to the surface. Present clips the source to
the screen. RGB values use `0x00RRGGBB` and the BIOS 8×16 font is reused.

The driver checks VBE metadata before use: 32 bits per pixel, plausible width
and height, adequate aligned pitch, framebuffer range below the bootstrap
4 GiB identity-map limit, and console character-grid capacity. Invalid mode
metadata is rejected; the console uses its VGA/serial diagnostic path.
The graphics demo and full-screen surface require framebuffer mode.

The kernel heap is now 4 MiB to hold a full 800×600×4 off-screen surface
(1,920,000 pixel bytes) plus the filesystem and process allocations. The
memory and filesystem exhaustion tests use the new limit. A 16 MiB QEMU boot
is part of the regression suite. Kernel loading remains within the Stage 2
256 KiB binary limit; no large image assets are embedded in the kernel.

## Verification

```bash
cd ~/myos
python3 tools/run_logged.py -- python3 tests/test_system.py --suite graphics
python3 tools/run_logged.py -- python3 tests/test_system.py
```

The graphics suite sends actual PS/2 events in QEMU, captures framebuffer
pixels, verifies clipping and BIOS font text, presses R to verify redraw,
reopens the demo to check that its first surface was released, and confirms
the VGA text fallback. The full suite additionally covers memory, processes,
syscalls, filesystem, storage, keyboard and CPU faults and restores the normal
framebuffer build. All acceptance evidence is from QEMU, not physical hardware.

`docs/phase14-ready.png` shows the normal framebuffer graphics demo. The
owner pushes commits manually.
