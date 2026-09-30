# Phase 33.2 native event/lifecycle substrate

Continuation from local 2831cfe, 2026-10-01. Phase 33.1/33.2 remain OPEN.
This is an isolated Ring 3 runtime and native fixture, NOT Cobalt execution.

## ABI and ownership

`user/platform-event.c` provides SbRunStarboardMain, SbEventSchedule/Cancel,
RequestStop/Conceal/Reveal/Blur/Focus and SupportsResume (constant false).
`include/starboard-event-abi.h` describes the pinned Starboard 18 x64 wire ABI.
The fixture compiles the actual pinned event/input/key/window headers, checks
Event=24, StartData=24, InputData=88 and EventId=4, and verifies enum semantics.
Header hashes are in PHASE331_ABI_SOURCES.json, including input.h and key.h.

The loop is owned heap storage, referenced under a process-shared TLS lock
(main TLS bytes 1712..1743). UI state uses bytes 1680..1711. No mutable state is
placed in RX runtime pages. Caller-owned callback contexts are never freed by
the runtime. Event and payload objects are borrowed only during the handler.
StartData carries the caller's argc/argv and a null link; a real Cobalt loader
entry and startup URL/resource handling still need implementation.

The fixed pool holds 128 pending callbacks. Zero/negative delays are immediate;
positive microsecond delays use monotonic time, with INT64_MAX saturation.
Exhaustion, null callbacks, closed loops or exhausted uint32 IDs return invalid
zero. IDs increase and do not wrap/reuse. Equal deadlines select the earlier ID.
Callbacks execute only on the thread running SbRunStarboardMain. Cancel safely
removes a pending ID from any thread; main-thread cancel guarantees a callback
not already dispatched cannot run. Cancelling an already dispatched ID is a
no-op, not an attempt to interrupt a running callback.

Read the shared UI sequence before checking both queues, then SYS_WAIT on the
same sequence. IRQ input and worker requests broadcast wakeups. SYS_WAKE count
zero wakes NOBODY; UINT32_MAX is required. The infinite-wait stress fixture
found and fixed this bug, which finite deadlines had hidden. Immediate callback
chains alternate with input; incoming input cannot suppress overdue callbacks.

## Lifecycle and real visibility

Normal first event is Start. Pre-launch RequestConceal gives Preload (no data)
and hidden windows. Re-entering or running the loop again in the same process
returns EBUSY; relaunch uses a new process and TLS pages. Stop's first status
wins, dispatching Blur -> Conceal -> Freeze -> Stop as needed. Scheduled work
does not run after Stop. SupportsResume=false; no Freeze/Unfreeze resume API is
advertised. Applications must stop their background activity in Freeze.

Conceal releases tracked held input, blurs and hides the real native surface.
Reveal restores the surface to BLURRED, not automatically FOCUSED. Focus is a
separate request; blurred queued input is drained before focusing. One pending
lifecycle request is consumed per iteration, preserving concurrent Conceal and
Reveal. Repeated same-state requests are idempotent/coalesced; Stop has priority.

Kernel SYS_UI_VISIBLE=47 validates owner, generation and Boolean/reserved args.
Acquisition owns the original screen, a foreground surface and a 128-event heap
queue. Hide captures the real app frame and restores the original. Hidden
present updates foreground pixels only; Reveal presents them. Failed acquisition
allocation/capture frees partial objects; hide/reveal failure retains prior
visibility. Release/fault/kill wakes borrowers
before freeing snapshots/queue and invalidating the notification address.
Moving the 6144-byte queue from BSS to owned heap preserves the unchanged BIOS
stack/DMA layout bounds. Fullscreen surfaces cost about 3.84 MiB at 800x600.

Keyboard mapping uses physical SbKey identity, ASCII layout characters, left/
right modifier locations and pinned modifier masks; pointer floats report actual
PS/2 positions/deltas. Pressure/tilt unavailable values are NaN. The pinned X11
adapter (`starboard/shared/x11/application_x11.cc` at the same commit) confirms
Mouse1=left, Mouse2=middle, Mouse3=right; PS/2 bits instead order left/right/middle.
Input is dispatched only while focused. Overflow releases tracked held input
before processing retained events; it cannot reconstruct lost hardware history.

## Evidence and remaining gates

`tests/test_platform_events.py` PASS 05:29:13 +07 after the infinite-wake fix:
real main-thread callbacks, pool/ID bounds, positive/negative/overflow deadlines,
main/worker cancellation, fairness, infinite wait, concurrent teardown, actual
Shift-A/arrows/right modifiers and three mouse buttons, hidden/blurred exclusion,
VRAM/PPM hide/reveal/hidden presents, preload, concurrent lifecycle requests,
graceful stop, held-key release, forced fault/cancel/relaunch and exact page/heap
reclamation plus sampled framebuffer restoration. Native UI + ASan/UBSan PASS
05:29:23 includes allocation/capture/present failure and hidden pixel retention.
Pinned binary/44-header hashes and 243-row contract register PASS 05:29:52.
Full tests/test_system.py PASS 06:00:21, including native events/UI, all prior
runtime/lifecycle/TCP/storage, fault/RAM and VGA fallback; normal image restored
(kernel 253624, image 1474560 bytes). Actual Cobalt staging/RELRO/reclamation and
64 MiB OOM rollback PASS 06:04:49, engine NOT executed. UEFI package/DMA/real
frames/captured 435/655 Hz PCM and DHCP/DNS PASS 06:05; native event fixture PASS
06:06:05 and native UI + supplementary ASan/UBSan PASS 06:06:18 on UEFI.
Additional invalid main arguments/explicit Blur cases (unchanged production)
PASS on BIOS 06:10:29 and UEFI 06:11:03.

Self-test BSS ends at 0x7f460 and image file is 261816 bytes (328 bytes below
the unchanged cap). An isolated `bash tools/measure-runtime-pic.sh` probe at
05:54 measured 34182 -> 28944 runtime bytes, no remaining ELF relocations,
one RX LOAD and no mutable RX data/TLS overlap. This is code-generation evidence
only; production flags and image are unchanged. Native validation is required
before adopting it to make room for launcher/adapter code.

The actual library has e_entry=0; GetEvergreenSabiString at 0x35a5bd0 returns
read-only JSON for x64/SysV/API18. A real launcher must verify that boundary
and visible executable SbEventHandle before constructors, preserve argc/argv
through init/fini, and size/guard its main stack (generic ELF currently 16 KiB).

Four imported event functions now have supported-subset fixture coverage:
160/242 strong imports, 82 strong plus one weak TLS still unresolved. Counts
do not certify full contracts, EGL/GLES, audio/media plumbing or Cobalt itself.
Limits: one 800x600 fullscreen owner; no wheel/IME/non-ASCII text, hotplug,
compositor focus, resumable suspension, network-state events or native launcher.
Escape remains a force-cancel key. Native renderer/required TLS dependencies,
real SbEventHandle loader entry and remaining interfaces are still required.
HTML/JS/DOM, typing/click/scroll, concurrent verified HTTPS, interactive >=60s
and close/relaunch must be proved with Cobalt itself before closing 33.1/33.2.
No Phase 33.3; no push.
