#include <network.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>

static int udp_socket;
static struct sockaddr_in send_address;

// This must be called before attempting to send or receive from the network.
u32 udp_init() {
    // Make a socket for both the sending and the reciving.
    udp_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_socket <= 0) {
        return 0;
    }

    // This enables broadcast for the socket.
    int settings = 1;
    int result = setsockopt(udp_socket, SOL_SOCKET, SO_BROADCAST, &settings, sizeof(settings));
    if (result) {
        return 0;
    }
    
    settings = 1;
    result = setsockopt(udp_socket, SOL_SOCKET, SO_REUSEPORT, &settings, sizeof(settings));
    if (result) {
        return 0;
    }

    // Set the timeout to 5 ms if the socket is read.
    struct timeval timeout;
    timeout.tv_sec  = 0;
    timeout.tv_usec = 1000;
    
    if (setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
        return 0;
    }

    // Configure the broadcast address. This is were we send packets.
    memset(&send_address, 0, sizeof send_address);
    send_address.sin_family = AF_INET;
    send_address.sin_port   = htons(9000);
    inet_pton(AF_INET, "127.255.255.255", &send_address.sin_addr);

    // Since we receive on the same socket, we must let the OS know that it should save 
    // ALL packets destinationed to this port, regardless of the IP address.
    struct sockaddr_in in_address;

    memset(&in_address, 0, sizeof in_address);
    in_address.sin_family      = AF_INET;
    in_address.sin_port        = htons(9000);
    in_address.sin_addr.s_addr = htonl(INADDR_ANY);

    result = bind(udp_socket, (struct sockaddr *)&in_address, sizeof in_address);
    if (result == -1) {
        return 0;
    }

    return 1;
}

u32 udp_send(void* buffer, u32 size) {
    int result = sendto(udp_socket, buffer, size, 0, 
        (struct sockaddr *)&send_address, sizeof send_address);

    if (result < 0) {
        return 0;
    }

    return 1;
}

u32 udp_receive(void* buffer, u32 size) {
    // For returning the senders IP address.
    struct sockaddr_in in_address;
    socklen_t len = sizeof(in_address);

    ssize_t bytes = recvfrom(udp_socket, buffer, size, 0, 
        (struct sockaddr *)&in_address, &len);

    if (bytes != size) {
        return 0;
    }

    return 1;
}
