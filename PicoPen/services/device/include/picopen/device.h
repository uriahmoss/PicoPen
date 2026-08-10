#ifndef PICOPEN_DEVICE_H
#define PICOPEN_DEVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PICOPEN_DEVICE_CAPACITY 12u
#define PICOPEN_DEVICE_NAME_SIZE 16u

typedef enum picopen_device_state {
    PICOPEN_DEVICE_UNAVAILABLE = 0,
    PICOPEN_DEVICE_READY_READ_ONLY,
    PICOPEN_DEVICE_READY,
    PICOPEN_DEVICE_DISABLED_POLICY,
    PICOPEN_DEVICE_UNVERIFIED,
    PICOPEN_DEVICE_READY_LOCAL_ONLY,
} picopen_device_state_t;

typedef struct picopen_device_record {
    uint16_t identifier;
    char name[PICOPEN_DEVICE_NAME_SIZE];
    picopen_device_state_t state;
    bool external;
} picopen_device_record_t;

typedef struct picopen_device_manager {
    picopen_device_record_t records[PICOPEN_DEVICE_CAPACITY];
    size_t count;
} picopen_device_manager_t;

typedef enum picopen_device_identifier {
    PICOPEN_DEVICE_DISPLAY = 1u,
    PICOPEN_DEVICE_KEYBOARD,
    PICOPEN_DEVICE_SD,
    PICOPEN_DEVICE_FATFS,
    PICOPEN_DEVICE_BATTERY,
    PICOPEN_DEVICE_PSRAM,
    PICOPEN_DEVICE_WIFI,
    PICOPEN_DEVICE_BLE,
    PICOPEN_DEVICE_ATTACHMENTS,
} picopen_device_identifier_t;

void picopen_device_manager_init(picopen_device_manager_t *manager);
bool picopen_device_register(picopen_device_manager_t *manager,
                             uint16_t identifier, const char *name,
                             picopen_device_state_t state, bool external);
bool picopen_device_set_state(picopen_device_manager_t *manager,
                              uint16_t identifier,
                              picopen_device_state_t state);
const char *picopen_device_state_name(picopen_device_state_t state);

#endif
