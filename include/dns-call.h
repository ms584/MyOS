#ifndef MYOS_DNS_CALL_H
#define MYOS_DNS_CALL_H
#include "dns-wire.h"
#define MYOS_DNS_CALL 52u
enum {DNS_CALL_START=0,DNS_CALL_STATUS=1,DNS_CALL_CLOSE=2,DNS_CALL_CONFIGURED=3};
struct dns_start_request {uint64_t sequence;char name[255];uint8_t reserved;};
struct dns_job_status {int32_t error;struct dns_answer answer;};
_Static_assert(sizeof(struct dns_start_request)==264,"private resolver start ABI");
_Static_assert(sizeof(struct dns_job_status)==296,"private resolver status ABI");
#endif
