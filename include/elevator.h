#ifndef ELEVATOR_H
#define ELEVATOR_H

#include <types.h>

typedef enum { 
    MOTOR_DOWN = -1,
    MOTOR_STOP = 0,
    MOTOR_UP   = 1
} MotorState;

typedef enum {
    BUTTON_CALL_UP,
    BUTTON_CALL_DOWN,
    BUTTON_COMMAND
} ButtonType;

void elevator_hardware_init(const char* port, u8 floors);

void elevator_set_motor_state(MotorState state);
void elevator_set_button_lamp(ButtonType button_type, u8 floor, u8 value);
void elevator_set_floor_indicator(u8 floor);
void elevator_set_door_open_lamp(u8 value);
void elevator_set_stop_lamp(u8 value);

u8 elevator_get_button_signal(ButtonType button_type, u8 floor);
s8 elevator_get_floor_sensor_signal();
u8 elevator_get_stop_signal();
u8 elevator_get_obstruction_signal();

#endif