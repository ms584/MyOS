#ifndef MYOS_SDK_ERRNO_H
#define MYOS_SDK_ERRNO_H
#ifdef __cplusplus
extern "C" {
#endif
int *__errno_location(void);
#ifdef __cplusplus
}
#endif
#define errno (*__errno_location())
#define EINTR 4
#define EBADF 9
#define EAGAIN 11
#define ENOMEM 12
#define EBUSY 16
#define EINVAL 22
#define ERANGE 34
#define EOVERFLOW 75
#define EILSEQ 84
#define ETIMEDOUT 110
#endif
