# MyOS roadmap — Desktop, applications and online media

Revised 2026-09-25 at the owner's request. The product goal is a graphical
MyOS desktop with Settings, Calculator, Snake and eventually a browser that
can search the web and play YouTube. The revised roadmap has **33 phases,
numbered 0–32**. Phases 22–23 are complete at the native UEFI/release-tooling QEMU baseline.
Phase 24 has a validated virtual USB image but hardware preparation is pending;
Phase 25 is deferred at the owner's request (no USB device).

Phases 0–13 retain their existing numbering and history. Earlier commits named
2.1–2.3 cover parts of phases 3–5. Future phase completion must be supported by
the acceptance criteria below, not just a screenshot or an updated phase name.
This is a sequence of milestones, not a promise of equal effort per phase or
a delivery date for modern web compatibility.

## Approved execution order

The owner approved this order on 2026-09-25:

**26 → 27 → 28 → 29 → 30 → 31 → 32 → remaining Phase 24 → Phase 25**

Phases 26–27 are complete at their documented QEMU baselines. Develop and validate the application, network and media
platform in QEMU before returning to physical hardware. Keep existing phase
numbers and commit history; the milestone sections below group scope, not
execution order. The completed Phase 24 USB image remains available.

| Order | Phase | Work |
|---|---|---|
| Done | 26 | **Complete QEMU baseline:** app loading, capacity and browser-component feasibility |
| Done | 27 | **Complete QEMU baseline:** networking and supported virtual NIC |
| Next | 28 | HTTPS, certificate validation and supporting services |
| In progress | 29 | HTTP browser preview; HTTPS search and engine port pending |
| 5 | 30 | Audio playback and controls |
| 6 | 31 | Video decoding and audio/video synchronization |
| 7 | 32 | Actual online YouTube playback acceptance in QEMU |
| 8 | 24 (remaining) | Select the physical target and implement required drivers |
| 9 | 25 | Physical-machine boot, display, input, apps and persistence acceptance |

Keep network, audio, input and storage drivers separate from higher-level
services so later hardware adaptation can replace device-specific code.
QEMU success does not establish physical-machine support. Phase 32 still
requires the actual browser/media acceptance below; this order does not
promise feasibility or a completion date. If a feasibility gate fails,
document it and revise the plan with the owner.

## Completed foundations

| Phase | Scope | Accepted baseline |
|---|---|---|
| 0 | Development environment | WSL, GCC, NASM, binutils and QEMU |
| 1 | Bootloader | BIOS two-stage loader with retries |
| 2 | Bootloader → Kernel | Load build-sized kernel and enter C |
| 3 | CPU execution mode | Real → protected → 64-bit long mode |
| 4 | Interrupts | GDT/IDT, exceptions, PIC and timer/keyboard IRQs |
| 5 | Console | Framebuffer text, VGA fallback and scrolling |
| 6 | Keyboard | PS/2 input and US keyboard modifiers |
| 7 | Memory | E820 PMM, paging APIs and fixed-size kernel heap |
| 8 | Scheduler | Timer preemption, integer context preservation and task exit |
| 9 | User processes | Embedded Ring 3 demos, private address spaces and fault containment |
| 10 | System calls | Validated console/process calls and bounded user buffers |
| 11 | RAMFS | Directories, binary files, per-process file handles and file syscalls |
| 12 | Shell | File commands, bounded line editing and PS/2 acceptance tests |
| 13 | Storage | ATA PIO, marked data image, dual RAMFS snapshots and reboot recovery |

These are educational QEMU baselines. The kernel-resident window demo is
not a general application platform, embedded user demos are not a general
application loader, and the custom snapshot disk is not FAT32. Current
limits remain documented in README and PHASE9–PHASE20. Settings,
Calculator and Snake work; the browser is pending.

## Milestone A — MyOS Desktop 1.0

Phases 14–20 below are complete at the QEMU desktop and app baseline.
Phases 21–32 cover distribution and later milestones; Phases 21–23 are complete in QEMU.

| Phase | Scope | Acceptance criteria |
|---|---|---|
| 14 | Graphics foundation | **Complete QEMU baseline:** clipped pixels/rectangles/font text, full-size off-screen surface and redraw, validated VBE mode, 4 MiB heap, text fallback and screenshot acceptance. See PHASE14.md. |
| 15 | Mouse and GUI input | **Complete QEMU baseline:** PS/2 IRQ12 movement/buttons, bounded cursor and event queue, keyboard/mouse with timer preemption, real QEMU input and VGA fallback. See PHASE15.md. |
| 16 | Desktop and windows | **Complete QEMU baseline:** launcher with three placeholder windows, dragging, focus/stacking, close/redraw, resource cleanup and adjacent-window pixel checks. See PHASE16.md. |
| 17 | Widgets and Settings | **Complete QEMU baseline:** reusable buttons/labels/field, Settings theme/background/RGB accent, strict configuration validation, Phase 13 disk save/reboot restore and RAM-only notice. See PHASE17.md. |
| 18 | Calculator | **Complete QEMU baseline:** keyboard/mouse numeric entry, fixed-point decimal arithmetic, clear/backspace and four operators; tested division by zero, invalid input, overflow, 16 MiB RAM and VGA fallback. See PHASE18.md. |
| 19 | Snake | **Complete QEMU baseline:** arrow/WASD steering, PIT-driven movement, growth, score, wall/body collision, pause/restart; validated high-score save/reboot restore, invalid-file fallback, other-window responsiveness, 16 MiB RAM and VGA fallback. See PHASE19.md. |
| 20 | Desktop integration and release | **Complete QEMU baseline:** top-bar app launch, all three apps in one session, Settings/high-score restoration after reboot, error recovery, usable Shell, repeatable QEMU demo and guide. Validated at 16 MiB and with VGA diagnostic fallback. See PHASE20.md. |

**The first project delivery, Phase 20, is complete in QEMU:** a usable graphical desktop with
three working applications. Browser/YouTube support is a later milestone.
The initial GUI may use kernel-resident built-in apps; it must be described
accurately and must not claim isolation between those apps. Prefer explicit
event handlers and separate app modules so Phase 26 can move apps into user
processes. GUI work targets the existing QEMU framebuffer first; the VGA text
fallback remains a diagnostic interface, not a graphical desktop.

## Milestone B — Distribution and selected real hardware

| Phase | Scope | Acceptance criteria |
|---|---|---|
| 21 | BIOS distribution | **Complete QEMU baseline:** documented 16 MiB raw HDD image with embedded desktop/apps/assets, EDD LBA boot loader, separate persistent data disk and direct acceptance boot of the packaged image. See PHASE21.md. |
| 22 | UEFI | **Complete QEMU baseline:** native x86-64 loader, GOP, allocated boot data/page tables and memory-map handoff; the same kernel runs all three desktop apps and restores saved state under OVMF. Secure Boot is unsupported. See PHASE22.md. |
| 23 | Build and release tooling | **Complete:** one documented build/test/package workflow, dependency/version and source records, SHA-256 checksums, byte-identical rebuilds and preserved user save images. Packaging rejects stale validation evidence. See PHASE23.md. |
| 24 | USB image and hardware preparation | **Partial:** 64 MiB hybrid BIOS/UEFI image validated through QEMU USB mass storage. Target-machine selection and required input/storage drivers (including USB HID if needed) are scheduled after Phase 32. Any physical-device write needs explicit target selection. See PHASE24.md. |
| 25 | Real-hardware acceptance | **Deferred by owner until after Phase 32 and remaining Phase 24; no USB device yet.** On an explicitly selected machine, verify boot, display, input, the three apps and supported persistence; record hardware, limitations and evidence. QEMU success alone cannot complete this phase. |

Hardware-driver work is scoped to the selected machine, not every PC. If access
to hardware is unavailable, record Phase 25 as pending; independent later
QEMU development can continue without claiming this milestone is complete.

## Milestone C — Online browser and YouTube

| Phase | Scope | Acceptance criteria |
|---|---|---|
| 26 | Application runtime and capacity | **Complete QEMU runtime/feasibility baseline:** file-based multi-page Ring 3 C apps, private heap/BSS, v2 64 KiB files with v1 compatibility, 100 Hz ticks and a tested NetSurf `libwapcaplet` component. This validates one browser dependency, not a complete engine or YouTube. The full browser will need larger limits and later network/HTTPS/GUI/media work. See PHASE26.md. |
| 27 | Networking | **Complete QEMU baseline:** RTL8139 driver, bounded buffers, IPv4/ARP/DHCP/DNS/UDP/TCP and streaming HTTP response API; live QEMU tests cover failure, timeout and reconnect. Only one active request is supported; HTTPS and browser integration are later phases. See PHASE27.md. |
| 28 | HTTPS platform | Reliable time, entropy, certificate store and TLS through a maintained library; fetch HTTPS resources and reject invalid certificates. Do not disable certificate verification to pass a demo. |
| 29 | Browser and web search | **In progress:** a kernel-resident GUI preview loads bounded local HTTP pages, displays text/links, supports Back and explicit errors. Phase 28 TLS, a chosen/ported browser engine, and actual HTTPS search results remain required for completion. Current HTML/CSS/JavaScript limits and tests are in PHASE29.md. |
| 30 | Audio | Supported audio device, buffered PCM playback, volume/mute and synchronization timing; validate playback and underrun recovery. Add Settings controls only when they work. |
| 31 | Video and media | Decoding, buffered playback, rendering and audio/video synchronization for documented formats/resolutions. Account for memory/CPU needs and any codec licensing obligations. |
| 32 | YouTube acceptance | In the MyOS browser, open YouTube, search, select and play an actual online video with sound, pause/resume and seek at a documented resolution. Verify HTTPS and required scripting/media support; record test date, browser version and limitations. |

Phase 29's initial browser is not evidence that YouTube works. Phase 32 depends
on the browser runtime, JavaScript/DOM, required media interfaces and codecs,
networking, TLS, audio and video all working together. A local video player,
link handed off to the host browser, or a picture of YouTube does not satisfy
this goal. Site compatibility must be rechecked at implementation time.

The exact engine/port and feasibility of modern sites remain open technical
decisions. If a feasibility gate fails, document the missing OS capabilities
and revise the implementation plan with the owner; do not silently replace
MyOS with a Linux-based desktop or mark the phase complete. No completion
date for the web/media milestone is promised by this roadmap.

## Mapping from the original roadmap

| Original planned phase | Revised phase |
|---|---|
| 14 — BIOS distribution | 21 |
| 15 — UEFI | 22 |
| 16 — Build system | 23 |
| 17 — USB image | 24 |
| 18 — Real hardware | 25 |

The original remaining work is retained. Phases 14–20 now introduce the GUI
and apps, and phases 26–32 add the online platform. There are **8 phases not yet fully complete (24–25, 27–32)**: Phase 24 is partial, Phase 25 is deferred, and Phases 27–32 are planned.
Desktop 1.0 was delivered in Phase 20 as a QEMU baseline.

## Working and validation rules

- Preserve existing phase numbers and commit history through Phase 13.
- Use the actual revised phase in future implementation commits. A roadmap
  edit does not complete any implementation phase.
- Give each phase runnable acceptance checks and a short user guide. For
  architectural changes run `python3 tests/test_system.py` and restore the
  normal image after fault variants.
- Exercise GUI behavior with real QEMU keyboard/mouse events and inspect the
  rendered result; verify resource cleanup and persistence where relevant.
- Keep claimed capabilities tied to tests: QEMU, chosen hardware, built-in
  kernel apps and isolated user processes are different acceptance scopes.
- Record development commands through `tools/run_logged.py`. The owner always
  pushes to GitHub manually.
