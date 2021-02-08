// UDP network interface

#ifndef UDP_H
#define UDP_H

#include <types.h>

struct udp_message {
// Full udp message
};

void udp_init();
void udp_send(const struct udp_message* m);
struct udp_message* udp_receive();

#endif
