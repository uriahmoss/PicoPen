#ifndef PICOPEN_HOSTS_H
#define PICOPEN_HOSTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "picopen/recon.h"
#include "picopen/wifi.h"

#define PICOPEN_HOST_CAPACITY 16u
#define PICOPEN_HOST_NAME_SIZE 32u
#define PICOPEN_HOST_SERVICE_CAPACITY 6u

typedef enum picopen_host_source {
    PICOPEN_HOST_LOCAL = 0,
    PICOPEN_HOST_GATEWAY,
    PICOPEN_HOST_DNS,
    PICOPEN_HOST_TASK,
} picopen_host_source_t;

typedef struct picopen_host_record {
    char address[PICOPEN_RECON_ADDRESS_SIZE];
    char hostname[PICOPEN_HOST_NAME_SIZE];
    uint16_t services[PICOPEN_HOST_SERVICE_CAPACITY];
    size_t service_count;
    uint64_t last_seen_ms;
    picopen_host_source_t source;
    bool reachable;
} picopen_host_record_t;

typedef struct picopen_host_inventory {
    picopen_host_record_t records[PICOPEN_HOST_CAPACITY];
    size_t count;
    bool truncated;
    uint32_t generation;
} picopen_host_inventory_t;

void picopen_hosts_init(void);
void picopen_hosts_observe_wifi(const picopen_wifi_status_t *wifi,
                                uint64_t now_ms);
void picopen_hosts_observe_recon(const picopen_recon_snapshot_t *result,
                                 uint64_t now_ms);
void picopen_hosts_snapshot(picopen_host_inventory_t *inventory);
const char *picopen_host_source_name(picopen_host_source_t source);

#endif
