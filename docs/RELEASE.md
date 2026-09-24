# MyOS BIOS/UEFI release

This release is an educational MyOS desktop tested in QEMU. It contains
Settings, Calculator, Snake and a Shell. The apps run inside the kernel.
USB-image validation is virtual; real hardware is deferred. Networking,
browser/YouTube, USB HID and UEFI Secure Boot are not implemented.

## Build and validate (source checkout, Linux/WSL Ubuntu)

```bash
sudo apt-get install gcc binutils nasm python3 qemu-system-x86 ovmf gnu-efi dosfstools mtools
cd ~/myos
python3 tools/run_logged.py -- python3 tools/release.py
```

The test/build/package workflow takes several minutes. Watch
`tail -f build/activity.log` in another terminal. It preserves user data images.
Pillow and DejaVu are optional only for regenerating the already bundled font.
The archive and checksum are in `build/release/`. The manifest identifies the
tested source and toolchain. Packaging alone rejects stale validation evidence.

## Run from a source checkout

```bash
python3 tools/run_image.py --firmware bios
python3 tools/run_image.py --firmware uefi
python3 tools/run_image.py --firmware uefi --media usb
```

Add `--data build/myos-data.img` to create or reuse a separate MyOS save disk.
It is never replaced if it already exists. Enter `desktop` at `myos>`; click
an app in the top bar. Q/Esc exits to Shell. Snake: arrows/WASD, P pauses, R
restarts. Calculator: digits/operators, Enter evaluates, C clears. Settings:
choose values and Save. Reboot with the same data image to restore saved data.

## Run an extracted release (no compiler needed)

Verify the extracted contents with `sha256sum -c SHA256SUMS`. For BIOS:

```bash
qemu-system-x86_64 -m 128M -drive format=raw,file=myos-bios.img,if=ide,index=0 -boot order=c
```

For UEFI, use a private copy of the OVMF variable store:

```bash
cp /usr/share/OVMF/OVMF_VARS_4M.fd myos-vars.fd
qemu-system-x86_64 -m 128M \
  -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd \
  -drive if=pflash,format=raw,file=myos-vars.fd \
  -drive format=raw,file=myos-uefi.img,if=ide,index=0 -boot order=c
```

For virtual USB boot, replace the last drive option with:

```text
-drive format=raw,file=myos-usb.img,if=none,id=bootusb -usb -device usb-storage,drive=bootusb,bootindex=1
```

All images require x86-64 with NX. Native UEFI acceptance uses 128 MiB and
800x600 BGR GOP, an allocatable low kernel range and an unsigned loader.
The USB image uses firmware's preboot storage support. Kernel input remains
PS/2; native USB controllers/input/storage and modern SATA/NVMe are future work.
Font and GNU-EFI license notices are included in `licenses/`.
