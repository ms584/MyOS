#ifndef MYOS_SYSCALL_H
#define MYOS_SYSCALL_H
#include "process.h"

/* MyOS x86-64 INT 0x80 ABI, independent of the Linux ABI.
 * RAX: call/result; RDI, RSI, RDX: arguments. Other registers preserved.
 * Errors are negative signed 64-bit values. */
enum { SYS_WRITE = 1, SYS_GETPID = 2, SYS_EXIT = 3, SYS_YIELD = 4, SYS_FOPEN = 5, SYS_FREAD = 6, SYS_FWRITE = 7,
       SYS_FCLOSE = 8, SYS_MKDIR = 9, SYS_UNLINK = 10, SYS_READDIR = 11, SYS_STAT = 12 };
#define SYS_WRITE_MAX 256
struct frame *syscall_dispatch(struct frame *frame);
void syscall_test(void);
#endif
