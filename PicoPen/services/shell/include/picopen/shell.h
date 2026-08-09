#ifndef PICOPEN_SHELL_H
#define PICOPEN_SHELL_H

#include <stdbool.h>
#include <stdint.h>

#include "picopen/keyboard.h"
#include "picopen/capability.h"
#include "picopen/device.h"
#include "picopen/engagement.h"
#include "picopen/sd.h"
#include "picopen/storage.h"

typedef struct picopen_shell_state {
    bool keyboard_ready;
    bool battery_ready;
    bool storage_ready;
    picopen_battery_info_t battery;
    picopen_sd_info_t sd;
    picopen_storage_listing_t storage;
    picopen_security_context_t security;
    picopen_device_manager_t devices;
    picopen_engagement_t engagement;
    bool ipc_ready;
} picopen_shell_state_t;

void picopen_shell_init(const picopen_shell_state_t *state);
void picopen_shell_update_state(const picopen_shell_state_t *state);
void picopen_shell_handle_key(uint8_t key);

#endif
