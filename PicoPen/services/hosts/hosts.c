#include "picopen/hosts.h"

#include <string.h>

static picopen_host_inventory_t inventory;

static picopen_host_record_t *find_or_add(const char *address,
                                          picopen_host_source_t source) {
    if (!address || !address[0] || strcmp(address, "0.0.0.0") == 0) return NULL;
    for (size_t index = 0u; index < inventory.count; ++index) {
        if (strcmp(inventory.records[index].address, address) == 0)
            return &inventory.records[index];
    }
    if (inventory.count >= PICOPEN_HOST_CAPACITY) {
        inventory.truncated = true;
        return NULL;
    }
    picopen_host_record_t *record = &inventory.records[inventory.count++];
    *record = (picopen_host_record_t){.source = source};
    strncpy(record->address, address, sizeof(record->address) - 1u);
    ++inventory.generation;
    return record;
}

static void observe(const char *address, picopen_host_source_t source,
                    uint64_t now_ms, bool reachable) {
    picopen_host_record_t *record = find_or_add(address, source);
    if (!record) return;
    record->last_seen_ms = now_ms;
    record->reachable |= reachable;
}

void picopen_hosts_init(void) {
    inventory = (picopen_host_inventory_t){0};
}

void picopen_hosts_observe_wifi(const picopen_wifi_status_t *wifi,
                                uint64_t now_ms) {
    if (!wifi || wifi->state != PICOPEN_WIFI_CONNECTED) return;
    observe(wifi->ipv4, PICOPEN_HOST_LOCAL, now_ms, true);
    observe(wifi->gateway, PICOPEN_HOST_GATEWAY, now_ms, true);
    observe(wifi->dns, PICOPEN_HOST_DNS, now_ms, true);
}

void picopen_hosts_observe_recon(const picopen_recon_snapshot_t *result,
                                 uint64_t now_ms) {
    if (!result || result->state < PICOPEN_RECON_COMPLETE) return;
    const char *address = result->address[0] ? result->address : result->target;
    picopen_host_record_t *record = find_or_add(address, PICOPEN_HOST_TASK);
    if (!record) return;
    record->last_seen_ms = now_ms;
    record->reachable |= result->state == PICOPEN_RECON_COMPLETE ||
                         result->state == PICOPEN_RECON_REFUSED;
    if (result->port == 0u || result->state != PICOPEN_RECON_COMPLETE) return;
    for (size_t index = 0u; index < record->service_count; ++index)
        if (record->services[index] == result->port) return;
    if (record->service_count < PICOPEN_HOST_SERVICE_CAPACITY)
        record->services[record->service_count++] = result->port;
}

void picopen_hosts_snapshot(picopen_host_inventory_t *output) {
    if (output) *output = inventory;
}

const char *picopen_host_source_name(picopen_host_source_t source) {
    static const char *const names[] = {"LOCAL", "GATEWAY", "DNS", "TASK"};
    return (unsigned)source < sizeof(names) / sizeof(names[0])
        ? names[source] : "UNKNOWN";
}
