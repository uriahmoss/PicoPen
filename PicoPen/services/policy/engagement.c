#include "picopen/engagement.h"

#include <stddef.h>

void picopen_engagement_init(picopen_engagement_t *engagement) {
    if (engagement != NULL) {
        *engagement = (picopen_engagement_t){0};
    }
}

bool picopen_engagement_is_active(const picopen_engagement_t *engagement,
                                  uint64_t now_ms) {
    return (engagement != NULL) && engagement->active &&
           (engagement->reference[0] != '\0') &&
           (engagement->expires_ms > now_ms);
}
