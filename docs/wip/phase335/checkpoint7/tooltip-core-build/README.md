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
