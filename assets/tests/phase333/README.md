# Phase33.3 standard test asset
Original moving bars/frame identity cells and generated sine PCM,40seconds,
160x90 VP8@15fps + mono48kHz Opus in standard WebM.
No third-party audiovisual content was used.
The host encoder is used ONLY to create this frozen input. All acceptance
fetch/demux/decode/render/audio operations execute in MyOS.
tools/generate_media_fixture.py records encoding provenance; it never decodes.
Guest decoder pins/licenses are in docs/PHASE333_MEDIA_LOCK.json and licenses/phase333/.
SHA256 c04a3650cfbfd72ef82020e2828ef96e2432b8a81ed1ab26b376648b7047dbe2.
