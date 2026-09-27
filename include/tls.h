#ifndef MYOS_TLS_H
#define MYOS_TLS_H
#include "net.h"
bool tls_http_fetch(const uint8_t ip[4],uint16_t port,const char *host,const char *path,
                    net_http_sink sink,void *context);
/* Deliver verified response chunks without retaining the whole response. */
bool tls_http_fetch_limit(const uint8_t ip[4],uint16_t port,const char *host,const char *path,
                          size_t max_bytes,net_http_sink sink,void *context);
bool tls_response_too_large(void);
#endif
