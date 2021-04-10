#ifndef NETWORK_H
#define NETWORK_H

#include <types.h>

u32 udp_init();

u32 udp_send(void* buffer, u32 size);

u32 udp_receive(void* buffer, u32 size);

#endif
