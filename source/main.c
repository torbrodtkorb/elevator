#include <types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <elevator.h>
#include <network.h>
#include <sys/time.h>
#include <assert.h>
#include <time.h>

static char BACKUP_FILE_NAME[] = "build/backup_port_xxxx";

static const u64 DOOR_OPEN_TIME       = 3000000;
static const u64 TIME_PER_FLOOR_US    = 3000000;
static const u64 TIMEOUT_PER_FLOOR_US = 20000;
static const u8  ENABLE_FILE_BACKUP   = 1;

static const s8 NO_FLOOR = -1;

typedef enum {
    MESSAGE_NEW_ORDER,
    MESSAGE_ORDER_TAKEN,
    MESSAGE_ORDER_COMPLETE,
} MessageType;

typedef enum {
    DIRECTION_UP,
    DIRECTION_DOWN
} Direction;

typedef enum {
    ELEVATOR_CLOSED,
    ELEVATOR_OPEN,
    ELEVATOR_MOVING
} ElevatorState;

typedef enum {
    FLOOR_IDLE,
    FLOOR_SHOULD_BE_TAKEN,
    FLOOR_PENDING,
    FLOOR_EXTERNAL_PENDING
} FloorState;

typedef struct {
    MessageType type;
    Direction direction;
    u8  floor;
    u64 estimated_completion_time;
} Message;

typedef struct {
    FloorState state;
    union {
        u64 internal_timer;
        u64 external_timer;
    };
} Floor;

typedef struct {
    ElevatorState state;
    Floor* floors;
    u8 floor_count;
    u8 current_floor;
    s8 next_floor;
    u64 door_timer;
} Elevator;

static Elevator elevator;
static Time start;
static File* file;

static void reset_timer() {
    gettimeofday(&start, 0);
}

static u64 get_runtime() {
    Time stop;
    gettimeofday(&stop, 0);
    return (stop.tv_sec - start.tv_sec) * 1000000 + (stop.tv_usec - start.tv_usec);
}

static u32 send_message(Message* message) {
    return udp_send(message, sizeof(Message));
}

static u32 recive_message(Message* message) {
    return udp_receive(message, sizeof(Message));
}

static void backup_data() {
    if (ENABLE_FILE_BACKUP) {
        file = fopen(BACKUP_FILE_NAME, "w");
        if (file) {
            fwrite(elevator.floors, sizeof(Floor), elevator.floor_count, file);
            fclose(file);
        }
    }
}

static void read_backup() {
    if (ENABLE_FILE_BACKUP) {
        file = fopen(BACKUP_FILE_NAME, "r");
        if (file) {
            u32 size = fread(elevator.floors, sizeof(Floor), elevator.floor_count, file);
            assert(size == elevator.floor_count);
            fclose(file);

            // If any orders are active; either pending, or has some running timer, we
            // ignore the state and the timer value, and just take the order.
            for (u32 i = 0; i < elevator.floor_count; i++) {
                if (elevator.floors[i].state != FLOOR_IDLE)
                    elevator.floors[i].state = FLOOR_PENDING;
            }
        }
    }
}

static u8 estimate_floors_before_order(u8 destination_floor) {
    s8 diff = elevator.current_floor - destination_floor;
    return (diff < 0) ? -diff : diff;
}

static u8 get_number_of_pending_orders() {
    u8 count = 0;
    for (u32 i = 0; i < elevator.floor_count; i++) {
        if (elevator.floors[i].state == FLOOR_PENDING)
            count++;
    }
    return count;
}

static u64 get_time_to_complete_order(u8 destination_floor) {
    return estimate_floors_before_order(destination_floor) * TIME_PER_FLOOR_US;
}

static u64 get_time_before_we_take_the_order(Direction direction, u8 destination_floor) {
    u64 time = estimate_floors_before_order(destination_floor) * TIMEOUT_PER_FLOOR_US;

    time += get_number_of_pending_orders() * 100000;
    time += (rand() % 10) * 1000;
    time += (elevator.state == ELEVATOR_OPEN) ? 100000 : 0;

    return time;
}

static void add_new_order(Direction direction, u8 floor_number) {
    Floor* floor = &elevator.floors[floor_number];

    if (floor->state != FLOOR_IDLE)
        return;

    floor->internal_timer = get_time_before_we_take_the_order(direction, floor_number);
    floor->state = FLOOR_SHOULD_BE_TAKEN;

    backup_data();
}

static void handle_incoming_message() {
    Message message;
    if (!recive_message(&message))
        return;

    switch (message.type) {
        case MESSAGE_NEW_ORDER : {
            add_new_order(message.direction, message.floor);
            return;
        }
        case MESSAGE_ORDER_TAKEN : {
            Floor* floor = &elevator.floors[message.floor];
            if (floor->state != FLOOR_PENDING) {
                floor->external_timer = message.estimated_completion_time;
                floor->state          = FLOOR_EXTERNAL_PENDING;
            }
            return;
        }
        case MESSAGE_ORDER_COMPLETE : {
            elevator.floors[message.floor].state = FLOOR_IDLE;
            return;
        }
    }
}

static void handle_external_button_event(Direction direction, u8 destination_floor) {
    Floor* floor = &elevator.floors[destination_floor];

    if (floor->state != FLOOR_IDLE)
        return;

    Message new_order = {
        .type      = MESSAGE_NEW_ORDER,
        .direction = direction,
        .floor     = destination_floor,
    };

    send_message(&new_order);
    add_new_order(direction, destination_floor);
}

static void handle_internal_button_event(u8 floor_number) {
    elevator.floors[floor_number].state = FLOOR_PENDING;
    elevator_set_button_lamp(BUTTON_COMMAND, floor_number, 1);
}

static void handle_buttons() {
    for (u32 i = 0; i < elevator.floor_count; i++) {
        if (elevator_get_button_signal(BUTTON_CALL_UP, i))
            handle_external_button_event(DIRECTION_UP, i);

        if (elevator_get_button_signal(BUTTON_CALL_DOWN, i))
            handle_external_button_event(DIRECTION_DOWN, i);

        if (elevator_get_button_signal(BUTTON_COMMAND, i))
            handle_internal_button_event(i);
    }
}

static void process_expired_timers() {
    for (u32 i = 0; i < elevator.floor_count; i++) {
        Floor* floor = &elevator.floors[i];

        if (floor->internal_timer == 0 && floor->state == FLOOR_SHOULD_BE_TAKEN) {
            Message order_taken = {
                .type    = MESSAGE_ORDER_TAKEN, 
                .estimated_completion_time = get_time_to_complete_order(i),
                .floor   = i
            };

            send_message(&order_taken);
            floor->state = FLOOR_PENDING;
        }

        if (floor->external_timer == 0 && floor->state == FLOOR_EXTERNAL_PENDING) {
            floor->state = FLOOR_IDLE;
            add_new_order(DIRECTION_DOWN, i); // Direction does not matter here.
        }
    }

    if (elevator.door_timer == 0 && elevator.state == ELEVATOR_OPEN) {
        elevator_set_door_open_lamp(0);
        elevator.state = ELEVATOR_CLOSED;
        elevator.floors[elevator.current_floor].state = FLOOR_IDLE;
        backup_data();
    }
}

static void update_next_order() {
    u8 index = elevator.current_floor;
    for (u32 i = 0; i < elevator.floor_count; i++) {
        if (elevator.floors[index].state == FLOOR_PENDING)
            elevator.next_floor = index;

        if (++index >= elevator.floor_count)
            index = 0;
    }
}

static void move_elevator() {
    if (elevator.state == ELEVATOR_MOVING) {
        s8 floor = elevator_get_floor_sensor_signal();
        if (floor != NO_FLOOR)
            elevator.current_floor = floor;
    }

    if (elevator.state != ELEVATOR_OPEN && elevator.next_floor == elevator.current_floor) {
        elevator_set_door_open_lamp(1);
        elevator_set_motor_state(MOTOR_STOP);
        elevator_set_button_lamp(BUTTON_CALL_DOWN, elevator.current_floor, 0);
        elevator_set_button_lamp(BUTTON_CALL_UP, elevator.current_floor, 0);
        elevator_set_button_lamp(BUTTON_COMMAND, elevator.current_floor, 0);

        elevator.door_timer = DOOR_OPEN_TIME;
        elevator.state      = ELEVATOR_OPEN;
        elevator.next_floor = NO_FLOOR;

        Message order_complete = {
            .floor = elevator.current_floor,
            .type  = MESSAGE_ORDER_COMPLETE
        };

        send_message(&order_complete);
    }

    if (elevator.state == ELEVATOR_CLOSED) {
        if (elevator.next_floor == NO_FLOOR)
            update_next_order();

        if (elevator.next_floor != NO_FLOOR) {
            elevator.state = ELEVATOR_MOVING;
            if (elevator.next_floor < elevator.current_floor)
                elevator_set_motor_state(MOTOR_DOWN);
            else
                elevator_set_motor_state(MOTOR_UP);
        }
    }
}

static void decrement_one_timer(u64* timer, u64 decrement) {
    *timer = (*timer >= decrement) ? *timer - decrement : 0;
}

static void decrement_timers(u64 time_us) {
    decrement_one_timer(&elevator.door_timer, time_us);
    for (u32 i = 0; i < elevator.floor_count; i++)
        decrement_one_timer(&elevator.floors[i].internal_timer, time_us);
}

static void elevator_init(char* server_port, char* floor_count) {
    elevator.floor_count = atoi(floor_count);
    elevator.floors      = calloc(sizeof(Floor), elevator.floor_count);

    elevator_hardware_init(server_port, elevator.floor_count);
    assert(udp_init());
    srand(time(0));

    // Start in a known state. If we are between floors, we go down.
    s8 current_floor = elevator_get_floor_sensor_signal();
    if (current_floor == NO_FLOOR) {
        elevator_set_motor_state(MOTOR_DOWN);
        while (current_floor == NO_FLOOR)
            current_floor = elevator_get_floor_sensor_signal();
    }

    elevator_set_motor_state(MOTOR_STOP);

    elevator.current_floor = (u8)current_floor;
    elevator.next_floor    = NO_FLOOR;
    elevator.state         = ELEVATOR_CLOSED;

    // Add the server port to the file name to make it unique.
    char* destination = BACKUP_FILE_NAME + sizeof(BACKUP_FILE_NAME) - 4 - 1;
    while (*server_port)
        *destination++ = *server_port++;

    read_backup();
}

int main(int argument_count, char** arguments) {
    assert(argument_count == 3);
    elevator_init(arguments[1], arguments[2]);

    while (1) {
        reset_timer();
        handle_buttons();
        handle_incoming_message();
        move_elevator();
        process_expired_timers();
        decrement_timers(get_runtime());
    }
}