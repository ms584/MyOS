# NetSurf LibWapcaplet source provenance

Upstream: https://github.com/netsurf-browser/libwapcaplet
Revision: c7c128d3eb3223b216c974471f82e9337fbcf4ba
License: MIT; exact upstream text in COPYING.
Files copied unchanged: src/libwapcaplet.c and include/libwapcaplet/libwapcaplet.h.
MyOS compiles the source freestanding with NDEBUG and supplies memory/string
functions through user/app-runtime.c. This is a Phase 26 dependency port,
not a bundled full NetSurf browser.
