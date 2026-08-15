#include "picopen/attachment.h"

#include <string.h>

static picopen_attachment_registry_t registry;

static bool valid_text(const char *value, size_t capacity) {
    if (!value || !value[0]) return false;
    for (size_t index = 0u; index < capacity; ++index) {
        const char c = value[index];
        if (c == '\0') return true;
        if (c < 0x20 || c > 0x7e) return false;
    }
    return false;
}

static bool valid_descriptor(const picopen_attachment_descriptor_t *descriptor) {
    const uint64_t supported = PICOPEN_PROVIDER_CAPABILITY_COUNT == 64u
        ? UINT64_MAX
        : (UINT64_C(1) << PICOPEN_PROVIDER_CAPABILITY_COUNT) - 1u;
    return descriptor &&
        descriptor->abi_version == PICOPEN_ATTACHMENT_ABI_VERSION &&
        valid_text(descriptor->id, sizeof(descriptor->id)) &&
        valid_text(descriptor->name, sizeof(descriptor->name)) &&
        descriptor->transport > PICOPEN_ATTACHMENT_TRANSPORT_NONE &&
        descriptor->transport <= PICOPEN_ATTACHMENT_TRANSPORT_MOCK &&
        descriptor->capabilities != 0u &&
        (descriptor->capabilities & ~supported) == 0u &&
        descriptor->max_current_ma <= 1000u;
}

void picopen_attachment_init(void) {
    registry = (picopen_attachment_registry_t){0};
}

bool picopen_attachment_register(const picopen_attachment_descriptor_t *descriptor,
                                 picopen_attachment_state_t state) {
    if (!valid_descriptor(descriptor) || state == PICOPEN_ATTACHMENT_ABSENT)
        return false;
    for (size_t index = 0u; index < registry.count; ++index) {
        if (strcmp(registry.records[index].descriptor.id, descriptor->id) == 0)
            return false;
    }
    if (registry.count >= PICOPEN_ATTACHMENT_CAPACITY) {
        registry.truncated = true;
        return false;
    }
    picopen_attachment_record_t *record = &registry.records[registry.count++];
    record->descriptor = *descriptor;
    record->state = state;
    record->generation = 1u;
    return true;
}

bool picopen_attachment_set_state(const char *id,
                                  picopen_attachment_state_t state,
                                  int error) {
    if (!id || state > PICOPEN_ATTACHMENT_ERROR) return false;
    for (size_t index = 0u; index < registry.count; ++index) {
        picopen_attachment_record_t *record = &registry.records[index];
        if (strcmp(record->descriptor.id, id) != 0) continue;
        record->state = state;
        record->last_error = error;
        ++record->generation;
        return true;
    }
    return false;
}

bool picopen_attachment_has_provider(picopen_provider_capability_t capability) {
    if (capability >= PICOPEN_PROVIDER_CAPABILITY_COUNT) return false;
    const uint64_t requested = UINT64_C(1) << capability;
    for (size_t index = 0u; index < registry.count; ++index) {
        const picopen_attachment_record_t *record = &registry.records[index];
        const bool usable = record->state == PICOPEN_ATTACHMENT_READY ||
            record->state == PICOPEN_ATTACHMENT_READY_RECEIVE_ONLY;
        if (usable && (record->descriptor.capabilities & requested)) return true;
    }
    return false;
}

void picopen_attachment_snapshot(picopen_attachment_registry_t *output) {
    if (output) *output = registry;
}

const char *picopen_attachment_state_name(picopen_attachment_state_t state) {
    static const char *const names[] = {
        "ABSENT", "REGISTERED", "DISABLED", "READY-RX", "READY",
        "DEGRADED", "ERROR",
    };
    return (unsigned)state < sizeof(names) / sizeof(names[0])
        ? names[state] : "UNKNOWN";
}

const char *picopen_attachment_transport_name(
    picopen_attachment_transport_t transport) {
    static const char *const names[] = {
        "NONE", "I2C", "SPI", "UART", "USB", "GPIO", "PIO", "MOCK",
    };
    return (unsigned)transport < sizeof(names) / sizeof(names[0])
        ? names[transport] : "UNKNOWN";
}
