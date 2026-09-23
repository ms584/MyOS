%define SYS_WRITE 1
%define SYS_GETPID 2
%define SYS_EXIT 3
%define SYS_YIELD 4
; int 0x80: rax=number; rdi/rsi/rdx=arguments; rax=signed result.
; All other general registers and the interrupted flags are preserved.

%define SYS_FOPEN 5
%define SYS_FREAD 6
%define SYS_FWRITE 7
%define SYS_FCLOSE 8
%define SYS_MKDIR 9
%define SYS_UNLINK 10
%define SYS_READDIR 11
%define SYS_STAT 12
