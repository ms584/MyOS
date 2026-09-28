# Phase 33.1 Cobalt ABI contract register

This is an implementation contract, **not an implemented POSIX or Starboard
layer**. [PHASE331_IMPORTS.csv](PHASE331_IMPORTS.csv) contains every one of the
243 pinned imports exactly once, its binding strength, owning layer, contract
group and pinned source. All entries remain `unimplemented`: existing kernel
functions do not automatically satisfy these user-space ABIs.

The [header manifest](PHASE331_ABI_SOURCES.json) records 42 inspected upstream
header hashes at source commit `3befd332486952f6603ae3baf2a8713878d6e200`.
109 symbols link directly to a matching ABI-wrapper declaration; remaining
rows link to the release's export map, whose includes identify their native
declarations. These source signatures and per-function failure rules are
normative. The table below adds MyOS ownership and acceptance requirements.
Do not treat a group summary as a substitute for a function's prototype.

## Binary boundary

- Target: x86-64 little endian; pointers and size_t are 64 bits, int is 32 bits.
  The user library follows the x86-64 C calling convention, not INT 0x80's
  existing three-argument MyOS syscall convention. A new library shim must
  translate between them and validate arguments at the kernel boundary.
- Cobalt's modular wrappers normalize musl ABI types. For the inspected x64
  headers, mmap offsets are signed 64-bit; timespec is two signed 64-bit fields.
  `musl_stat` has size 144, alignment 8; st_size offset 48, st_atim offset 72.
- Pthread opaque wrappers have pointer alignment (8 here): mutex 80 bytes,
  mutex attributes 40, condition 96, condition attributes 40, thread
  attributes 80, once 64, rwlock 80. Thread/key identities are pointers in this
  ABI. These are wrapper storage sizes, not MyOS's existing scheduler structs.
- libc errno storage must be per-thread. Ordinary POSIX -1/errno errors,
  pthread error-number returns, allocator failures and Starboard invalid-handle
  results differ. Do not leak negative MyOS syscall results through every API.
- Current ELF fixtures use MyOS's existing syscalls and a small custom entry
  convention. They do not establish this Cobalt ABI. Phase 33.2 adds in-image relocation binding and Ring 3 constructors for
  self-contained ELF fixtures. It still has no external platform bindings
  or ELF TLS implementation.

## Required groups and acceptance checks

| Contract | Count | Owner and semantics to implement | Required evidence before binding |
|---|---:|---|---|
| allocator | 7 | User libc allocation over process VM, size/alignment/overflow validation; realloc failure retains original allocation; posix_memalign uses its own error convention. | Alignment, growth, exhaustion, zero-size policy, free/realloc lifetime tests. |
| vm | 6 | MyOS VM areas for mmap/munmap/mprotect and related advice/query/sync; MAP_FAILED and errno on failure; W^X and file offsets preserved. | Overlap, huge lengths, partial unmap, permissions/faults, OOM rollback, mapped file bounds. |
| thread | 48 | User pthread/sem wrapper over kernel scheduling and blocking primitives; TLS destructors, join/detach, once, mutex/cond/rwlock semantics. | Contention, timed waits, ownership errors, teardown, no lost wakeups; FP/SIMD/TLS survive preemption. |
| errno | 1 | User thread-local errno address. | Two preempted threads retain different errno values. |
| clock | 2 | Realtime/monotonic clocks and interruptible sleeps with signed 64-bit timespec. | Invalid nanoseconds, past deadline, interrupt/remainder, monotonic behavior. |
| signal | 4 | Defined signal actions/delivery/wait/raise behavior in the user runtime. | Correct handlers/masks and faults without corrupting interrupted state; unsupported features fail honestly. |
| network | 23 | Concurrent user sockets, resolver, options, address and interface layouts; no aliasing of the current single kernel TCP stream. | Multiple simultaneous connections, partial I/O, EOF, errors, nonblocking timeout/cancel. |
| wait | 8 | Descriptor readiness, epoll/poll/eventfd, pipes and FIONREAD. | Wakeup/coalescing, close while waiting, finite timeout, invalid descriptor, backpressure. |
| file | 41 | User file/dir/metadata APIs, signed offsets and seek, descriptor duplication, sync, permissions and notifications. | Large file random reads, EOF, offsets, metadata layouts, errors and cleanup; mutable snapshot regression. |
| system | 13 | Process/thread IDs, CPU/resource limits, scheduling information, system queries. | Return actual capabilities and supported policy; no fabricated CPU features or scheduler success. |
| constant | 4 | Correctly typed platform data for path limits/separator and thread limit. | Data-symbol relocation and limits match actual implementation. |
| graphics | 10 | Starboard window/decode-target lifecycle plus complete EGL/GLES interface tables. | Context/thread ownership, buffers, texture sampling, redraw and input on MyOS; no missing function-pointer slots. |
| media | 23 | Capability negotiation, sample queue, timestamps, decoded frames, seeking, rate, volume, EOS and teardown callbacks. | Real compressed samples, A/V clock, seek flush, malformed input, stop/restart; advertise only working codecs. |
| audio | 5 | Starboard sink format/frequency queries and streaming lifecycle. | Conversion as needed, callback threading, audio clock, underrun/latency and cleanup through native audio. |
| storage | 5 | Starboard persistent record open/read/size/close/delete semantics. | Durable state, missing record, bad handle, reopen and bounds. |
| event | 2 | Timed event scheduling/cancellation tied to engine event loop. | Cancel before firing, callback ordering and shutdown without use-after-free. |
| platform | 23 | Paths, properties, locale, logging, resources, extension/capability queries, stop requests and diagnostics. | Consistent actual values, bounded strings and stop responsiveness; unknown features report unavailable. |
| optional | 16 | DRM, microphone and speech have strong imports even if disabled. Provide only documented unsupported behavior until real support exists. | Capability and create/use results agree; no dummy successful sessions/devices. |
| certification | 1 | SbSystemSignWithCertificationSecretKey: unresolved platform/frontend dependency. | Determine whether selected YouTube deployment requires credentials; do not invent keys or successful signatures. |
| weak-tls | 1 | V8 weak TLS-initializer symbol: unresolved call-site and loader semantics. | Establish null weak-binding behavior from pinned source before execution; not a dummy callable address. |

The read-only asset volume added in this slice supplies an internal loader
reader; it does **not** implement the 41 POSIX file imports or expose new user
file syscalls. Likewise the restricted ELF fixture runner does not implement
the VM/thread groups above.

## Renderer and codec feasibility decisions

This slice intentionally leaves engine launch blocked until these contracts
are real. Candidates below are evaluated directions, not installed libraries.

- **Renderer:** Cobalt imports EGL/GLES interfaces. Current
  [SwiftShader](https://github.com/google/swiftshader) implements Vulkan; GLES
  would add [ANGLE](https://chromium.googlesource.com/angle/angle/+/main/README.md).
  Reject the assumption that SwiftShader alone supplies EGL/GLES. That route
  requires two ports, native C++/thread/VM services and a presentation backend.
  Mesa is an alternative; its [license documentation](https://docs.mesa3d.org/license.html)
  confirms component-specific terms and mostly MIT code. A specific supported
  software driver/configuration and its dependency closure still need a build
  spike before selecting either renderer. No renderer is yet approved as usable.
- **Video/audio:** investigate [libvpx](https://github.com/webmproject/libvpx)
  for VP8/VP9 and [Opus](https://github.com/xiph/opus) for Opus audio. Their
  [license](https://github.com/webmproject/libvpx/blob/main/LICENSE) and
  [license/notices](https://github.com/xiph/opus/blob/main/COPYING) must be
  preserved when vendoring. CPU dispatch/SIMD, allocator and thread hooks need
  native validation. This is a candidate codec pair, not evidence that the
  selected YouTube stream offers it or that demux/media integration is done.
- **Broader codecs:** FFmpeg remains a fallback for demux/decoding gaps. Its
  [license depends on configuration](https://ffmpeg.org/legal.html), so pin and
  inventory enabled components before linking. No host FFmpeg process may supply
  playback for the native acceptance test.

The next contract work is renderer build/dependency measurement, codec build
selection, and the two explicitly unresolved symbols. It can proceed alongside
the next isolated loader step, but blocks claiming full engine integration.

## Recheck the pinned boundary

```bash
python3 tools/run_logged.py -- python3 tools/check_cobalt_contract.py
python3 tools/run_logged.py -- python3 tools/check_cobalt_contract.py \
  --library build/engine-evaluation/libcobalt.so \
  --headers build/engine-evaluation/contracts
```

The optional binary/header arguments verify cached upstream artifacts by hash.
The checker reports implementation status explicitly; a PASS means exact
inventory coverage, not implemented APIs or a functioning browser.
