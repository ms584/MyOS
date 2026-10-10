# Phase33.5 checkpoint6: reproduced native tooltip abort; NOT REPAIRED

The private diagnostic online bundle `j_4b33rx` completed at 2026-10-10
18:10:59.971911+07. Opens1/2 completed the scheduled controls and closed;
open3 returned to shell after137.174s with main-thread FATAL and SIGABRT6.
Normal `di3oxi_6` previously failed similarly; its exact callchain was not
captured, so matching it to this cause remains an inference.

Exact official buildID739726c6552d87a319de5106f918c6a8e9ab9341 symbols,
previously verified across all21 loaded sections, resolve the abort stack to
LogMessage destructor → PlatformWindowStarboard::Show → Ozone platform
window creation → DesktopNativeWidgetAura → TooltipAura::CreateTooltipWidget
→ TooltipStateManager::ShowNow → OneShotTimer. Use returnPC minus1, rather
than the next function's first byte. Only records AFTER the FATAL belong to
this abort; preceding resource-error PC records concern disk cache ENOSPC.

Exact tag27.lts.3 platform_window_starboard.cc lines156–165 assigns requested
bounds to SbWindowOptions, calls SbWindowCreate, then CHECKs its validity.
Current MyOS platform-window.c rejects requested dimensions different from
800×600, windowed mode, and a second acquired physical UI (EBUSY).
A private native Ring3 reproduction ran twice at18:19:03–18:19:07:
200×30 with primary -> ENOTSUP; second800×600 -> EBUSY;200×30 without primary
-> ENOTSUP; primary close/recreate and physical/SHM/alias cleanup exact.
This proves the substrate limitation, not the actual tooltip's requested
width/height. A numeric-only private window diagnostic now measures that
request in a live online follow-up; no production window changes yet.

The pinned Starboard contract explicitly permits fullscreen-only devices
with a single SbWindow; default options must work, unsatisfiable options
may fail. Upstream OzoneStarboard reports IsWindowCompositingSupported=false.
TooltipAura sets params.context and uses a separate DesktopNativeWidgetAura.
Returning a shared fake handle or lying about size would not implement the
required drawing/input/lifetime. A correct engine/platform adaptation must
retain the tooltip within the actual native compositor or implement real
popup surfaces and bounds; inspect placement/resizing and callback ownership
before selecting the repair. Do not infer resolution support from a handle.

Scope: diagnostic builds are not normal MyOS acceptance. No claim of listening,
seek stale-audio inspection, freshboot acceptance, stability or owner signoff.
The original normal images remain untouched. Build/full architectural suite
and normal repeated/freshboot online controls remain required after repair.
Screen/PCM originals are ignored in the source bundle; evidence.json.gz keeps
native frame/timing references and QEMU profile. Public serial excludes URLs,
headers/cookies and payloads. Source files retain upstream license notices.

Follow-up `1x2i1nyl` completed18:24:14.445708+07,180s single online open:
settings/quality/144p native clicks and a150s pointer hover. No tooltip creation
or FATAL: only the primary800×600 create was logged. This does not repair the
reproduced abort or prove its requested tooltip dimensions. A native HTTPS
HTML-title fixture is now running as a separate diagnostic. Initial attempts
failed in the host test setup (Unix socket path length; omitted NIC); these
are recorded, not Cobalt/YouTube failures or passes. Private keys remain ignored.

Native HTTPS HTML-title attempt (NIC corrected) ended18:29:25 with no DOM
ready within90s. It is an unsuccessful diagnostic, not a tooltip reproduction.
Ordinary content-asset HTML-title diagnostic follows; it cannot count as
online YouTube acceptance. `controls-observation.json` records image differences
and native PCM byte deltas at scheduled pause/resume for opens1/2 of j_4b33rx;
it does not identify music or prove stale audio after seek.

First content-file attempt omitted /dev/shm and failed in
platform_shared_memory_region_posix.cc with SIGABRT, before any window.
This fixture setup mistake is unrelated to the original tooltip callchain;
original online trials already prepared /dev/shm. The corrected content-file
fixture prepares /dev/shm and is running separately. Never relabel either
fixture as online video evidence. Review actual native frame before relying
on the button's title hover to reproduce the original failure.

Source-level repair candidate (not implemented/built): TooltipAura already
passes the actual context root. NativeWidgetAura::InitNativeWidget supports
ParentWindowWithContext and retains bounds/input/lifetime within the existing
compositor. Explicit native-widget selection for Cobalt tooltip may avoid a
second DesktopWindowTreeHost. Confirm parenting, bounds, repaint/destruction
and source build/import ABI before implementation. Simply setting compositing
capability true or disabling tooltip is not a tested fix. Full source build
configuration is under ignored build/phase335-engine-source; host currently
has git,12 CPUs/15GiB RAM, no gn/ninja/clang on PATH.

Corrected /dev/shm content-file run ended18:36:26.802724+07 with no tooltip
FATAL. Both captured native frames are blank dark, so DOM/title hover was
not demonstrated. Post-close physical/SHM/aliases are exact; heap786176→818944
is reported separately. The tool's final assertion was wrong: assert_live_resources
expects ELF live and equal heap, whereas this is ELF pair live with persistent
cache. This is a tool failure, not proof of a new guest resource leak. It is
not a successful tooltip reproduction or HTML acceptance. Revised native HTTPS
title fixture uses the existing diagnostic210s wait (original web fixture allows
600s), SB16 as in the online profile, actual ready HTTP request and correct
pair resident metadata. Heap retention remains disclosed. This job is active
in phase335-tooltip-engine-210.log, session85121; no claim of its outcome yet.

Public log copies normalize line-end whitespace; raw originals remain unchanged in ignored build bundles. SHA256SUMS describes these public copies.

## Confirmed deterministic engine reproduction,18:39:39+07
Corrected native HTTPS fixture `oojztkc8` completed successfully as a fault
reproduction: HTML/app.js/ready and actual pointer180,200 callbacks arrived
97.12–97.74s after start. Cobalt requested176×24,windowed=false,primary1 still
live. MyOS returned ENOTSUP(-95), then FATAL/SIGABRT6 with ALL16 core returnPCs
identical to online `j_4b33rx`. This establishes the exact tooltip creation
failure, not merely a guessed small-window limitation. Physical257743/SHM0/
aliases0 returned exactly; heap786176→827136 is disclosed separately.
The pre-hover native screenshot still shows Cobalt's YouTube splash; it does
not visibly demonstrate the HTML button. DOM/JS callback and matching fault
chain are the actual diagnostic proof. No network/movie/audio acceptance is
claimed for this test page. Private fixture CA/keys never enter normal image.
No QEMU remains. Next: choose/build a real tooltip-in-root compositor adapter,
verify tooltip drawing/bounds/lifetime, full applicable regression and actual
normal repeated/freshboot YouTube controls, then owner audiovisual signoff.
