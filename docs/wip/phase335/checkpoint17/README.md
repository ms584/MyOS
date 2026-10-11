# Phase33.5 checkpoint17 — pending-seek clock repair (not acceptance)

2026-10-11. All network, TLS, JS, demux, decode, presentation and audio remain
inside MyOS. No host media fetch/decode, MYV1 substitution or fixture for YouTube.

The native trace diagnostic `build/phase334-public-zc6rkiv9` completed after
Windows restart. It proved the paused-backward target used the pre-forward
cached time, while native first-video/audio anchors were already reset correctly.
Cobalt Blink OnPipelineSeeked caches Pipeline GetMediaTime in paused_time_.
SbPlayerGetInfo could report the previous audio anchor before the worker handled
seek_pending. Production clock_locked now reports requested seek_time during
that pending interval; actual hardware consumption remains the playback clock.

A meaningful native regression queries GetInfo immediately after a real seek,
before starting new readers. Old provider8homrhps reports10030437us instead of
requested15000000us and fails. Production VP9cjb_ftis and VP8x0_6ui03 report
15000000us and pass existing playback/pause/resume/seek/EOS/restart/cleanup tests.
Negative and positive originals remain under build; see pending-seek-summary.json.

Normal nontrace/source-core online tjks53iv completed two opens16:24:00+07.
Actual Rickroll images inspected in both opens. First paused pair229.907/238.853
has unchanged movie pixels and zero PCM growth; resume249.170 changes pixels
and adds98304 bytes. Second observed movie endpoints193.497/233.645 change
and PCM grows450560 bytes. These observations are not music-listening proof.
Paused forward/backward/resume UI2:51→2:41→2:42 no longer has the10s jump.
Native seek tickets11/13 start171280000/161280000PTS with correct new clock.
Stream logs confirm WebM/VP9/Opus,48kHz stereo input; native SB16 output11025Hz U8mono.
Source256x144 and640x360 both occurred; do not claim fixed144p throughout.
Max native sync89986us; decoded/dropped counts are disclosed in the JSON.
PMM complete runtime history peak555679744B; heap10254208B is separate and
must not be added to physical RAM. All physical/SHM/aliases reclaimed, desktop
and shell responsive twice. Linked cache217088→290816B/53→57 inodes and
total heap785920→994848→1068576B tracks cache growth. Subtracting linked
file capacities gives non-file heap777728→777760→777760B; fresh boot stays
777728B. This is bounded sampled metadata accounting, not proof of every
allocation's identity or unlimited repeated-open stability. No cache plateau
acceptance from two opens alone.

Fresh normal boot53waebuq completed16:34:36+07 with correct paused seek
2:57→2:47→resume2:48 and later2:50. Native movie window31.908s, all8 sampled
pixel pairs changed and PCM grew; pause pixels/PCM freeze and resume advance.
Maxsync89532us, complete runtime PMM peak548675584B, exact physical/SHM/aliases
and desktop/shell response. Owner audible music/stale-audio review remains absent.
Full system regression ran16:34:55–17:18:25+07 and completed exit0.
Manual assets sealed for tjks53iv. Launcher rejects concurrent real QEMU
(exit2 verified during fresh run), preserving the one-VM resource budget.
Passive normal MyOS boot for the new tjks53iv manual bundle passed (exit0). Owner explicitly has not
tried/listened; stale music/audio and interactive/freshboot signoff are unverified.
Phase32 search/select/play must be decided separately. Phase33.5/33 remain OPEN.

Unchanged resource paths also have the actual earlier xnmkswbc four-open check:
normal5/core match, physical/SHM/aliases recovered and desktop responsive all4;
cache114688→135168→135168→135168B at48→64→64→64 normal inodes.
The clock fix has no allocation/FS/cache changes. See repeated-resource-scope.json
for the explicit reuse boundary: this does not erase the old seek failure or
claim an observed current-provider cache plateau. Normal FS remains64 nodes,
65536B/file, theoretical ordinary buffer cap4MiB excluding SHM/other RAM.
Native current controls and two new closes plus fresh boot are separately tested.

Verified cleanup37 archived old Mesa copies net8605818880B inside Ubuntu;
Windows VHDX compaction/C: refund not yet performed. Current source/provider
change is intentional after cleanup identity preservation. See checkpoint16
receipt and guarded Windows compact script in the Windows review workspace.

## Final technical checkpoint at17:19+07

Full tests/test_system.py completed17:18:25 exit0, including fault and VGA fallback,
and restored all five normal image/runtime hashes exactly. See regression-and-recording.json.
One fresh-boot native screen/audio review AVI was generated (29,232,710 bytes,1FPS).
Its approximate WAV byte-poll alignment is not a synchronization measurement;
original PNG/WAV/timeline and native hardware-clock logs remain authoritative.
No host online-media fetch/decode/player was used. Listening to music, stale-audio
verification and owner interactive acceptance remain absent. Phase33.5/33 stay OPEN.
