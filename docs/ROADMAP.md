# MyOS roadmap — Desktop, applications and online media

Revised 2026-09-24 at the owner's request. The product goal is a graphical
MyOS desktop with Settings, Calculator, Snake and eventually a browser that
can search the web and play YouTube. The revised roadmap has **33 phases,
numbered 0–32**. Phase 18 is the latest completed baseline; Phase 19 is next.

Phases 0–13 retain their existing numbering and history. Earlier commits named
2.1–2.3 cover parts of phases 3–5. Future phase completion must be supported by
the acceptance criteria below, not just a screenshot or an updated phase name.
This is a sequence of milestones, not a promise of equal effort per phase or
a delivery date for modern web compatibility.

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
limits remain documented in README and PHASE9–PHASE18. Settings and
Calculator work; Snake and the browser are pending.

## Milestone A — MyOS Desktop 1.0

Phases 14–18 below are complete at the QEMU desktop/Settings/Calculator baseline.
Phases 19–32 are **planned**.

| Phase | Scope | Acceptance criteria |
|---|---|---|
| 14 | Graphics foundation | **Complete QEMU baseline:** clipped pixels/rectangles/font text, full-size off-screen surface and redraw, validated VBE mode, 4 MiB heap, text fallback and screenshot acceptance. See PHASE14.md. |
| 15 | Mouse and GUI input | **Complete QEMU baseline:** PS/2 IRQ12 movement/buttons, bounded cursor and event queue, keyboard/mouse with timer preemption, real QEMU input and VGA fallback. See PHASE15.md. |
| 16 | Desktop and windows | **Complete QEMU baseline:** launcher with three placeholder windows, dragging, focus/stacking, close/redraw, resource cleanup and adjacent-window pixel checks. See PHASE16.md. |
| 17 | Widgets and Settings | **Complete QEMU baseline:** reusable buttons/labels/field, Settings theme/background/RGB accent, strict configuration validation, Phase 13 disk save/reboot restore and RAM-only notice. See PHASE17.md. |
| 18 | Calculator | **Complete QEMU baseline:** keyboard/mouse numeric entry, fixed-point decimal arithmetic, clear/backspace and four operators; tested division by zero, invalid input, overflow, 16 MiB RAM and VGA fallback. See PHASE18.md. |
| 19 | Snake | GUI game, keyboard direction, timer-based movement, score, collision, pause and restart. Persist high score when storage is available; other windows remain responsive. |
| 20 | Desktop integration and release | Launch Settings, Calculator and Snake from the desktop; use all three in one session, restore settings/high score after reboot, recover from app errors and retain a usable shell. Package a repeatable QEMU demo and user guide. |

**The first project delivery is Phase 20:** a usable graphical desktop with
three working applications. Browser/YouTube support is a later milestone.
The initial GUI may use kernel-resident built-in apps; it must be described
accurately and must not claim isolation between those apps. Prefer explicit
event handlers and separate app modules so Phase 26 can move apps into user
processes. GUI work targets the existing QEMU framebuffer first; the VGA text
fallback remains a diagnostic interface, not a graphical desktop.

## Milestone B — Distribution and selected real hardware

| Phase | Scope | Acceptance criteria |
|---|---|---|
| 21 | BIOS distribution | Boot a documented distributable disk image with the desktop and assets; replace floppy/loader constraints where necessary and verify the actual distributed image. |
| 22 | UEFI | UEFI loader, framebuffer and memory-map handoff; the same kernel reaches the desktop under UEFI virtual firmware. Secure Boot support is a separate decision. |
| 23 | Build and release tooling | One documented build/test/package workflow, dependency/version records, artifact checksums and repeatable releases. Preserve user data images across builds. |
| 24 | USB image and hardware preparation | Create and validate a USB-bootable image; identify the target machine's input/storage needs and implement the required drivers, including USB HID if needed. Any physical-device write needs explicit target selection. |
| 25 | Real-hardware acceptance | On an explicitly selected machine, verify boot, display, input, the three apps and supported persistence; record hardware, limitations and evidence. QEMU success alone cannot complete this phase. |

Hardware-driver work is scoped to the selected machine, not every PC. If access
to hardware is unavailable, record Phase 25 as pending; independent later
QEMU development can continue without claiming this milestone is complete.

## Milestone C — Online browser and YouTube

| Phase | Scope | Acceptance criteria |
|---|---|---|
| 26 | Application runtime and capacity | Load executable apps from storage; expand address spaces, heap/filesystem/file sizes and APIs required by the chosen browser; add ABI/library support, time services and safe FPU/SIMD context handling where required. Prove feasibility with a small port before selecting the browser engine. |
| 27 | Networking | A supported NIC driver, bounded packet buffers and IPv4/ARP/DHCP/DNS/TCP/UDP functionality sufficient for the target browser; tests include failure, timeout and reconnect behavior. |
| 28 | HTTPS platform | Reliable time, entropy, certificate store and TLS through a maintained library; fetch HTTPS resources and reject invalid certificates. Do not disable certificate verification to pass a demo. |
| 29 | Browser and web search | GUI address/search bar, navigation and rendering of documented target sites; choose or port an engine based on Phase 26 feasibility. Open actual search results through a search provider; record HTML/CSS/JavaScript support and unsupported sites. |
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
and apps, and phases 26–32 add the online platform. There are **15 planned phases remaining (18–32)**, including **3 to
Desktop 1.0 (18–20)**.

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
