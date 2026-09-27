#ifndef MYOS_TLS_H
#define MYOS_TLS_H
#include "net.h"
bool tls_http_fetch(const uint8_t ip[4],uint16_t port,const char *host,const char *path,
                    net_http_sink sink,void *context);
bool tls_response_too_large(void);
#endif
