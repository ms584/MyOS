#ifndef MYOS_SDK_NETDB_H
#define MYOS_SDK_NETDB_H
#include <stddef.h>
#include <stdint.h>
/* Selected LP64 musl-1.2.6 netdb declarations, not a complete POSIX header.
 * Source archive/hash and supported resolver semantics must be recorded. */
typedef unsigned int socklen_t;
struct sockaddr;
struct addrinfo {
    int ai_flags,ai_family,ai_socktype,ai_protocol;
    socklen_t ai_addrlen;
    struct sockaddr *ai_addr;
    char *ai_canonname;
    struct addrinfo *ai_next;
};
#define AI_PASSIVE 1
#define AI_CANONNAME 2
#define AI_NUMERICHOST 4
#define AI_V4MAPPED 8
#define AI_ALL 16
#define AI_ADDRCONFIG 32
#define AI_NUMERICSERV 0x400
#define EAI_BADFLAGS -1
#define EAI_NONAME -2
#define EAI_AGAIN -3
#define EAI_FAIL -4
#define EAI_NODATA -5
#define EAI_FAMILY -6
#define EAI_SOCKTYPE -7
#define EAI_SERVICE -8
#define EAI_MEMORY -10
#define EAI_SYSTEM -11
#define EAI_OVERFLOW -12
#ifdef __cplusplus
extern "C" {
#endif
int getaddrinfo(const char *,const char *,const struct addrinfo *,struct addrinfo **);
void freeaddrinfo(struct addrinfo *);
const char *gai_strerror(int);
#ifdef __cplusplus
}
static_assert(sizeof(struct addrinfo)==48,"pinned netdb ABI");
static_assert(offsetof(struct addrinfo,ai_addr)==24,"pinned sockaddr pointer");
#else
_Static_assert(sizeof(struct addrinfo)==48,"pinned netdb ABI");
_Static_assert(offsetof(struct addrinfo,ai_addr)==24,"pinned sockaddr pointer");
#endif
#endif
