#ifndef PICOPEN_WIFI_H
#define PICOPEN_WIFI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PICOPEN_WIFI_ABI_VERSION 2u
#define PICOPEN_WIFI_AP_CAPACITY 6u
#define PICOPEN_WIFI_SSID_SIZE 33u

typedef enum picopen_wifi_state {
    PICOPEN_WIFI_OFF = 0,
    PICOPEN_WIFI_READY_UNASSOCIATED,
    PICOPEN_WIFI_SCANNING,
    PICOPEN_WIFI_CONNECTING,
    PICOPEN_WIFI_CONNECTED,
    PICOPEN_WIFI_ERROR,
} picopen_wifi_state_t;

typedef struct picopen_wifi_ap {
    char ssid[PICOPEN_WIFI_SSID_SIZE];
    int16_t rssi;
    uint16_t channel;
    uint8_t auth_mode;
} picopen_wifi_ap_t;

typedef struct picopen_wifi_status {
    uint16_t abi_version;
    picopen_wifi_state_t state;
    int driver_result;
    uint32_t transition_count;
    size_t ap_count;
    bool ap_truncated;
    int link_status;
    bool dhcp_bound;
    char ipv4[16];
    char gateway[16];
    char dns[16];
    int32_t rssi;
    picopen_wifi_ap_t aps[PICOPEN_WIFI_AP_CAPACITY];
} picopen_wifi_status_t;

void picopen_wifi_init(void);
bool picopen_wifi_enable(bool locally_confirmed);
bool picopen_wifi_scan_passive(bool locally_confirmed);
bool picopen_wifi_select_ap(size_t index, char *ssid, size_t capacity);
bool picopen_wifi_connect(const char *ssid, char *password,
                          bool locally_confirmed, uint64_t now_ms);
bool picopen_wifi_disconnect(bool locally_confirmed);
void picopen_wifi_disable(void);
void picopen_wifi_poll(void);
void picopen_wifi_get_status(picopen_wifi_status_t *status);
const char *picopen_wifi_state_name(picopen_wifi_state_t state);

#endif
