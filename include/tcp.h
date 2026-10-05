#ifndef MYOS_TCP_H
#define MYOS_TCP_H
#include "process.h"
#include "tcp-server-call.h"

/* Nonblocking kernel transport. Handles include a generation and are private
 * to their creating process. All calls run with IRQs disabled (one CPU). */
#define TCP_STREAM_LIMIT 32u
#define TCP_RX_CAPACITY 8192u
#define TCP_TX_CAPACITY 4096u
struct tcp_address { uint8_t ip[4]; uint16_t port,reserved; };
struct tcp_status {
    uint32_t ready,available,error,state;
    struct tcp_address local,peer;
};
int64_t tcp_stream_create(struct process *,uint64_t,uint64_t);
int tcp_stream_connect(struct process *,uint32_t,const struct tcp_address *);
int tcp_stream_bind(struct process *,uint32_t,const struct tcp_address *);
int tcp_stream_listen(struct process *,uint32_t,unsigned);
int64_t tcp_stream_accept(struct process *,uint32_t,struct tcp_accept_request *);
/* 0: reported error; 1: consume reported error; 2: persistent failure. */
int tcp_stream_status(struct process *,uint32_t,struct tcp_status *,unsigned);
int tcp_stream_send(struct process *,uint32_t,const uint8_t *,size_t);
int tcp_stream_recv(struct process *,uint32_t,uint8_t *,size_t,bool);
int tcp_stream_shutdown(struct process *,uint32_t,unsigned);
int tcp_stream_close(struct process *,uint32_t);
void tcp_stream_release_owner(struct process *);
bool tcp_stream_busy(void);
bool tcp_stream_port_used(uint16_t);
/* net.c verifies IPv4/TCP checksums and headers before dispatching here. */
bool tcp_stream_receive(const uint8_t[4],const uint8_t *,size_t);
void tcp_stream_tick(void);
#endif
