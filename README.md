# MyOS

MyOS is a freestanding x86-64 operating system with BIOS and UEFI boot,
a framebuffer desktop, a shell, RAMFS, preemptive scheduling, and Ring 3
ELF applications. The desktop includes Settings, Calculator, and Snake.

## Build and try

From the repository root on Linux or WSL Ubuntu, with GCC, binutils, NASM,
Python 3, and QEMU available:

```bash
bash build.sh
```

For the BIOS quick start, use the normal image produced by the build:

```bash
qemu-system-x86_64 -m 128M -drive format=raw,file=build/myos-current.img,if=floppy -boot order=a
```

Type `help` at the MyOS shell to list commands. Native Cobalt online
playback was demonstrated in QEMU using a separately prepared engine and
asset bundle. The source checkout provides the kernel build. Some development
and acceptance tools require private fixtures or separate native assets.

## License

Original MyOS material uses the [MyOS Non-Commercial Source-Available License](LICENSE).
Keep attribution when sharing. Commercial use and sale require written permission.
Third-party material retains its original terms. See the notices under
[THIRD_PARTY_NOTICES](THIRD_PARTY_NOTICES/phase333), [assets](assets),
[vendor](vendor), and [third_party](third_party).
