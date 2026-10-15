# MyOS roadmap — Desktop, applications and online media

Updated2026-10-12: Phase32/33.1–33.5 complete on the declared QEMU profile.
Owner accepts A/V sync<=500ms; physical Phase24/25 work remains pending.


Latest checkpoint3 (2026-10-10 16:45+07): Phase33.5/33 remain NOT ACCEPTED.
A private abort-diagnostic kernel/MRT passed four native diagnostic fixtures
and four online opens without reproducing the NORMAL di3oxi_6 abort. This
is no repair or normal playback pass. File table reached64/64; fixed-window
motion audit failed in three of four diagnostic opens. Root normal5 exact;
no QEMU remains. Owner has not tried; audible/interactive acceptance absent.
Next:use bounded live inode metadata + abort caller diagnostic to investigate
repeat/cache saturation; fix proven cause, retest normal/fresh boot, then owner
listening/controls. Thai guide is a draft; finalize only after real completion.
See wip/phase335/checkpoint3 and PHASE335_ACCEPTANCE.md.

Latest checkpoint2:normal4opens di3oxi_6 thirdnativeVP9 playback aborted
(FATAL/USER SIGNAL6). Resources reclaimed but repeated playback stabilityFAIL.
No rootcause/repair yet;diagnostics active. Owner has not tried audio/controls.
See PHASE335_ACCEPTANCE.md and Thai draft PHASE335_USER_GUIDE_TH.md.
Do not close33.5/33 or treat prior2opens as proof of this repeated-open gate.

### Phase33.5 current audit — 2026-10-10
See PHASE335_ACCEPTANCE.md and PHASE335_EXPERIMENT_GUIDE.md. Actual native
Rickroll trials found baseline sync failures108771/160083us and first-fix outage
failure172217us; all retained. Final native VP9 rebuffer ordering fix has focused/
OOM/fault passes and live max89970us; fresh g_wbd0r3 twoopens:45s motioneach,331.380sPCM,max89974us,exactresources.
Normal BIOS/UEFI rebuilt0/all5 hashes exact/defaultVP8 exact. No QEMU remains.
Screen/PCM/control/resource evidence does not replace listening or owner acceptance.
Audio input is unsupported in this chat. Cache retention is disclosed; warm plateau
not proved. Offline Retry later played a sponsored different video, not Rickroll.
Phase33.5/parent33 remain NOT ACCEPTED. Phase32 search/select/play is separately
rechecked; audible/interactive acceptance remains open. Local commit only/no push.

Revised 2026-09-28 at the owner's request. The product goal is a graphical
MyOS desktop with Settings, Calculator, Snake and eventually a browser that
can search the web and play YouTube. The revised roadmap has **34 phases,
numbered 0–33**. Phases 22–23 are complete at the native UEFI/release-tooling QEMU baseline.
Phase 24 has a validated virtual USB image but hardware preparation is pending;
Phase 25 is deferred at the owner's request (physical hardware not yet accepted device).

Phases 0–13 retain their existing numbering and history. Earlier commits named
2.1–2.3 cover parts of phases 3–5. Future phase completion must be supported by
the acceptance criteria below, not just a screenshot or an updated phase name.
This is a sequence of milestones, not a promise of equal effort per phase or
a delivery date for modern web compatibility.

## Approved execution order

The owner approved Phases 26–32 before the deferred hardware work on
2026-09-25 and added the strict Rickroll gate on 2026-09-28:

**26 → 27 → 28 → 29 → 30 → 31 → 32 → 33 → remaining Phase 24 → Phase 25**

Phases 26–33 are complete at their documented QEMU baselines; the owner confirmed Phase32/33 acceptance before this hardware preparation. Develop and validate the application, network and media
platform in QEMU before returning to physical hardware. Keep existing phase
numbers and commit history; the milestone sections below group scope, not
execution order. The completed Phase 24 USB image remains available.

| Order | Phase | Work |
|---|---|---|
| Done | 26 | **Complete QEMU baseline:** app loading, capacity and browser-component feasibility |
| Done | 27 | **Complete QEMU baseline:** networking and supported virtual NIC |
| Done | 28 | **Complete QEMU baseline:** native verified HTTPS and certificate rejection tests |
| Done | 29 | **Complete QEMU text-browser baseline:** verified HTTPS search, results, links and Back |
| Done | 30 | QEMU SB16 buffered PCM audio, file/tone playback, volume/mute, IRQ timing and underrun recovery |
| Done | 31 | MYV1 RLE video with synchronized PCM in QEMU |
| Done | 32 | Accepted native MyOS/QEMU online playback profile; see PHASE335_ACCEPTANCE.md and the accepted 2026-10-12 evidence |
| Done | 33 | Owner-accepted Rickroll video, audible sound, controls and A/V limit on the documented QEMU profile; no physical-hardware claim |
| 9 | 24 (remaining) | Select the physical target and implement required drivers |
| 10 | 25 | Physical-machine boot, display, input, apps and persistence acceptance |

Keep network, audio, input and storage drivers separate from higher-level
services so later hardware adaptation can replace device-specific code.
QEMU success does not establish physical-machine support. Phase32/33 accepted
records are preserved; their complete online audit is not repeated in Phase24.
Any newly found target limitation remains explicit in Phase24/25 evidence.

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
Phases 21–33 cover distribution and later milestones; Phases 21–23 are complete in QEMU.

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
| 24 | USB image and hardware preparation | **Partial (2026-10-15):** owner selected ThinkBook 15 G3 ACL Type 21A4. Firmware/controller/USB inventory collected. Bounded root-port xHCI/HID and SCSI/BOT512 prepared data partition implemented; focused QEMU input/persistence checks pass, full default architectural/firmware/layout/reproducibility checks pass. Actual AMD/mouse/Samsung/GOP handoff remains unconfirmed. No physical write or hardware pass. See PHASE24.md and PHASE24_HANDOFF_TH.md. |
| 25 | Real-hardware acceptance | **Deferred by owner until after Phase 33 and remaining Phase 24; physical target not yet accepted.** On an explicitly selected machine, verify boot, display, input, the three apps and supported persistence; record hardware, limitations and evidence. QEMU success alone cannot complete this phase. |

Hardware-driver work is scoped to the selected machine, not every PC. If access
to hardware is unavailable, record Phase 25 as pending; independent later
QEMU development can continue without claiming this milestone is complete.

## Milestone C — Online browser and YouTube

| Phase | Scope | Acceptance criteria |
|---|---|---|
| 26 | Application runtime and capacity | **Complete QEMU runtime/feasibility baseline:** file-based multi-page Ring 3 C apps, private heap/BSS, v2 64 KiB files with v1 compatibility, 100 Hz ticks and a tested NetSurf `libwapcaplet` component. This validates one browser dependency, not a complete engine or YouTube. The full browser will need larger limits and later network/HTTPS/GUI/media work. See PHASE26.md. |
| 27 | Networking | **Complete QEMU baseline:** RTL8139 driver, bounded buffers, IPv4/ARP/DHCP/DNS/UDP/TCP and streaming HTTP response API; live QEMU tests cover failure, timeout and reconnect. Only one active request is supported; HTTPS and browser integration are later phases. See PHASE27.md. |
| 28 | HTTPS platform | **Complete QEMU baseline:** RTC-derived UTC, RDRAND entropy, a small trusted root store and native Mbed TLS 3.6.7 HTTPS fetch. QEMU tests accept a valid certificate and reject wrong-host, untrusted, expired, invalid-time and no-entropy cases. TLS 1.2 and two production roots are configured; Browser GUI integration follows in Phase 29. See PHASE28.md. |
| 29 | Browser and web search | **Complete QEMU text-browser baseline:** the selected in-tree bounded HTML-to-text/link renderer loads HTTP and verified HTTPS pages, fetches live DuckDuckGo Lite search results, opens a real HTTPS result and restores cached Back history. Local TLS and live QEMU tests validate behavior. This is a kernel-mode non-JavaScript profile; CSS/DOM/media and broader browser compatibility remain outside this baseline. See PHASE29.md. |
| 30 | Audio | **Complete QEMU baseline:** SB16 ISA DMA/IRQ5 output at 11025 Hz mono 8-bit, buffered PCM queue, WAV file and tone playback, volume/mute/stop, IRQ-paced refill and silence/recovery on underrun. QEMU WAV capture tests verify actual output, controls and imported files. No physical-device driver or media decoding yet. See PHASE30.md. |
| 31 | Video and media | **Complete QEMU baseline:** native MYV1 palette/RLE video decode at 80x60, 10 fps, framebuffer scaling and PIT-synchronized 11025 Hz PCM via SB16. Pause/resume and stop are tested; malformed clips are rejected. The format is custom and unencumbered; MP4/WebM/YouTube remain unsupported. See PHASE31.md. |
| 32 | YouTube acceptance | **Complete on declared MyOS/QEMU profile,2026-10-12:** native homepage search,official result selection,online movie/sound and controls; owner separately confirms search→selection→sound. See PHASE32.md. |
| 33 | Rickroll end-to-end acceptance | **Complete on declared MyOS/QEMU profile,2026-10-12:** actual native online Rickroll>=30s,pause/resume,seek both ways,close/reopen,fresh boot,resources and regression. Owner sync criterion<=500ms; measured204.618/140.037/87.339ms. Loading/UI delay,drops/rebuffer remain. See PHASE335_ACCEPTANCE.md. |

Phase 29's initial browser is not evidence that YouTube works. Phases 32–33 depend
on the browser runtime, JavaScript/DOM, required media interfaces and codecs,
networking, TLS, audio and video all working together. A local video player,
link handed off to the host browser, or a picture of YouTube does not satisfy
this goal. Site compatibility must be rechecked at implementation time.

The accepted native engine is pinned Cobalt27.lts.3/Starboard18. Broader site
compatibility and physical drivers remain separate work; do not replace MyOS
with a host/Linux desktop or infer broader support from this accepted profile.

## Mapping from the original roadmap

| Original planned phase | Revised phase |
|---|---|
| 14 — BIOS distribution | 21 |
| 15 — UEFI | 22 |
| 16 — Build system | 23 |
| 17 — USB image | 24 |
| 18 — Real hardware | 25 |

The original remaining work is retained. Phases 14–20 now introduce the GUI
and apps, and phases 26–33 add the online platform. There are **2 phases not yet fully complete (24 and25)**: Phase24 hardware
preparation is partial and Phase25 physical acceptance remains deferred/pending.
Phases32/33 are accepted on their documented QEMU profile.
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
- Record development commands through `tools/run_logged.py`. The owner normally
  pushes manually; this Phase33 closure push is explicitly authorized.

## Historical engine/loader planning

Earlier incomplete-engine notes are archived in
`wip/phase335/checkpoint22/pointer-boundary/pre-closure-documents.json.gz`.
Current engine/media/Rickroll acceptance is in PHASE33.md and PHASE335_ACCEPTANCE.md.
