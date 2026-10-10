# Phase33.5 checkpoint8: source core built and native tooltip retested

2026-10-11 Bangkok. Full Ninja build terminal0 at03:42:04; actual core SHA256
72a802f90fddd071971bca252a00ffbc824c3ef48ecc19197e6b6c0665848505,
103391688bytes, ELF build ID50d92a3d1e508ee4433be2b34a96cbd8129584f4.
Source27.lts.3/3befd332486952f6603ae3baf2a8713878d6e200 plus the exact
9-line Cobalt tooltip NativeWidgetAura patch in checkpoint7. Source-built gold
configuration is not an official Google binary. Dependency pins/compiler/setup
remain in checkpoint7; private full log build/phase335-core-compile.log.

Private test native-verified-tls-6ioxiz7_ completed03:46:33 terminal0.
QEMU10.2.1/KVM/CPUhost/1024MiB/RTL8139/SB16/800x600; command retained in
build/phase335-after-build.log. Ordinary verified HTTPS fixture CA, guest native
TLS/JS/rendering. Actual bindings missing=0; API18 SABI matched. Two page launches,
two pointer hover/hide cycles each: real DOM background433388pixels; actual title
text visible at approximately[190,216,365,239], disappears on pointer away,
recreates on next hover and next launch. Assistant inspected saved PNGs using
view_image; raw automated drawing flags remain false, separate visual-review.json
records this subsequent image inspection. Only primary800x600 window requests,
no extra native tooltip window, no FATAL/SIGABRT. Physical257743→257743,
SHM0→0 and aliases0→0 after both closes. Heap786176→827136 on first open,
then827136→827136; two samples do not prove long-run warm stability.

Normal root kernel.bin/kernel.elf/BIOS/UEFI/platform-runtime hashes remain exact.
No normal core/provider promotion, no YouTube playback or owner listening proved.
This fixes the reproduced tooltip path in a private diagnostic test; online
third-open behavior still requires retest, alongside affected33.1–33.4 gates,
normal-build Rickroll controls/freshboot/resource stability and regression.
Phase33.5/33 and independent Phase32 audible search/select/play acceptance remain
open. No host browser/player/decoder/transcoder and no fixture substitution for
Rickroll. No private keys, signed URLs, cookies or raw external headers committed.
