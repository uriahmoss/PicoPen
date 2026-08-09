#ifndef PICOPEN_KEYBOARD_H
#define PICOPEN_KEYBOARD_H

#include <stdbool.h>
#include <stdint.h>

typedef enum picopen_key_state {
    PICOPEN_KEY_IDLE = 0,
    PICOPEN_KEY_PRESSED = 1,
    PICOPEN_KEY_HELD = 2,
    PICOPEN_KEY_RELEASED = 3,
} picopen_key_state_t;

typedef struct picopen_key_event {
    uint8_t key;
    picopen_key_state_t state;
} picopen_key_event_t;

typedef struct picopen_keyboard_info {
    uint32_t baud_hz;
    int write_result;
    int read_result;
    uint8_t response[2];
    uint8_t found_address;
    bool sda_high;
    bool scl_high;
} picopen_keyboard_info_t;

typedef struct picopen_battery_info {
    uint8_t percent;
    bool charging;
} picopen_battery_info_t;

bool picopen_keyboard_init(picopen_keyboard_info_t *info);
bool picopen_keyboard_poll(picopen_key_event_t *event);
bool picopen_keyboard_read_battery(picopen_battery_info_t *battery);

#endif
