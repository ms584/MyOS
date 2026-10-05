# MyOS project state

Updated: 2026-10-05 17:24 +07. Repo /home/xytron404/myos, WSL Ubuntu, main.
Source checkpoint eb31a69 TCP server; normal BIOS/UEFI retained; docs8b84f24.
Origin/main3cbd25f, fetched15:42:43 +07. Owner pushes only; NEVER push.

## Scope and measured progress
- Phase33.1/33.2 OPEN; native Cobalt web acceptance0/5 (0% verified).
- Production registered212/242 (87.6%); actual paired214/242 (88.4%),28 missing.
  TCP server full/scoped1701 terminal0 PASS16:31:49; normal BIOS/UEFI restored.
  Native server and actual core/provider paired BIOS/UEFI PASS (not web execution).
- Counts are binding/fixture coverage, NOT full API semantics or phase percent.
- Phase32/33 NOT PASSED; no33.3. YouTube playback unproven.

## Current work and evidence

- Owned SB16 PCM+typed Float32/Int16 sink committedf895303; production62023
  BIOS+UEFI PASS13:24:24: waveform, ring/pause/EOS, malformed input, exhaustion,
  callback Destroy, fault/Escape cleanup/relaunch/exact pages+heap.
- Owner restarted WslService; production pair-q4xi8xvv PASS14:03. New audio
  full58842 terminal0 PASS14:43:40; normal BIOS+UEFI restored.
  Audio pair-execution40404 terminal0 PASS14:48:20 BIOS+UEFI; final hashes unchanged.
  Normal hashes and full evidence: PHASE332_PCM_WIP.md.
- Actual pinned audio_sink/media/drm header ABI native BIOS+UEFI67207 PASS14:03;
  private4Fohgm. Upstream inline IsValid is null-only; target helper checks
  stale registry separately. PHASE332_PCM_WIP.md. Pinned fixture promotion pending.
- New media policy8 +sendto/recvmmsg2 private BIOS+UEFI87168 PASS14:32:01;
  RAM64/256 budgets, concurrent/relaunch/resources, shared caller+socket timeout,
  WAITFORONE and original descriptor generation across vector receive.
  Production77672 terminal0 PASS14:58:33: normal BIOS+UEFI, media/message both
  profiles plus actual pinned core/provider paired adversarial/rollback/resources.
- activity.log bounded48MiB. NEVER read it through run_logged.py.
- Private filesystem inode/atomic rename unit/native BIOS/UEFI PASS; filesystem
  61299 and explicit ATA/storage52587 PASS16:11/16:12. No new POSIX export credit.
- Private sparse32GiB/entropy native42995 BIOS/UEFI PASS16:56:00; exact resources,
  no entropy failure closed. Real VM ASAN/UBSAN allocation rollback PASS16:59:37.
- Genuine Cobalt resource trace86372 terminal0 17:22:11: opens pinned
  /assets/cobalt_shell.pak successfully; requests mmap(fd65,length1318438,
  PROT_READ,MAP_SHARED,offset0). Anonymous-only runtime rejects this, then
  resource-load FATAL/SIGABRT6; zero HTTP requests, exact heap125184 reclaimed.
  Verified135 pinned resources. Diagnostic28 abort thunks never succeed/count.
- Private sparse full2835 terminal1 17:22:14: isolated tree omitted third_party
  compiler-rt/emutls.c. Harness fixed, same source full retry90043 LIVE17:23:53,
  /tmp/myos-sparse-regression-oH6bTA/sparse-full-retry.log. No full PASS claim.

## Existing foundations and limits

- Clock/pthread/descriptors/signals: PHASE331_WAIT.md, PHASE331_PTHREAD.md,
  PHASE331_CROSS_THREAD_SIGNALS.md. Native TLS/thread/C11 adapters tested.
- Native UI/events/SABI launcher and genuine Mesa/EGL/GLES rendering tested;
  PHASE332_RENDERER_CLOSURE.md and PHASE332_PAIR_EXECUTION.md, not Cobalt web.
- Async owned IPv4 DNS/UDP+TCP fallback tested; IPv6/CNAME/full flags open.
- Loader restricted two-module pair/one PT_TLS; no generic DT_NEEDED/DTV/unload.
- Actual Cobalt resource/emutls/thread/RAM behavior unmeasured (eight workers).
  Absolute path127 bound remains; asset hierarchy supports longer relative names.

## Next work

1. Poll SAME90043; freeze its source/tests/images until terminal. Implement
   genuine file-backed mmap (readonly immutable MAP_SHARED required by pak),
   preserving fd offsets/lifetime, protections/EOF/rollback; no anonymous fake.
   Engine tree /tmp/myos-sparse-vm-BCcMw4; PHASE332_SPARSE_VM_WIP.md.
2. Next28: file11, audio config2, player13/decode-target2. FS inode prerequisite
   private only; PHASE332_FS_IDENTITY_WIP.md. No fake handles/codecs/success.
3. Native Cobalt HTML+JS/DOM/timers, input, verified concurrent HTTPS,
   >=60s interaction and close/relaunch/resources; only then close33.1/33.2.

Background: HANDOFF.md; detailed current batch: PHASE332_IMPORT_BATCH_WIP.md.
