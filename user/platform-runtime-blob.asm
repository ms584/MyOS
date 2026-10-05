bits 64
section .platform_runtime progbits alloc noexec write
global platform_runtime_start,platform_runtime_end
%ifndef RUNTIME_EXTERNAL
%define RUNTIME_EXTERNAL 0
%endif
platform_runtime_start:
%if RUNTIME_EXTERNAL = 0
incbin "build/platform-runtime.mrt"
%else
db 0
%endif
platform_runtime_end:
section .note.GNU-stack noalloc noexec nowrite progbits
