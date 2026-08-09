#include "picopen/workbench.h"

#include <string.h>

#define PICOPEN_WORKBENCH_STEP_MS 120u

static picopen_workbench_snapshot_t current;
static picopen_device_manager_t source_devices;
static uint64_t next_step_ms;
static uint32_t next_job_id;

static picopen_device_state_t device_state(uint16_t identifier) {
    for (size_t index = 0u; index < source_devices.count; ++index) {
        if (source_devices.records[index].identifier == identifier) {
            return source_devices.records[index].state;
        }
    }
    return PICOPEN_DEVICE_UNAVAILABLE;
}

static picopen_workbench_item_state_t claimed_state(uint16_t identifier,
                                                     bool read_only) {
    const picopen_device_state_t state = device_state(identifier);
    if (state == PICOPEN_DEVICE_UNAVAILABLE) {
        return PICOPEN_WORKBENCH_UNAVAILABLE;
    }
    if (state == PICOPEN_DEVICE_DISABLED_POLICY) {
        return PICOPEN_WORKBENCH_DISABLED_POLICY;
    }
    return read_only ? PICOPEN_WORKBENCH_READ_ONLY
                     : PICOPEN_WORKBENCH_CLAIMED;
}

static void add_item(const char *name, picopen_workbench_item_state_t state) {
    if ((name == NULL) ||
        (current.item_count >= PICOPEN_WORKBENCH_ITEM_CAPACITY)) {
        current.state = PICOPEN_WORKBENCH_ERROR;
        return;
    }
    picopen_workbench_item_t *const item =
        &current.items[current.item_count++];
    strncpy(item->name, name, sizeof(item->name) - 1u);
    item->name[sizeof(item->name) - 1u] = '\0';
    item->state = state;
}

void picopen_workbench_init(void) {
    current = (picopen_workbench_snapshot_t){
        .abi_version = PICOPEN_WORKBENCH_ABI_VERSION,
        .state = PICOPEN_WORKBENCH_IDLE,
    };
    source_devices = (picopen_device_manager_t){0};
    next_step_ms = 0u;
    next_job_id = 1u;
}

bool picopen_workbench_start(const picopen_device_manager_t *devices,
                             uint64_t now_ms) {
    if ((devices == NULL) ||
        (current.state == PICOPEN_WORKBENCH_RUNNING)) {
        return false;
    }
    source_devices = *devices;
    current = (picopen_workbench_snapshot_t){
        .abi_version = PICOPEN_WORKBENCH_ABI_VERSION,
        .job_id = next_job_id++,
        .state = PICOPEN_WORKBENCH_RUNNING,
    };
    next_step_ms = now_ms;
    return true;
}

bool picopen_workbench_cancel(void) {
    if (current.state != PICOPEN_WORKBENCH_RUNNING) {
        return false;
    }
    current.state = PICOPEN_WORKBENCH_CANCELLED;
    return true;
}

bool picopen_workbench_poll(uint64_t now_ms) {
    if ((current.state != PICOPEN_WORKBENCH_RUNNING) ||
        (now_ms < next_step_ms)) {
        return false;
    }
    switch (current.item_count) {
        case 0u:
            add_item("I2C1 KBD", claimed_state(PICOPEN_DEVICE_KEYBOARD, false));
            break;
        case 1u:
            add_item("SPI0 SD", claimed_state(PICOPEN_DEVICE_SD, true));
            break;
        case 2u:
            add_item("SPI1 LCD", claimed_state(PICOPEN_DEVICE_DISPLAY, false));
            break;
        case 3u:
            add_item("GPIO EXP", PICOPEN_WORKBENCH_DISABLED_POLICY);
            break;
        case 4u:
            add_item("ADC EXP", PICOPEN_WORKBENCH_UNVERIFIED);
            break;
        case 5u:
            add_item("UART EXP", PICOPEN_WORKBENCH_UNVERIFIED);
            break;
        case 6u:
            add_item("ATTACHMENTS",
                     claimed_state(PICOPEN_DEVICE_ATTACHMENTS, false));
            break;
        default:
            current.state = PICOPEN_WORKBENCH_COMPLETE;
            current.progress_percent = 100u;
            return true;
    }
    if (current.state == PICOPEN_WORKBENCH_ERROR) {
        return true;
    }
    current.progress_percent = (uint8_t)
        ((current.item_count * 100u) / PICOPEN_WORKBENCH_ITEM_CAPACITY);
    next_step_ms = now_ms + PICOPEN_WORKBENCH_STEP_MS;
    return true;
}

void picopen_workbench_snapshot(picopen_workbench_snapshot_t *snapshot) {
    if (snapshot != NULL) {
        *snapshot = current;
    }
}

const char *picopen_workbench_job_state_name(
    picopen_workbench_job_state_t state) {
    static const char *const names[] = {
        "IDLE", "RUNNING", "COMPLETE", "CANCELLED", "ERROR",
    };
    return (unsigned int)state < sizeof(names) / sizeof(names[0])
        ? names[state] : "UNKNOWN";
}

const char *picopen_workbench_item_state_name(
    picopen_workbench_item_state_t state) {
    static const char *const names[] = {
        "CLAIMED", "READ-ONLY", "DISABLED", "UNVERIFIED", "DOWN",
    };
    return (unsigned int)state < sizeof(names) / sizeof(names[0])
        ? names[state] : "UNKNOWN";
}
