#ifndef PICOPEN_ENGAGEMENT_H
#define PICOPEN_ENGAGEMENT_H

#include <stdbool.h>
#include <stdint.h>

#define PICOPEN_ENGAGEMENT_REFERENCE_SIZE 24u
#define PICOPEN_ENGAGEMENT_TARGET_SIZE 64u
#define PICOPEN_ENGAGEMENT_MIN_DURATION_MS UINT64_C(60000)
#define PICOPEN_ENGAGEMENT_MAX_DURATION_MS UINT64_C(14400000)

typedef struct picopen_engagement {
    bool active;
    uint64_t expires_ms;
    char reference[PICOPEN_ENGAGEMENT_REFERENCE_SIZE];
    char target[PICOPEN_ENGAGEMENT_TARGET_SIZE];
    uint32_t network;
    uint32_t netmask;
    uint16_t port_first;
    uint16_t port_last;
    bool hostname_target;
} picopen_engagement_t;

void picopen_engagement_init(picopen_engagement_t *engagement);
bool picopen_engagement_is_active(const picopen_engagement_t *engagement,
                                  uint64_t now_ms);
bool picopen_engagement_activate(picopen_engagement_t *engagement,
                                 const char *reference, uint64_t now_ms,
                                 uint64_t duration_ms,
                                 bool local_confirmation);
void picopen_engagement_deactivate(picopen_engagement_t *engagement);

void picopen_engagement_session_init(void);
bool picopen_engagement_session_activate(const char *reference,
                                         uint64_t now_ms,
                                         uint64_t duration_ms,
                                         bool local_confirmation);
bool picopen_engagement_session_activate_scoped(
    const char *reference, const char *target, uint16_t port_first,
    uint16_t port_last, uint64_t now_ms, uint64_t duration_ms,
    bool local_confirmation);
bool picopen_engagement_session_allows_ipv4(uint32_t address, uint16_t port,
                                            uint64_t now_ms);
bool picopen_engagement_session_allows_hostname(const char *hostname,
                                                uint16_t port,
                                                uint64_t now_ms);
bool picopen_engagement_session_deactivate(bool local_confirmation);
bool picopen_engagement_session_poll(uint64_t now_ms);
void picopen_engagement_session_snapshot(picopen_engagement_t *engagement);
bool picopen_engagement_session_active(uint64_t now_ms);

#endif
