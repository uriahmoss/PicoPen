#ifndef PICOPEN_WORKBENCH_H
#define PICOPEN_WORKBENCH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "picopen/device.h"

#define PICOPEN_WORKBENCH_ABI_VERSION 1u
#define PICOPEN_WORKBENCH_ITEM_CAPACITY 7u
#define PICOPEN_WORKBENCH_NAME_SIZE 12u

typedef enum picopen_workbench_job_state {
    PICOPEN_WORKBENCH_IDLE = 0,
    PICOPEN_WORKBENCH_RUNNING,
    PICOPEN_WORKBENCH_COMPLETE,
    PICOPEN_WORKBENCH_CANCELLED,
    PICOPEN_WORKBENCH_ERROR,
} picopen_workbench_job_state_t;

typedef enum picopen_workbench_item_state {
    PICOPEN_WORKBENCH_CLAIMED = 0,
    PICOPEN_WORKBENCH_READ_ONLY,
    PICOPEN_WORKBENCH_DISABLED_POLICY,
    PICOPEN_WORKBENCH_UNVERIFIED,
    PICOPEN_WORKBENCH_UNAVAILABLE,
} picopen_workbench_item_state_t;

typedef struct picopen_workbench_item {
    char name[PICOPEN_WORKBENCH_NAME_SIZE];
    picopen_workbench_item_state_t state;
} picopen_workbench_item_t;

typedef struct picopen_workbench_snapshot {
    uint16_t abi_version;
    uint32_t job_id;
    picopen_workbench_job_state_t state;
    uint8_t progress_percent;
    size_t item_count;
    picopen_workbench_item_t items[PICOPEN_WORKBENCH_ITEM_CAPACITY];
} picopen_workbench_snapshot_t;

void picopen_workbench_init(void);
bool picopen_workbench_start(const picopen_device_manager_t *devices,
                             uint64_t now_ms);
bool picopen_workbench_cancel(void);
bool picopen_workbench_poll(uint64_t now_ms);
void picopen_workbench_snapshot(picopen_workbench_snapshot_t *snapshot);
const char *picopen_workbench_job_state_name(
    picopen_workbench_job_state_t state);
const char *picopen_workbench_item_state_name(
    picopen_workbench_item_state_t state);

#endif
