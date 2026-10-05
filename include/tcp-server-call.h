#ifndef MYOS_TCP_SERVER_CALL_H
#define MYOS_TCP_SERVER_CALL_H
/* Additive MyOS ABI. Caller-owned notification words; no kernel pointer. */
struct tcp_accept_request {
    uint64_t object_sequence,registry_sequence;
    uint8_t peer_ip[4];
    uint16_t peer_port,reserved;
};
_Static_assert(sizeof(struct tcp_accept_request)==24,"TCP accept request ABI");
#endif
