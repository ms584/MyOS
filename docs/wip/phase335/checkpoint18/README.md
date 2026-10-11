# Phase33.5 checkpoint18 — owner trial FAILED, audiovisual stutter

2026-10-11. Owner tried the current normal manual bundle tjks53iv and reported
both picture and sound stutter. No owner acceptance was received. Agent reviewed
sparse screenshots extracted from the provided Windows recording; it did not
listen to audio or watch the whole22-minute recording. The original802MB video
was hashed in place, not copied/deleted. Windows reading of this evidence is
not host playback/decode substituting for the online MyOS demonstration.

Manual serial metrics:3155presented,5326decoded,2171dropped,
max-sync900037us (>100000us required limit),640x360 source first stream.
Repeated native rebuffer pauses/resumes are retained as failure evidence.
A subsequent256x144 stream began, but this does not erase the earlier failure
or prove smoothness. Physical pages/SHM/aliases reclaimed at cancellation;
that cleanup does not make audiovisual acceptance pass.

One bounded diagnostic trace run is active using unchanged production source,
1024MiB, one QEMU,1FPS,nice10. It is diagnosis, not timing acceptance.
No root cause/fix is claimed yet. Normal images remain preserved.
Phase33.5/33 and separate Phase32 stay OPEN. No push.

GTK/PulseAudio trace reproduces severe stalls: presentation-max3730000us,
max-sync660037us and77rebuffers by224.92s (still live, not final).
The headless/WAV diagnostic did not reproduce these multi-second stalls,
and was interrupted by an unsupported esc key action; this observer mistake
is corrected using its existing close action. It is not counted as a pass.
QEMU display OpenGL will be checked as a candidate virtual-display setting;
this does not move YouTube decoding/rendering out of MyOS. No fix claimed.
Official display-option reference: https://www.qemu.org/docs/master/system/qemu-manpage.html

Baseline GTK/PulseAudio diagnostic completed exit0; this means observer
completed, not audiovisual pass. See gtk-pulse-diagnostic.json. OpenGL display
boot probe reached normal shell, but EGL/Zink warnings remain and the actual
hardware/soft renderer is not established. A paired online GL diagnostic is
active; do not claim acceleration or a performance repair from the boot probe.

GL paired diagnostic completed exit0 but fails smooth playback: presentation
max4830000us, observed maxsync620037us. No GL fix promoted. Window restored
to foreground on owner request; native caption WARN:COPY MODE was observed.
WSLg Weston reports shared-memory Input/output error. This host-display defect
is proven, but causality for all stutter still needs before/after testing.
Owner has saved work and authorized WSL restart; all QEMU jobs are terminal.

Visible production post-restart observer completed exit0 but owner confirms
only Linux icon, no visible QEMU picture. Foreground API success did not prove
rendered visibility. Repeated shared-memory EIO persisted. No acceptance.
Next minimal check launches normal manual QEMU from a visible Windows Terminal
with Ubuntu kept open; optional local QMP only permits real native inputs later.
It does not change normal images, provider/core or import a movie.
