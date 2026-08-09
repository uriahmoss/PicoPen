#ifndef PICOPEN_ENGAGEMENT_H
#define PICOPEN_ENGAGEMENT_H

#include <stdbool.h>
#include <stdint.h>

#define PICOPEN_ENGAGEMENT_REFERENCE_SIZE 24u

typedef struct picopen_engagement {
    bool active;
    uint64_t expires_ms;
    char reference[PICOPEN_ENGAGEMENT_REFERENCE_SIZE];
} picopen_engagement_t;

void picopen_engagement_init(picopen_engagement_t *engagement);
bool picopen_engagement_is_active(const picopen_engagement_t *engagement,
                                  uint64_t now_ms);

#endif
