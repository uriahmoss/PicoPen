#ifndef PICOPEN_WIFI_H
#define PICOPEN_WIFI_H

#include <stdbool.h>
#include <stdint.h>

#define PICOPEN_WIFI_ABI_VERSION 1u

typedef enum picopen_wifi_state {
    PICOPEN_WIFI_OFF = 0,
    PICOPEN_WIFI_READY_UNASSOCIATED,
    PICOPEN_WIFI_ERROR,
} picopen_wifi_state_t;

typedef struct picopen_wifi_status {
    uint16_t abi_version;
    picopen_wifi_state_t state;
    int driver_result;
    uint32_t transition_count;
} picopen_wifi_status_t;

void picopen_wifi_init(void);
bool picopen_wifi_enable(bool locally_confirmed);
void picopen_wifi_disable(void);
void picopen_wifi_poll(void);
void picopen_wifi_get_status(picopen_wifi_status_t *status);
const char *picopen_wifi_state_name(picopen_wifi_state_t state);

#endif
