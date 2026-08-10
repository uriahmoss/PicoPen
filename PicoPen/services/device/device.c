#include "picopen/device.h"

#include <stddef.h>
#include <string.h>

void picopen_device_manager_init(picopen_device_manager_t *manager) {
    if (manager != NULL) {
        *manager = (picopen_device_manager_t){0};
    }
}

bool picopen_device_register(picopen_device_manager_t *manager,
                             uint16_t identifier, const char *name,
                             picopen_device_state_t state, bool external) {
    if ((manager == NULL) || (identifier == 0u) || (name == NULL) ||
        (name[0] == '\0') || (manager->count >= PICOPEN_DEVICE_CAPACITY)) {
        return false;
    }
    for (size_t index = 0u; index < manager->count; ++index) {
        if (manager->records[index].identifier == identifier) {
            return false;
        }
    }
    picopen_device_record_t *const record = &manager->records[manager->count++];
    record->identifier = identifier;
    strncpy(record->name, name, sizeof(record->name) - 1u);
    record->name[sizeof(record->name) - 1u] = '\0';
    record->state = state;
    record->external = external;
    return true;
}

bool picopen_device_set_state(picopen_device_manager_t *manager,
                              uint16_t identifier,
                              picopen_device_state_t state) {
    if ((manager == NULL) || (identifier == 0u)) {
        return false;
    }
    for (size_t index = 0u; index < manager->count; ++index) {
        if (manager->records[index].identifier == identifier) {
            manager->records[index].state = state;
            return true;
        }
    }
    return false;
}

const char *picopen_device_state_name(picopen_device_state_t state) {
    static const char *const names[] = {
        "DOWN", "READY-RO", "READY", "DISABLED", "UNVERIFIED", "READY-LOCAL",
    };
    if ((unsigned int)state >= sizeof(names) / sizeof(names[0])) {
        return "UNKNOWN";
    }
    return names[state];
}
