# Phase33.4 — live signed-URL expiry review plan

Status2026-10-10:NOT EXECUTED; explicit scoped permission required by automatic
approval review. This is a reviewable plan, not an expiry harness or proof.

The proposed diagnostic would reuse one signed Googlevideo media URI originally
obtained by Cobalt inside an authorized MyOS YouTube session. Private URI/tokens
remain in ignored build evidence. Do not print, commit or send them to third parties.
Network traffic would be sent only by MyOS native TLS to the URI's original media
hostname, with certificate/hostname/time verification enabled. No host browser,
downloader, decoder, transcoder or converting proxy participates.

Before implementation, establish the real method/headers/body and media protocol
from bounded native request metadata. Current observed YouTube transport is SABR;
an arbitrary GET, CORS failure, status0 or a fabricated403 cannot establish expiry.
The approved test must preserve an actual supported request and normal TLS,
measure the real UTC expiry, and compare the same URI SHA256 before/after actual
expiry if possible. Do not modify tokens, signature, guest time or credentials.
A previously successful native request may supply the before evidence only when
its method/protocol and byte-response evidence are comparable.

After actual expiry, require an explicit native server error attributable to expiry,
a native UI/error transition, user-input restart to the ordinary public watch page
and fresh site-selected stream, later moving video plus native PCM, and exact
player-close resources with responsive MyOS desktop/shell. If the old request cannot
be replayed faithfully, report the limitation; do not count a controlled fixture as
live YouTube expiry. The existing controlled403/timeout tests remain useful separate
error/restart evidence.

Authorization requested: reuse this session's private signed media URI in this
bounded MyOS-only expiry diagnostic against its original media host. This permission
does not authorize publishing credentials, host fetching/decoding or TLS bypass.
Automatic review rejected the earlier proposed reuse as external signed-token use
without explicit scoped human permission. No replay harness/action was executed.
