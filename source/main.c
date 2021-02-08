// Elevator project

#include <types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> 
#include <stdlib.h> 

static u8 buffer[1024];

void server_init() {
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    // Zero the address
    memset(&server_addr, 0x00, sizeof(struct sockaddr_in));

    // Configure the address
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(5000);
    server_addr.sin_family = AF_INET;

    // Make a new socket
    int s = socket(AF_INET, SOCK_DGRAM, 0);

    // Bind the port
    bind(s, (struct sockaddr *)&server_addr, sizeof(server_addr));

    while (1) {
        int len = sizeof(client_addr);
        int n = recvfrom(s, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &len);

        buffer[n] = 0;
        printf("Message: %s\n", buffer);
    }

    close(s);
}

void client_init() {
    struct sockaddr_in server_addr;

    // Fill the address with zero
    memset(&server_addr, 0x00, sizeof(server_addr));

    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    server_addr.sin_port = htons(5000); 
    server_addr.sin_family = AF_INET; 

    // Create UDP socket 
    int s = socket(AF_INET, SOCK_DGRAM, 0); 

    // Connect
    if(connect(s, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("\n Error : Connect Failed \n"); 
    } 
  
    // Send UDP
    while (1) {
        sendto(s, "HELLO", 1000, 0, (struct sockaddr*)NULL, sizeof(server_addr));
        usleep(500000);
    }

    close(s);
}

int main(int arg_count, char** args) {

    for (u32 i = 0; i < arg_count; i++) {
        printf("Arg => %s\n", args[i]);
    }

    if (arg_count != 2) {
        return 0;
    }

    // Simple test
    if (args[1][0] == 's') {
        server_init();
    } else {
        client_init();
    }



    // Main loop
    while (1) {
        // Les knapper
        // Les UDP message in
        // Les motor

        // Process
    }

    return 0;
}