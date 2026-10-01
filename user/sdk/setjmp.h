#ifndef MYOS_SDK_SETJMP_H
#define MYOS_SDK_SETJMP_H
/* x86_64 musl storage; this SDK does not expose signal-saving variants. */
#if __SIZEOF_LONG__ != 8 || __SIZEOF_POINTER__ != 8
#error MyOS jump SDK requires x86_64 LP64
#endif
typedef struct __jmp_buf_tag {
    unsigned long __jb[8];
    unsigned long __fl;
    unsigned long __ss[16];
} jmp_buf[1];
#ifdef __cplusplus
extern "C" {
#endif
int setjmp(jmp_buf) __attribute__((returns_twice));
int _setjmp(jmp_buf) __attribute__((returns_twice));
void longjmp(jmp_buf,int) __attribute__((noreturn));
void _longjmp(jmp_buf,int) __attribute__((noreturn));
#define setjmp setjmp
#ifdef __cplusplus
}
#endif
#endif
