#include "picopen/engagement.h"

#include <stddef.h>
#include <string.h>

static picopen_engagement_t session;

static bool parse_octet(const char **cursor, uint8_t *octet) {
    unsigned int value = 0u, digits = 0u;
    while (**cursor >= '0' && **cursor <= '9') {
        value = value * 10u + (unsigned int)(*(*cursor)++ - '0');
        if (++digits > 3u || value > 255u) return false;
    }
    if (digits == 0u) return false;
    *octet = (uint8_t)value; return true;
}

static bool parse_cidr(const char *text, uint32_t *network, uint32_t *mask) {
    if (!text || !network || !mask) return false;
    const char *cursor = text; uint8_t octets[4];
    for (size_t index=0u; index<4u; ++index) {
        if (!parse_octet(&cursor,&octets[index])) return false;
        if (index<3u) { if (*cursor!='.') return false; ++cursor; }
    }
    unsigned int prefix=32u;
    if (*cursor=='/') {
        ++cursor; prefix=0u; unsigned int digits=0u;
        while (*cursor>='0' && *cursor<='9') { prefix=prefix*10u+(unsigned int)(*cursor++-'0'); ++digits; }
        if (digits==0u || prefix>32u) return false;
    }
    if (*cursor!='\0') return false;
    const uint32_t address=((uint32_t)octets[0]<<24u)|((uint32_t)octets[1]<<16u)|
        ((uint32_t)octets[2]<<8u)|octets[3];
    *mask=prefix==0u ? 0u : UINT32_MAX << (32u-prefix); *network=address & *mask;
    return true;
}

static bool valid_hostname(const char *value) {
    if (!value || !value[0]) return false;
    size_t length=0u;
    for (; value[length]; ++length) {
        const char c=value[length];
        if (length>=PICOPEN_ENGAGEMENT_TARGET_SIZE-1u ||
            !((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-'||c=='.')) return false;
    }
    return length>=3u && value[0]!='.' && value[length-1u]!='.';
}

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

bool picopen_engagement_session_activate_scoped(
    const char *reference, const char *target, uint16_t port_first,
    uint16_t port_last, uint64_t now_ms, uint64_t duration_ms,
    bool local_confirmation) {
    if (port_first==0u || port_last<port_first ||
        !picopen_engagement_activate(&session,reference,now_ms,duration_ms,local_confirmation)) return false;
    uint32_t network=0u, mask=0u;
    const bool cidr=parse_cidr(target,&network,&mask);
    if (!cidr && !valid_hostname(target)) { picopen_engagement_deactivate(&session); return false; }
    strncpy(session.target,target,sizeof(session.target)-1u);
    session.network=network; session.netmask=mask; session.hostname_target=!cidr;
    session.boundary_configured=true;
    return true;
}

bool picopen_engagement_session_activate_boundary(
    const char *reference, const char *target, uint64_t now_ms,
    uint64_t duration_ms, bool local_confirmation) {
    return picopen_engagement_session_activate_scoped(
        reference, target, 1u, UINT16_MAX, now_ms, duration_ms,
        local_confirmation);
}

bool picopen_engagement_session_activate_optional_boundary(
    const char *reference, const char *target, uint64_t now_ms,
    uint64_t duration_ms, bool local_confirmation) {
    if (target && target[0]) {
        return picopen_engagement_session_activate_boundary(
            reference, target, now_ms, duration_ms, local_confirmation);
    }
    return picopen_engagement_session_activate(
        reference, now_ms, duration_ms, local_confirmation);
}

bool picopen_engagement_session_allows_ipv4(uint32_t address,uint16_t port,uint64_t now_ms) {
    (void)port;
    if (!picopen_engagement_is_active(&session,now_ms)) return false;
    if (!session.boundary_configured) return true;
    return !session.hostname_target &&
        (address & session.netmask)==session.network;
}
bool picopen_engagement_session_allows_hostname(const char *hostname,uint16_t port,uint64_t now_ms) {
    (void)port;
    if (!hostname || !picopen_engagement_is_active(&session,now_ms)) return false;
    if (!session.boundary_configured) return true;
    return session.hostname_target &&
        strcmp(hostname,session.target)==0;
}

bool picopen_engagement_session_allows_task_ipv4(uint32_t address,
                                                 uint64_t now_ms) {
    return picopen_engagement_session_allows_ipv4(address, 1u, now_ms);
}

bool picopen_engagement_session_allows_task_hostname(const char *hostname,
                                                     uint64_t now_ms) {
    return picopen_engagement_session_allows_hostname(hostname, 1u, now_ms);
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
