#ifndef PICOPEN_RECOVERY_H
#define PICOPEN_RECOVERY_H
#include <stdbool.h>
#include <stdint.h>
typedef struct picopen_crash_record {
    uint16_t version, size;
    uint32_t count, reason, checksum;
} picopen_crash_record_t;
void picopen_recovery_init(bool watchdog_reset, uint32_t boot_scratch);
bool picopen_recovery_get(picopen_crash_record_t *record);
bool picopen_recovery_clear(bool locally_confirmed);
#endif
