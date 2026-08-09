#ifndef PICOPEN_AUDIT_H
#define PICOPEN_AUDIT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PICOPEN_AUDIT_CAPACITY 16u
#define PICOPEN_AUDIT_ACTION_SIZE 16u

typedef struct picopen_audit_record {
    uint64_t sequence;
    uint64_t monotonic_ms;
    char action[PICOPEN_AUDIT_ACTION_SIZE];
    bool allowed;
} picopen_audit_record_t;

void picopen_audit_init(void);
void picopen_audit_record(const char *action, bool allowed);
size_t picopen_audit_count(void);
bool picopen_audit_latest(picopen_audit_record_t *record);

#endif
