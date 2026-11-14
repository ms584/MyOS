# Publication audit — 2026-11-14

## Scope and result

Baseline: b2f1269. Read-only scan of all 271 commits reachable from local main,
origin/main and origin/HEAD, including deleted files reachable in that history:
5,849 unique blobs, 402,309,002 bytes. Tracked working-tree files were scanned
separately. Remote refs were not fetched; unreachable objects, external forks,
GitHub attachments and files outside the tracked worktree are outside scope.

A local Python pattern scanner checked private-key headers, GitHub/OpenAI/AWS/
Google/Slack tokens, OAuth tokens, JWTs, bearer tokens, credentials in URLs,
signed URL query parameters and quoted/unquoted credential assignments.
No production credentials were identified by these checks. This is a bounded
pattern audit, not a guarantee that arbitrary or encoded secrets do not exist.
Gitleaks/TruffleHog were unavailable; no external scanning service received data.

## Findings and disposition

- security/certs/test-server.key and rsa-server.key: intentionally public TLS
  regression fixtures, explicitly loaded by local test servers. Retained.
  Added security/certs/README.md to prohibit production use and document scope.
- vendor/mbedtls/library/pk_internal.h: four PEM delimiter constants, not keys.
  Unchanged upstream source.
- Six header matches per scan scope; no other configured detector matches.
- Two distinct Git author/committer email addresses occur in local history.
  These are attribution metadata, not authentication secrets. Values are not
  reproduced here. No author metadata was rewritten.
- 2,478 tracked media files inventoried. Pillow inspected 2,447 images: no EXIF
  tags found; one JPEG had standard JFIF/progressive format metadata only.
  31 other media/non-image files were not image-metadata inspected.
- No currently tracked individual file exceeds 50 MiB.

## Important limits before public release

Visible text in screenshots, video frames and spoken audio was NOT OCR/transcribed
or exhaustively reviewed. Neither were historical image metadata or compressed
archive contents decoded. Personal names, hardware identifiers and local paths
may be intentionally present in technical evidence. Review the media inventory
and author metadata if those identifiers must remain private. Source publication
readiness here is conditional on accepting/reviewing these remaining privacy
limits; this report does not declare every asset privacy-cleared.

No history rewrite, deletion of evidence, credential revocation, push, GitHub
visibility change or binary release was performed. If a real credential is
subsequently identified, revoke/rotate it first and explicitly approve a scoped
history cleanup; deleting the current file alone does not remove past exposure.

## Local evidence (ignored, not published)

- build/publication-audit/findings.json: paths, object IDs, kinds, line numbers;
  no matched credential values.
- build/publication-audit/media-inventory.json
- build/publication-audit/image-metadata.json
- build/publication-audit-scan.py and companion inventory/metadata scripts
- build/activity.log: commands and redacted summaries

## Licensing and validation

Root LICENSE implements the owner's requested custom non-commercial terms for
original MyOS material only. Third-party licenses remain intact; see
PUBLICATION_LICENSE.md. Combined binary redistribution is not cleared merely
by adding this license. This change publishes no binaries.

Documentation/ignore changes only. git diff --check and local Markdown link
validation pass; representative ignore rules and test-fixture exceptions were
checked. Runtime tests were not rerun because executable code is unchanged.
