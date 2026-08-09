#include "picopen/keyboard.h"

#include <stddef.h>

#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"

#include "picopen/board_pins.h"

#define KEYBOARD_TIMEOUT_US    20000u
#define KEYBOARD_RESPONSE_MS   16u
#define KEYBOARD_REGISTER_VERSION 0x01u
#define KEYBOARD_REGISTER_FIFO    0x09u
#define KEYBOARD_REGISTER_BATTERY 0x0Bu
#define KEYBOARD_BATTERY_CHARGING 0x80u
#define KEYBOARD_BATTERY_PERCENT  0x7Fu
#define I2C_SCAN_FIRST_ADDRESS     0x08u
#define I2C_SCAN_LAST_ADDRESS      0x77u
#define I2C_SCAN_TIMEOUT_US        2000u
#define I2C_DIAGNOSTIC_BAUD_HZ     100000u

static i2c_inst_t *const keyboard_i2c = i2c1;
static bool initialized;
static picopen_keyboard_info_t *initialization_info;

static uint8_t scan_readable_address(void) {
    uint8_t value;
    for (uint8_t address = I2C_SCAN_FIRST_ADDRESS;
         address <= I2C_SCAN_LAST_ADDRESS; ++address) {
        const int result = i2c_read_timeout_us(
            keyboard_i2c, address, &value, 1u, false, I2C_SCAN_TIMEOUT_US);
        if (result == 1) {
            return address;
        }
    }
    return 0u;
}

static bool read_register(uint8_t address, uint8_t response[2]) {
    const int write_result = i2c_write_timeout_us(
        keyboard_i2c, PICOPEN_KEYBOARD_ADDRESS, &address, 1u, false,
        KEYBOARD_TIMEOUT_US);
    if (initialization_info != NULL) {
        initialization_info->write_result = write_result;
    }
    if (write_result != 1) {
        return false;
    }
    sleep_ms(KEYBOARD_RESPONSE_MS);
    const int read_result = i2c_read_timeout_us(
        keyboard_i2c, PICOPEN_KEYBOARD_ADDRESS, response, 2u, false,
        KEYBOARD_TIMEOUT_US);
    if (initialization_info != NULL) {
        initialization_info->read_result = read_result;
        initialization_info->response[0] = response[0];
        initialization_info->response[1] = response[1];
    }
    return read_result == 2;
}

bool picopen_keyboard_init(picopen_keyboard_info_t *info) {
    static const uint32_t probe_bauds[] = {10000u, 100000u, 400000u};
    if (info == NULL) {
        return false;
    }
    *info = (picopen_keyboard_info_t){0};
    initialization_info = info;
    gpio_set_function(PICOPEN_KEYBOARD_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICOPEN_KEYBOARD_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICOPEN_KEYBOARD_SDA_PIN);
    gpio_pull_up(PICOPEN_KEYBOARD_SCL_PIN);
    sleep_us(10u);
    info->sda_high = gpio_get(PICOPEN_KEYBOARD_SDA_PIN);
    info->scl_high = gpio_get(PICOPEN_KEYBOARD_SCL_PIN);
    for (size_t index = 0u;
         index < sizeof(probe_bauds) / sizeof(probe_bauds[0]); ++index) {
        info->baud_hz = i2c_init(keyboard_i2c, probe_bauds[index]);
        uint8_t response[2] = {0u, 0u};
        if (read_register(KEYBOARD_REGISTER_VERSION, response) &&
            (response[0] == 0u) && (response[1] != 0u)) {
            initialized = true;
            initialization_info = NULL;
            return true;
        }
        i2c_deinit(keyboard_i2c);
    }
    info->baud_hz = i2c_init(keyboard_i2c, I2C_DIAGNOSTIC_BAUD_HZ);
    info->found_address = scan_readable_address();
    i2c_deinit(keyboard_i2c);
    initialized = false;
    initialization_info = NULL;
    return false;
}

bool picopen_keyboard_poll(picopen_key_event_t *event) {
    if (!initialized || (event == NULL)) {
        return false;
    }
    uint8_t response[2] = {0u, 0u};
    if (!read_register(KEYBOARD_REGISTER_FIFO, response)) {
        return false;
    }
    if ((response[0] > PICOPEN_KEY_RELEASED) ||
        ((response[0] == PICOPEN_KEY_IDLE) && (response[1] == 0u))) {
        return false;
    }
    event->state = (picopen_key_state_t)response[0];
    event->key = response[1];
    return true;
}

bool picopen_keyboard_read_battery(picopen_battery_info_t *battery) {
    if (!initialized || (battery == NULL)) {
        return false;
    }
    uint8_t response[2] = {0u, 0u};
    if (!read_register(KEYBOARD_REGISTER_BATTERY, response) ||
        (response[0] != KEYBOARD_REGISTER_BATTERY)) {
        return false;
    }
    const uint8_t percent = response[1] & KEYBOARD_BATTERY_PERCENT;
    if (percent > 100u) {
        return false;
    }
    battery->percent = percent;
    battery->charging =
        (response[1] & KEYBOARD_BATTERY_CHARGING) != 0u;
    return true;
}
