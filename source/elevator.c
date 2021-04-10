#include <assert.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdio.h>
#include <pthread.h>
#include <elevator.h>

static int sockfd;
static pthread_mutex_t sockmtx;

static u8 FLOOR_COUNT = 0;  // Is filled in during init.
static u8 BUTTON_COUNT = 3;

void elevator_hardware_init(const char* port, u8 floor_count) {
    char ip[16] = "localhost";    

    pthread_mutex_init(&sockmtx, NULL);
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(sockfd != -1 && "Unable to set up socket");
    
    struct addrinfo hints = {
        .ai_family      = AF_INET, 
        .ai_socktype    = SOCK_STREAM, 
        .ai_protocol    = IPPROTO_TCP,
    };
    struct addrinfo* res;
    getaddrinfo(ip, port, &hints, &res);
    
    int fail = connect(sockfd, res->ai_addr, res->ai_addrlen);
    
    if (fail != 0) {
        printf("The server is not ready\n");
        exit(0);
    }

    FLOOR_COUNT = floor_count;
    
    freeaddrinfo(res);
    
    send(sockfd, (char[4]) {0}, 4, 0);
}

void elevator_set_motor_state(MotorState state) {
    pthread_mutex_lock(&sockmtx);
    send(sockfd, (char[4]) {1, state}, 4, 0);
    pthread_mutex_unlock(&sockmtx);
}

void elevator_set_button_lamp(ButtonType button_type, u8 floor, u8 value) {
    assert(floor >= 0);
    assert(floor < FLOOR_COUNT);
    assert(button_type >= 0);
    assert(button_type < BUTTON_COUNT);

    pthread_mutex_lock(&sockmtx);
    send(sockfd, (char[4]) {2, button_type, floor, value}, 4, 0);
    pthread_mutex_unlock(&sockmtx);
}

void elevator_set_floor_indicator(u8 floor) {
    assert(floor >= 0);
    assert(floor < FLOOR_COUNT);

    pthread_mutex_lock(&sockmtx);
    send(sockfd, (char[4]) {3, floor}, 4, 0);
    pthread_mutex_unlock(&sockmtx);
}

void elevator_set_door_open_lamp(u8 value) {
    pthread_mutex_lock(&sockmtx);
    send(sockfd, (char[4]) {4, value}, 4, 0);
    pthread_mutex_unlock(&sockmtx);
}

void elevator_set_stop_lamp(u8 value) {
    pthread_mutex_lock(&sockmtx);
    send(sockfd, (char[4]) {5, value}, 4, 0);
    pthread_mutex_unlock(&sockmtx);
}

u8 elevator_get_button_signal(ButtonType button_type, u8 floor) {
    pthread_mutex_lock(&sockmtx);
    send(sockfd, (char[4]) {6, button_type, floor}, 4, 0);
    char buf[4];
    recv(sockfd, buf, 4, 0);
    pthread_mutex_unlock(&sockmtx);
    return buf[1];
}

s8 elevator_get_floor_sensor_signal() {
    pthread_mutex_lock(&sockmtx);
    send(sockfd, (char[4]) {7}, 4, 0);
    char buf[4];
    recv(sockfd, buf, 4, 0);
    pthread_mutex_unlock(&sockmtx);
    return buf[1] ? buf[2] : -1;
}

u8 elevator_get_stop_signal() {
    pthread_mutex_lock(&sockmtx);
    send(sockfd, (char[4]) {8}, 4, 0);
    char buf[4];
    recv(sockfd, buf, 4, 0);
    pthread_mutex_unlock(&sockmtx);
    return buf[1];
}

u8 elevator_get_obstruction_signal() {
    pthread_mutex_lock(&sockmtx);
    send(sockfd, (char[4]) {9}, 4, 0);
    char buf[4];
    recv(sockfd, buf, 4, 0);
    pthread_mutex_unlock(&sockmtx);
    return buf[1];
}
