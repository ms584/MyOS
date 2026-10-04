# Phase33.2: original Gallium completion fences through native EGL

2026-10-04 (+07), baselineaaf4bff. Phases33.1/33.2 OPEN; no33.3/push.
Imports163/242=67.4% supported subset; actual Cobalt acceptance0/5.

## Actual implementation

user/platform-osmesa-fence.c compiles the original pinned/MyOS-configured
osmesa.c in its translation unit and adapts that frontend's actual state tracker.
It does not reimplement GL or embed the configure-only fake TLS runtime.
Create calls original st_flush, wait calls pipe_screen.fence_finish, release
calls fence_reference. Server wait uses the actual fence_server_sync when
present; original softpipe has none, so it checks actual zero-time completion.
Softpipe executes synchronously and returns its own completed token (1), not
an asynchronous hardware fence. The adapter does not fabricate that token.
Its screen lifetime survives context teardown until the original manager exits.
Static linking selects this original frontend TU, not a second copy from the
old archive; duplicate strong frontend definitions would make linking fail.

The EGL driver implements real KHR_fence_sync/KHR_wait_sync create/client wait/
server wait/destroy contracts, initialized with original attribute validation.
Sync resources retain display lifetime and use the final-core-reference callback
from PHASE332_EGL_LIFETIME.md. Status is atomically published/read, including
the original core fast path and query, to avoid cross-thread status races.
Core's EGLAttrib eglCreateSync entry remains unavailable until the1.5 contract
is implemented; do not toggle KHR_cl_event2 to bypass that guard. No reusable,
OpenCL/native-FD/image extension or complete1.5 contract is claimed.

## Native tests and evidence

Strict patch application/18 units compile PASS18:32:33 core-oqchddg5:
17 production units (including genuine OSMesa adapter), one white-box test TU.
Native BIOS window dgtafpln/UEFI window6ofaylet/pbufferkt9dtqk5 PASS;
session91809 terminal exit0 18:40:03, two launches each/exact resources.
Both window serial logs show two eight-waiter/final-pin success markers.
Logs in
build/renderer-evaluation/native-egl-fence-final.log,
native-egl-fence-final-uefi.log, native-egl-pbuffer-after-fence-final.log.
Initial69110 failed18:34:02: fixture tried8 additional workers while the render
worker occupied one of the kernel's8 slots. No production limit was relaxed.
Corrected lifecycle waits for render-worker join, then uses all8 worker slots
from main. The fence is deliberately kept after its creating context is destroyed.

Tests cover queued pixel write before creation (no Finish before fence), server
ordering/completed pixel result,8 context-free waiters each16 iterations of
zero/FOREVER+flush waits and status queries, no-context create/server-wait errors,
invalid attributes/type/query with unchanged output, bad server-wait flags,
double destroy and exact last-core-pin destructor. Original shader/texture/text/
QMP orientation/input/deferred destruction/two launches/resource checks remain.
Synchronous completion cannot exercise a real unsignaled timeout or asynchronous
GPU scheduling; do not claim those branches tested or replace them with mocks.

## Remaining work and reproduction

EGL still reports1.4; pinned SbEglInterface requires1.5. Genuine image sharing,
OpenCL-absent semantics, context/colorspace/surfaceless requirements, exact SABI
tables and production module coexistence remain. Also audit window ownership,
terminate/reinitialize, single-buffer semantics and engine thread/RAM limits.
MyOS worker limit8 is now directly evidenced; actual Cobalt thread needs unproven.
Native web/HTTPS/input>=60s/relaunch acceptance remains0/5.
No production kernel/runtime changes; full9925 remains architecture evidence.
Normal four hashes unchanged18:39:08; all jobs terminal, no image owner.
Run through tools/run_logged.py:
```
python3 tests/test_native_renderer.py --renderer-probe build/renderer-evaluation/target-config-snob6k4x --configuration build/cxx-target-evaluation/configure-_1j4xqjo --gles --egl-core build/egl-core-evaluation/core-oqchddg5 --egl-window
```
Add --uefi for UEFI; omit --egl-window for the pbuffer regression.
