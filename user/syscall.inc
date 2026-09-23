%define SYS_WRITE 1
%define SYS_GETPID 2
%define SYS_EXIT 3
%define SYS_YIELD 4
; int 0x80: rax=number; rdi/rsi/rdx=arguments; rax=signed result.
; All other general registers and the interrupted flags are preserved.
