# Phase33.3 checkpoint3 - open, not acceptance
The pinned native Cobalt core now displays real decoded VP8 frames through its
supported Starboard punch-out/Viz underlay path, above the native audio sink.
Both sessions serve the frozen WebM over ordinarily verified guest HTTPS/MSE.
QEMU captures framebuffer PNGs and the original output PCM WAV in one session.
No host player, decoder, transcoder, or MYV1 participates in playback.
The ZIP contains the original WAV bytes losslessly; SHA256 identifies them.

First session:22 distinct video crops,40.60s captured audio. Second session
reduces duplicate MMIO writes; its raw evidence and measured PCM windows are
included. These are observation runs, not A/V sync or RAM acceptance.
A heap delta of61440bytes after Cobalt exit remains unresolved. Do not label
these runs full cleanup PASS. DOM clocks do not prove presentation sync.

Native underlay contract runs validate real guest pixels: opaque occlusion,
premultiplied alpha, unchanged pixels after malformed requests, timestamp
rejection, clear/reset, bounds move, hide/show, cancel/fault/relaunch and exact
fixture resources. The native asset-only RAM checkpoint measured12636160bytes
including preloaded codec/provider delta and fixture stack; this is NOT the
Cobalt/MSE/compositor media RAM budget measurement.

Full regression2 stopped on missing process-probe boot markers after app helpers
restored a normal image. The harness now explicitly rebuilds PROCESS_TESTS=1
before memory boot assertions without changing any marker. Focused memory suite
passed. Full regression3 for the promoted underlay architecture is still pending.
Normal BIOS/UEFI restoration and final acceptance remain required.
No TLS private keys, asset disks, executable core/provider binaries are included.
