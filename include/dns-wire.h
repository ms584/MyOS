#ifndef MYOS_DNS_WIRE_H
#define MYOS_DNS_WIRE_H
#include <stddef.h>
#include <stdint.h>
#define DNS_QUERY_MAX 271u
#define DNS_REPLY_MAX 512u
#define DNS_ADDRESS_LIMIT 8u
struct dns_answer {
    uint32_t count;
    uint8_t addresses[DNS_ADDRESS_LIMIT][4];
    char canonical[254];
};
/* RFC1035 A/IN query and bounded response parser. Returns bytes or negative
 * pinned EAI error. Response output is untouched unless the full parse succeeds.
 * No transport, mutable state or allocation. TC returns EAI_AGAIN; its caller
 * must implement TCP fallback before claiming complete resolver behavior. */
int dns_make_query(const char *,uint16_t,uint8_t *,size_t);
int dns_parse_reply(const uint8_t *,size_t,const uint8_t *,size_t,struct dns_answer *);
#endif
