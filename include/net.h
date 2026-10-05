#ifndef MYOS_NET_H
#define MYOS_NET_H
#include "os.h"
#include "system-call.h"
void net_interface_snapshot(struct system_interface_snapshot *out);
bool net_up(void);
bool net_configured(void);
void net_status(void);
bool net_resolve(const char *name,uint8_t out[4]);
bool net_udp_echo(const uint8_t ip[4],uint16_t port,const char *message);
typedef bool (*net_http_sink)(void *context,const uint8_t *data,size_t length);
bool net_http_fetch(const uint8_t ip[4],uint16_t port,const char *host,const char *path,
                    net_http_sink sink,void *context);
bool net_http_get(const uint8_t ip[4],uint16_t port,const char *host,const char *path);
bool net_parse_ip(const char *text,uint8_t out[4]);
void net_poll(void);
/* One serialized TCP stream for the native TLS client. */
bool net_tcp_open(const uint8_t ip[4],uint16_t port);
int net_tcp_write(const uint8_t *data,size_t size);
int net_tcp_read(uint8_t *data,size_t capacity);
void net_tcp_close(void);
/* Nonblocking primitives used by the independently owned TCP stream pool. */
int net_stream_route(const uint8_t ip[4],uint8_t mac[6]);
bool net_stream_packet(const uint8_t ip[4],const uint8_t mac[6],uint8_t *tcp,size_t length);
void net_stream_address(uint8_t ip[4]);
#endif
