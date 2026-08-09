#include "picopen/audit.h"

#include <string.h>

#include "pico/stdlib.h"

static picopen_audit_record_t records[PICOPEN_AUDIT_CAPACITY];
static uint64_t next_sequence;
static size_t record_count;

void picopen_audit_init(void) {
    memset(records, 0, sizeof(records));
    next_sequence = 1u;
    record_count = 0u;
}

void picopen_audit_record(const char *action, bool allowed) {
    if (action == NULL) {
        return;
    }
    const size_t index = (size_t)((next_sequence - 1u) % PICOPEN_AUDIT_CAPACITY);
    picopen_audit_record_t *const record = &records[index];
    *record = (picopen_audit_record_t){
        .sequence = next_sequence++,
        .monotonic_ms = time_us_64() / 1000u,
        .allowed = allowed,
    };
    strncpy(record->action, action, sizeof(record->action) - 1u);
    record->action[sizeof(record->action) - 1u] = '\0';
    if (record_count < PICOPEN_AUDIT_CAPACITY) {
        ++record_count;
    }
}

size_t picopen_audit_count(void) {
    return record_count;
}

bool picopen_audit_latest(picopen_audit_record_t *record) {
    if ((record == NULL) || (record_count == 0u)) {
        return false;
    }
    const size_t index = (size_t)((next_sequence - 2u) % PICOPEN_AUDIT_CAPACITY);
    *record = records[index];
    return true;
}
