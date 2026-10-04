# Phase 33.1/33.2 native renderer OS-information work

2026-10-04, follows e12f51b; scoped/full regression complete, local checkpoint.
This is a renderer dependency slice, not a backend or Cobalt acceptance gate.

## Real managed-memory contract

SYS_MEMORY_INFO50 returns immutable total managed usable pages (selector0),
or an instantaneous free PMM page snapshot (selector1); others returnEINVAL.
No pointer output, allocation, host query or reservation is performed.
memory_init captures total_count from its availability bitmap before allocating
the kernel heap. The bitmap is built from the BIOS/UEFI boot memory map, after
reserved overlaps and partial pages are excluded, above1MiB and below256MiB.
Thus the value is RAM actually managed by MyOS, NOT installed/host RAM, firmware
reserved RAM, memory beyond the existing PMM limit or free kernel-heap bytes.
Free pages are available to eager guest VM allocations, not a guaranteed future
reservation; other threads may consume them immediately after the query.

sysconf exposes pinned musl _SC_PHYS_PAGES85, _SC_AVPHYS_PAGES86 and existing
_SC_PAGE_SIZE/_SC_PAGESIZE30. SDK declarations use those exact values; the
native test checks them against the SHA256-sealed musl1.2.6 archive, not glibc.
Successful queries preserve errno. Existing unknown-name EINVAL is unchanged.

mesa_build_target.patch adapts the actual pristine Mesa25.0.7 os_misc.c to use
these sysconf calls for total/available bytes and page size without enabling
Linux/proc or claiming a POSIX platform wholesale. Its available query accepts
a truthful zero count; zero-free-PMM exhaustion has not been forced by this test.
The u_dl.c change only supplies stddef.h for NULL in the existing unavailable
branch: plugin open/lookup remain unavailable, close remains harmless. The
restricted probe still selects in-process softpipe OSMesa, not native EGL/GLES.

## Real loader defect found by the consumer

The fixture's hidden utility functions leave only undefined imports in dynsym.
GNU hash has empty buckets and omits those unhashable symbols; its symoffset
alone need not equal the full independently bounded SysV nchain count.
Previously the loader rejected this valid both-hash ELF before relocation.
The fix uses SysV's checked total only when every GNU bucket is empty, retaining
the actual zero-chain GNU byte extent. Nonempty hash count agreement, all bounds,
relocation-symbol bounds and malformed bloom/bucket checks are preserved.
No generic dependency/dlopen or unbounded GNU-only import-table claim is made.

## Evidence and running jobs

- Capacity build90997 exit0 10:05:05: PROCESS_TESTS kernel261304/cap262144,
  before the small empty-GNU parser fix. Final test-profile size pending full.
- First fixture/patch attempts failed on patch grouping, upstream zero
  initializer warning, pthread handle type and C++ pointer conversion. Fixed
  the fixture/patch, without relaxing pointer/type checks or assertions.
- Native import-only ELF exposed the parse defect; after the real fix, first
  lifetime assertion revealed expected retained anonymous page tables. VM
  currently retains those tables until process reap. The fixture now checks
  exact data-page return, warms mapping/stack tables, then requires exact stable
  free counts across repeated maps/workers AND complete process resource reap.
- Initial scoped74376 exit0 10:14:28: C/C++ BIOS16/64/256MiB and UEFI128/256MiB,
  30 launches,8 workers twice/launch,1/17/65-page maps and complete resources.
  Reported managed page totals: BIOS3808/16096/65248; UEFI20217/52985. Values
  match actual kernel PMM bitmaps; profile differences are not invented RAM.
- Final chain93613 exit0 10:55:33 (started10:15:39): stronger scoped tests include empty
  GNU hash proof and malformed SysV count/GNU bloom rollback on every RAM
  profile; scoped prefix PASS observed10:18:14, BIOS native-eoyxv3lq/UEFI
  native-nvtf4_lr,30 positive launches/10 malformed rejections/resources.
  Full test_system.py, normal BIOS/UEFI restoration, pinned
  contract/staging/OOM checks and hashes all PASS. Normal BIOS/UEFI images
  restored. Shared-image owner released; no competing architecture build ran.
- Isolated renderer87879 exit1 10:19:42, target-config-yi0713xx: configure and
  boundary PASS; complete keep-going compile still fails11 units/11 errors
  (prior13/16). Actual os_misc.c and u_dl.c failures clear. C++ headers/stdlib,
  sys/file.h, mkdir and line-buffered stdio remain; no native backend selected
  or renderer success. The compile did not touch shared image outputs.

Reproduce: tests/test_mesa_os_info.py [--uefi] through tools/run_logged.py.
Source: memory/memory.c, kernel/syscall.c, kernel/elf-link.c, platform-runtime.c,
SDK unistd.h, mesa_build_target.patch, user/elf-mesa-os-info.c.
No new Cobalt binding count:163/242 coverage67.4%, actual web acceptance0/5.
Normal kernel253112/runtime29136 bytes. SHA256 from terminal93613:
```
04a3d7e2c95d41ddbd31ee73b842e1caf8f8bcd3a1e5e8d9731522beb3c6602c kernel.bin
a7e6ecfb7264f6f8bd43b46c28c15d44e393b2b18e6570ec14a8675d99642100 platform-runtime.bin
40eb2ba845cd0616a92e8c63ca740f89f8a8992c5c8d1ba360831698c816df81 myos-current.img
4727ce058723b5d3fe7d1fd871e196df3e94b17c34b4412770d4ef71842ed258 myos-uefi.img
```
Real target C++/EGL and cross-task signal/wait/SIGINFO remain required.
No Phase33.3, push, fake renderer success or phase closure.
