#ifndef MYOS_NIC_H
#define MYOS_NIC_H
#include "os.h"
#define NIC_FRAME_MAX 1514u
typedef void (*nic_receive_fn)(const uint8_t *,size_t);
bool nic_init(void);
bool nic_ready(void);
const uint8_t *nic_mac(void);
bool nic_send(const void *,size_t);
unsigned nic_poll(nic_receive_fn,unsigned budget);
#endif
