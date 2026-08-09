#include "picopen/engagement.h"

#include <stddef.h>
#include <string.h>

static picopen_engagement_t session;

static bool valid_reference(const char *reference) {
    if (reference == NULL) {
        return false;
    }
    size_t length = 0u;
    for (; reference[length] != '\0'; ++length) {
        const char value = reference[length];
        if ((length >= PICOPEN_ENGAGEMENT_REFERENCE_SIZE - 1u) ||
            !(((value >= 'A') && (value <= 'Z')) ||
              ((value >= 'a') && (value <= 'z')) ||
              ((value >= '0') && (value <= '9')) ||
              (value == '-') || (value == '_') || (value == '.'))) {
            return false;
        }
    }
    return length >= 3u;
}

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

bool picopen_engagement_activate(picopen_engagement_t *engagement,
                                 const char *reference, uint64_t now_ms,
                                 uint64_t duration_ms,
                                 bool local_confirmation) {
    if ((engagement == NULL) || !local_confirmation ||
        !valid_reference(reference) ||
        (duration_ms < PICOPEN_ENGAGEMENT_MIN_DURATION_MS) ||
        (duration_ms > PICOPEN_ENGAGEMENT_MAX_DURATION_MS) ||
        (UINT64_MAX - now_ms < duration_ms)) {
        return false;
    }
    *engagement = (picopen_engagement_t){
        .active = true,
        .expires_ms = now_ms + duration_ms,
    };
    strncpy(engagement->reference, reference,
            sizeof(engagement->reference) - 1u);
    return true;
}

void picopen_engagement_deactivate(picopen_engagement_t *engagement) {
    if (engagement != NULL) {
        *engagement = (picopen_engagement_t){0};
    }
}

void picopen_engagement_session_init(void) {
    picopen_engagement_init(&session);
}

bool picopen_engagement_session_activate(const char *reference,
                                         uint64_t now_ms,
                                         uint64_t duration_ms,
                                         bool local_confirmation) {
    return picopen_engagement_activate(&session, reference, now_ms,
                                       duration_ms, local_confirmation);
}

bool picopen_engagement_session_deactivate(bool local_confirmation) {
    if (!local_confirmation || !session.active) {
        return false;
    }
    picopen_engagement_deactivate(&session);
    return true;
}

bool picopen_engagement_session_poll(uint64_t now_ms) {
    if (session.active && !picopen_engagement_is_active(&session, now_ms)) {
        picopen_engagement_deactivate(&session);
        return true;
    }
    return false;
}

void picopen_engagement_session_snapshot(picopen_engagement_t *engagement) {
    if (engagement != NULL) {
        *engagement = session;
    }
}

bool picopen_engagement_session_active(uint64_t now_ms) {
    return picopen_engagement_is_active(&session, now_ms);
}
