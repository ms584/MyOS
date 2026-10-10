# Phase33.5 checkpoint7 — source-built tooltip repair in progress

NOT ACCEPTED. Source patch applied; GN generation succeeded; native Evergreen
Cobalt compile is active. No linked core or runtime result exists at this checkpoint.
Root normal kernel, MRT, images and official core have not been replaced.

Checkpoint6 established the actual 176x24 tooltip request returning ENOTSUP,
followed by FATAL/SIGABRT; all16 captured core return PCs match the online failure.
This patch uses NativeWidgetAura for Cobalt tooltip widgets so they composite in
the existing context/root. It does not fabricate a second SbWindow or disable tips.

Source27.lts.3 is pinned to3befd332486952f6603ae3baf2a8713878d6e200.
Patch hashes, tool/depot identities, generated metadata and normal artifact hashes
are recorded alongside this file. No host playback/decoder/transcoder is involved.

## Build preparation and actual outcomes

The first GN attempt failed because pinned depot_tools was not bootstrapped.
ensure_bootstrap succeeded, then GN failed on missing LASTCHANGE.committime.
After generating upstream metadata, GN made29494 targets from4132 files and
returned0 at20:35:03+07. The GPU/Skia generators initially emitted zero hashes
because GitHub commits lacked the default Gerrit Change-Id. Those headers were
regenerated with --filter=. using their real pinned repository HEAD before compile.
LASTCHANGE and committime already contain the actual source HEAD/date.
Failures and the successful GN log are retained; no fallback metadata is accepted.

Actual commands are recorded by tools/run_logged.py. Build workspace is
build/phase335-core-workspace/src; pinned depot_tools is prepended to PATH with
DEPOT_TOOLS_UPDATE=0 and DEPOT_TOOLS_DIR pointing to that checkout.

```sh
# From the workspace root, use upstream metadata generators:
python3 src/build/util/lastchange.py --filter=. -o src/build/util/LASTCHANGE
python3 src/build/util/lastchange.py --filter=. -m GPU_LISTS_VERSION --revision-id-only --header src/gpu/config/gpu_lists_version.h
python3 src/build/util/lastchange.py --filter=. -m SKIA_COMMIT_HASH -s src/third_party/skia --header src/skia/ext/skia_commit_hash.h
python3 src/build/util/lastchange.py -s src/third_party/dawn --revision src/gpu/webgpu/DAWN_VERSION
# From src, GN already completed; compile is running:
python3 cobalt/build/gn.py -p evergreen-x64 -c gold --no-rbe
third_party/ninja/ninja -C out/evergreen-x64_gold -j6 cobalt
```

Do not launch another Ninja against this output while the recorded job is live.
The official upstream build instructions are linked in scope.json.

## Required next verification

Verify the actual linked core SHA/build identity/imports and preserve dependency
pins. Test actual visible HTTPS DOM before hover; then inspect tooltip drawing,
bounds, hover-away hiding, recreation and cleanup in MyOS. Preserve the original
failure as a negative control; absence of abort alone is insufficient.
Recheck affected native prerequisites and real normal online Rickroll controls,
repeated opens, fresh boot and resource stability. Run mandatory architecture/full
relevant regression and restore normal images before promoting a final bundle.

Owner has not confirmed music or controls. Audio input in this chat is unsupported;
PCM/RMS is not proof of Rickroll music or stale-audio absence. Phase33.5/33 and the
independent audible/interactive Phase32 gate remain open. No push.

Prepared private source-core retest and freeze-core.py are archived here. Both
passed Python syntax checking only; neither has run against a new linked core.
After the existing compile log reports terminal exit0, freeze-core.py validates
source/patch/normal hashes, records real ELF/header/dynamic identity, and writes
build/phase335-core-workspace/core-build-identity.json. It rejects pending/failed
builds. Set MYOS_TOOLTIP_CORE_MANIFEST to that file for the private retest.
The retest waits for actual rendered page pixels, records two hover/hide cycles
in each of two launches, and checks no FATAL plus exact physical/SHM/alias cleanup.
Saved hover images still need visual inspection; this fixture cannot pass YouTube
or sound/music criteria. Source files live in the private diagnostic worktree.

Private online observation runner now accepts --source-core-manifest explicitly.
Its helper verifies the finished compile log, exact source/patch/core hashes and
labels all derived-core observations diagnostic. The default official-core hash
check and resource/provider/normal image checks remain intact. This prepares real
online follow-up; it has passed syntax checks only and has not launched a VM.

20:53+07 concurrency follow-up: measured12 logical CPUs,55.7%busy and11.5GB
available RAM. The owned6worker Ninja was deliberately interrupted (Ninja2,
wrapper1) to continue the same output with8 workers. A failed resume wrapper
truncated the standalone old log before archiving; its full interleaved activity
segment was recovered privately and hash-recorded here. Failure log and safe
interrupt tail retained. Source and completed objects were not deleted. Active
resume session is18106, using the same compile log path and identical source.
These are host build operations, not a guest failure or playback acceptance.

Private native HTTPS/JavaScript setup now supports MYOS_SOURCE_CORE_MANIFEST.
The original official baseline core hash is still checked first, then the shared
helper verifies the derived core before it is placed in the guest asset disk.
Its build identity is recorded in each output directory. The standard VP8/Opus
MSE observation inherits this setup, so affected prerequisites can be rerun using
the actual new core. This change has syntax checks only; no new guest run occurred.
The standard WebM fixture is a prerequisite/regression input and never substitutes
for online YouTube/Rickroll acceptance. Ring3/kernel underlay contract fixtures do
not load Cobalt and remain separate; changing their filenames would not retest it.
