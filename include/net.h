#ifndef MYOS_NET_H
#define MYOS_NET_H
#include "os.h"
bool net_up(void);
void net_status(void);
bool net_resolve(const char *name,uint8_t out[4]);
bool net_udp_echo(const uint8_t ip[4],uint16_t port,const char *message);
typedef bool (*net_http_sink)(void *context,const uint8_t *data,size_t length);
bool net_http_fetch(const uint8_t ip[4],uint16_t port,const char *host,const char *path,
                    net_http_sink sink,void *context);
bool net_http_get(const uint8_t ip[4],uint16_t port,const char *host,const char *path);
bool net_parse_ip(const char *text,uint8_t out[4]);
void net_poll(void);
#endif
