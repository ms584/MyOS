#ifndef MYOS_DNS_H
#define MYOS_DNS_H
#include "process.h"
#include "dns-call.h"
#define DNS_JOB_LIMIT 16u
/* Nonblocking, IRQ-disabled/single-CPU calls. Handles are process/thread-owned
 * and generation-checked. User pointers are validated by the syscall boundary;
 * no receive callback writes to an address in an unrelated CR3. */
int64_t dns_job_start(struct process *,uint64_t,uint64_t,const char *);
int dns_job_status(struct process *,uint64_t,uint32_t,struct dns_job_status *);
int dns_job_close(struct process *,uint64_t,uint32_t);
void dns_job_release_owner(struct process *,uint64_t);
bool dns_job_busy(void);
void dns_job_tick(void);
bool dns_job_receive(const uint8_t[4],uint16_t,uint16_t,const uint8_t *,size_t);
/* net.c DHCP DNS configuration and checked IPv4 UDP transmit primitive. */
bool net_dns_server(uint8_t[4]);
bool net_dns_packet(const uint8_t[4],const uint8_t[6],uint16_t,const uint8_t *,size_t);
#endif
