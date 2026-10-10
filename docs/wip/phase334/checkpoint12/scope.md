# Phase33.4 checkpoint12 — authorized native expiry protocol work

INCOMPLETE. Owner explicitly authorized scoped signed-URI reuse and necessary
Phase33.4 work. No permission is pending. Production code/images are unchanged.
All media traffic remains MyOS native TLS to the original Googlevideo hostname;
no host media fetch/decode/transcode or converting proxy participates.

Native Cobalt --net_log did not produce a file as configured. --log-net-log works.
The initial64KiB log filled before media. Shared-memory8MiB log exists; raw receive
payloads and a30-second serial-export budget caused diagnostic timeouts. An isolated
kernel collector now retains receive metadata/byte counts, omits only receive bytes
for events79/80/91/135/136, preserves outgoing plaintext request bytes, and handles
batched/fragmented writes with a bounded256KiB allocation freed on close.
A bounded600-second export returned7,747,320bytes;34214complete line records parse.
The container JSON footer is incomplete after forced native ESC teardown; do not
claim a clean complete JSON container. Target requests/response events are complete.

Googlevideo uses HTTP/1.1 POST in this session; page resources use HTTP/2. HTTP/1
framing analysis extracted original opaque1663-byte SABR request body, without any
host media decoding. Its URI/body hashes and native200/1087988received bytes are
in selected-request-safe.json. Signed queries/body/private CA keys stay ignored
under build; they are absent here. Native actual-playback observation0b99a0yv is
private supporting protocol evidence; main acceptance remains checkpoint11.

A native Cobalt diagnostic now reuses this exact URI/body before and after actual
UTC expiry. MyOS native HTTP source44 reports200 before expiry; browser reports0
because CORS hides that response. Do not infer expired/403 from browser status0.
Actual expiry12:51:09+07; test scheduled12:53:09+07,2026-10-10. Never modify URL,
signature or guest time. Native body SHA, peer status, error UI, N ordinary-watch
restart, moving video/native PCM and exact close/desktop resources remain to audit.
The original arbitrary expired GET returned0 and had a qmem desktop teardown
assertion; it was not expiry acceptance. The new harness opens desktop explicitly.

Optional live safe HTTP status instrumentation affects only the isolated diagnostic
n.json path, emits source IDs/status over serial, and does not change media data or
TLS. Snapshot source differences are recorded alongside harness/extractors. No
production architecture changes require a new full suite; checkpoint6 full regression
and checkpoint11 exact-provider focused/standard/public results retain their scope.
Do not close33.4,33.5,parent33 or Rickroll. Local commits only; owner pushes.

07:25+07 harness v2 supersedes l8dr2viu to require new shell prompt after ESC/q,
new desktop readiness/window markers and durable safe native reports. The old probe
was intentionally interrupted before expiry; its native-body SHA confirms original
1663-byte request identity. Current lz6fldt5 repeats same URI/body/deadline, native
source44/HTTP200 and native SHA bae9a411... confirmed again. No extra expiry wait.
All after-expiry/recovery/close acceptance flags remain false until actual audit.
