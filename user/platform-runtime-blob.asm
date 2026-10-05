bits 64
section .platform_runtime progbits alloc noexec write
global platform_runtime_start,platform_runtime_end
platform_runtime_start: incbin "build/platform-runtime.mrt"
platform_runtime_end:
section .note.GNU-stack noalloc noexec nowrite progbits
