# Phase 15 — PS/2 mouse and GUI input

Phase 15 adds a PS/2 mouse driver on IRQ12 and an interactive cursor to the
Phase 14 graphics scene. The mouse driver decodes standard three-byte packets,
tracks left/right/middle buttons, clamps coordinates to the current display,
and sends movement/button changes through a bounded event queue. The keyboard
and 100 Hz timer continue operating while the graphics demo is open.

## Try it in QEMU

```bash
cd ~/myos
bash build.sh
qemu-system-x86_64 -m 64M \
  -drive format=raw,file=build/myos-current.img,if=floppy -boot order=a
```

At `myos>`, type `mouse` to see the current position, pressed-button bitmask
and count of discarded old events. Type `gfx`, move the pointer, and click:
the drawn arrow follows the PS/2 mouse and turns amber while the left button
is down. The text below the shapes shows button state. Press **R** to redraw;
the pointer stays in place. Press **Enter** to return to the Shell and use
`mouse` again. The mouse stays active in VGA text mode, although `gfx` requires
a framebuffer. In QEMU, use Ctrl+Alt+G to release captured mouse input.

The button bitmask is 1=left, 2=right, 4=middle. Mouse coordinates are pixels
in framebuffer mode (normally 800×600). VGA fallback uses a 640×400 logical
range because it has no pixel framebuffer. Event consumers can call
`mouse_get`, `mouse_position` and `mouse_flush` from `include/mouse.h`.

## Driver behavior and limits

The controller enables the auxiliary PS/2 port, starts the mouse with device
ACK checks, and unmasks both PIC IRQ12 and its IRQ2 cascade. The IRQ handler
rejects bad packet starts and overflow packets, converts signed relative
movement into bounded screen coordinates, and records button changes. It does
not allocate memory or render from interrupt context. The queue has 64 slots
and can hold 63 events; when full, it discards the oldest event and increments
`dropped`. Consumers dequeue with interrupts masked for a short critical
section. The cursor restores only its previous 12×16 footprint from the
off-screen scene; button text redraws only its own small region.

This is a kernel-mode input and graphics demo. There are no desktop windows,
widgets, applications or user-mode GUI processes yet. PS/2 and PIC behavior
has been validated in QEMU, not on physical hardware or USB mice.

## Automated verification

```bash
cd ~/myos
python3 tools/run_logged.py -- python3 tests/test_system.py --suite mouse
python3 tools/run_logged.py -- python3 tests/test_system.py
```

The focused suite injects actual QEMU relative-motion and button events,
compares framebuffer pixels before/after movement and clicks, tests keyboard
redraw and return to the Shell, confirms timer progress, overfills the event
queue, checks coordinate clamping at all screen edges and boots VGA fallback.
The full suite also covers previous boot, memory, storage, process, syscall,
fault and keyboard behavior, then restores the normal framebuffer image.

`docs/phase15-mouse.png` captures the scene with a pressed mouse button.
The owner pushes commits manually.
