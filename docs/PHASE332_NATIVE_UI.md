# Phase 33.2 native window/presentation/input substrate

2026-10-01, continuation from local 2f4425f. Phase 33.1/33.2 remain OPEN.
Cobalt has NOT executed. No EGL/GLES backend or Starboard event loop is supplied
by this slice; no Phase 33.3 work and no push.

## Boundary and ownership

`user/platform-window.c` supplies the six pinned SbWindow imports. The actual
upstream window.h is hash-checked and compiled into the Ring 3 fixture:
SbWindowSize is 12 bytes; SbWindowOptions is 24, with name at offset 16.
The runtime module uses SSE2 so the float diagonal return uses the x64 ABI;
kernel code still uses general registers only. The module has no writable
global state in the RX runtime. Its shared lock/notification live at bytes
1680..1695 of the main process runtime TLS page, not a worker's errno/TLS.

The single fullscreen window uses the current boot framebuffer dimensions.
The tested mode is 800x600. Nonmatching dimensions or windowed requests fail;
the requested video_pixel_ratio is ignored as the header requires, and actual
ratio is 1.0. A physical diagonal of zero means unknown. Invalid GetSize leaves
the output unchanged. No 16:9 mode switching exists: the fixed 4:3 boot mode
is a documented limitation, not proof of the header's preferred default mode.
Names are currently metadata only; no window-manager title is drawn.

`kernel/native-ui.c` owns one process/generation capability, a saved framebuffer
surface, a bounded 128-event queue and a process-private notification address.
Tokens are not kernel pointers or dereferenced user pointers. Stale/high-bit/
other-process capabilities fail, and generation exhaustion rejects creation.
Only the owning process can present, consume events or destroy the window.
Allocation/capture failure does not install ownership. Exit/fault/forced kill
release ownership before process page reclamation, wake borrowed waiters,
restore the snapshot, free it, and drain legacy app input. Release is idempotent
for process cleanup; a stale explicit destroy returns false.

## Safe private presentation and raw input ABI

Private syscalls 42..46 query/acquire/present/release/dequeue. This ABI is NOT
SbInputData and is not Linux. The public mode query never exposes another
owner's token or dropped-event count. User output is validated in full before
any copyout or event dequeue. Source overflow, noncanonical/supervisor/unmapped
addresses, invalid dimensions/strides/format/destination are rejected.

Presentation copies RGBA8 or XRGB8888 into the real framebuffer, never maps
VRAM into Ring 3. Alpha is discarded, not composited; the renderer must produce
the final composited pixels. Every accessed source row is checked BEFORE the
first write, so a bad later row cannot partially change the screen. Padding
bytes are not accessed. A 64-pixel stripe uses 256 bytes of kernel scratch.
All calls are single-CPU/IRQ-serialized. Full frames hold IRQs off while copying;
native renderer latency/performance still needs measurement and optimization.
UI-owner writes go to serial, preserving the saved console cursor and pixels.

PS/2 IRQs deliver key down/up, set-1/E0 scan identity, translated ASCII keydown,
Shift/Control/Alt/Meta/Caps modifiers, pointer position/delta and button changes.
Existing shell/desktop queues remain usable. The raw queue drops the oldest
event on overflow and reports a saturated drop count; this is NOT a complete
Starboard focus/resynchronization policy. Pause/PrintScreen, wheel, Unicode/IME
and Starboard translation/dispatch remain open. Escape is still the explicit
`elf live` force-cancel control, not an ordinary engine Escape event.

MyOSUiPoll uses the existing sequence wait primitive, a single monotonic PIT
deadline (10 ms resolution), infinite/finite/nonblocking modes and close wakeup.
An old waiter receives EBADF after close/reuse. No timeout renewal on wakes;
zero-result polling leaves the event buffer unchanged. The platform handle is
the private software-winsys token; actual EGL use of it remains unimplemented.

## Evidence and reproduction

Run `python3 tools/run_logged.py -- env MYOS_TRACE=0 python3 tests/test_native_ui.py`
after a framebuffer build with the pinned headers cached under
build/engine-evaluation/contracts. For packaged UEFI, build with
`python3 tools/build_uefi.py`, then run the same test with `--uefi`.
Generated assets/logs/PPM captures are in
build/system-tests/native-ui and build/system-tests/native-ui*. No production
host presentation/input helper or guest-memory/result substitution is used.

- Native fixture passed at 04:00 +07, then again with the self-test build after
  the stripe correction at 04:06. It compiles against the pinned header, proves
  actual VRAM plus QEMU screen quadrant/XRGB samples, rejects bad later source
  rows without changing pixels, receives real injected PS/2 keyboard/mouse,
  tests finite/blocking waits, close/reuse, normal/fault/cancel/relaunch, and
  exact page/heap counts. Restoration checks sampled original screen pixels,
  not every byte of the screen or a complete compositor contract.
- Supplementary ASan/UBSan tests compile the real kernel ownership/queue code;
  they cover other-process isolation, queue overflow/order/timestamps/drop
  reporting, generation reuse, allocation/capture rollback, notifications,
  teardown, idempotent cleanup and modifier storage. Graphics/process callbacks
  are host mocks; this is not native framebuffer or Cobalt evidence.
- The first full attempt stopped at self-test link: BSS ended at 0x809c0,
  past the unchanged 0x80000 BIOS stack boundary. The stripe change saves 4864
  bytes. Self-test BSS now ends at 0x7f6c0 and kernel file is 256280 bytes;
  both original safety limits pass. `build/kernel.map` records the layout.
- Replacement full `tests/test_system.py` PASSED at 04:35 +07, including the
  native window fixture, lifecycle/storage/network/fault/low-memory checks and
  honest VGA ENOTSUP fallback. Normal framebuffer image restored: kernel
  248088 bytes, image 1474560 bytes, BSS end 0x7d6c0.
- Pinned Cobalt staging PASSED 04:37: 156 strong imports resolved, 86 missing,
  weak TLS zero-bound, relocation/RELRO/reclamation and 64 MiB OOM rollback.
  The engine was NOT executed; these results do not prove web acceptance.
- Final packaged UEFI DMA/stack/framebuffer/real PCM (435/655 Hz) and network
  tests PASSED 04:37. The same native UI/input/wait/fault/cancel/relaunch and
  page/heap/frame-sample restoration fixture PASSED on UEFI at 04:38.

## Next required contracts

Implement the real native renderer/winsys and pinned EGL/GLES semantics, then
translate focused raw events into SbInputData and dispatch through the actual
SbEventHandle lifecycle/launcher with scheduled callback cancellation. Complete
network resolver/options/signals and native Cobalt TLS as required. Run Cobalt
itself for HTML/JavaScript/DOM/input/verified HTTPS >=60s plus close/relaunch.
The 156 fixture-covered strong imports are subsets, not completion percentages
or evidence that the engine boots. Keep both parent phases open.
