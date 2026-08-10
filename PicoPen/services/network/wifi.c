#include "picopen/wifi.h"

#include <stddef.h>
#include <string.h>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/dns.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"

static picopen_wifi_status_t current;
static bool driver_initialized;
static uint64_t connect_deadline_ms;
static uint64_t dhcp_deadline_ms;

static void clear_ip(void) {
    current.dhcp_bound = false;
    current.ipv4[0] = current.gateway[0] = current.dns[0] = '\0';
    current.rssi = 0;
}

static void update_ip(void) {
    struct netif *const interface = netif_default;
    if (!interface || !netif_is_up(interface)) { clear_ip(); return; }
    const ip4_addr_t *address = netif_ip4_addr(interface);
    current.dhcp_bound = address && !ip4_addr_isany_val(*address);
    if (!current.dhcp_bound) { clear_ip(); return; }
    (void)ip4addr_ntoa_r(address, current.ipv4, sizeof(current.ipv4));
    (void)ip4addr_ntoa_r(netif_ip4_gw(interface), current.gateway, sizeof(current.gateway));
    const ip_addr_t *server = dns_getserver(0u);
    if (IP_IS_V4(server)) (void)ip4addr_ntoa_r(ip_2_ip4(server), current.dns, sizeof(current.dns));
    (void)cyw43_wifi_get_rssi(&cyw43_state, &current.rssi);
}

static int scan_result(void *environment,
                       const cyw43_ev_scan_result_t *result) {
    (void)environment;
    if (result == NULL) {
        return 0;
    }
    if (current.ap_count >= PICOPEN_WIFI_AP_CAPACITY) {
        current.ap_truncated = true;
        return 0;
    }
    picopen_wifi_ap_t *const ap = &current.aps[current.ap_count++];
    const size_t length = result->ssid_len < PICOPEN_WIFI_SSID_SIZE - 1u
        ? result->ssid_len : PICOPEN_WIFI_SSID_SIZE - 1u;
    memcpy(ap->ssid, result->ssid, length);
    ap->ssid[length] = '\0';
    ap->rssi = result->rssi;
    ap->channel = result->channel;
    ap->auth_mode = result->auth_mode;
    return 0;
}

static void scrub(char *value) {
    if (value == NULL) return;
    volatile char *cursor = value;
    while (*cursor != '\0') *cursor++ = '\0';
}

void picopen_wifi_init(void) {
    current = (picopen_wifi_status_t){
        .abi_version = PICOPEN_WIFI_ABI_VERSION,
        .state = PICOPEN_WIFI_OFF,
    };
    driver_initialized = false;
}

bool picopen_wifi_enable(bool locally_confirmed) {
    if (!locally_confirmed || (current.state != PICOPEN_WIFI_OFF) ||
        driver_initialized) {
        return false;
    }
    current.driver_result = cyw43_arch_init();
    ++current.transition_count;
    if (current.driver_result != 0) {
        current.state = PICOPEN_WIFI_ERROR;
        return false;
    }
    driver_initialized = true;
    cyw43_arch_enable_sta_mode();
    current.state = PICOPEN_WIFI_READY_UNASSOCIATED;
    return true;
}

bool picopen_wifi_scan_passive(bool locally_confirmed) {
    if (!locally_confirmed || !driver_initialized ||
        (current.state != PICOPEN_WIFI_READY_UNASSOCIATED)) return false;
    current.ap_count = 0u;
    current.ap_truncated = false;
    cyw43_wifi_scan_options_t options = {.scan_type = 1};
    current.driver_result = cyw43_wifi_scan(&cyw43_state, &options, NULL,
                                             scan_result);
    if (current.driver_result != 0) return false;
    current.state = PICOPEN_WIFI_SCANNING;
    return true;
}

bool picopen_wifi_select_ap(size_t index, char *ssid, size_t capacity) {
    if (!ssid || capacity == 0u || index >= current.ap_count) return false;
    const size_t length = strlen(current.aps[index].ssid);
    if (length + 1u > capacity) return false;
    memcpy(ssid, current.aps[index].ssid, length + 1u);
    return true;
}

bool picopen_wifi_connect(const char *ssid, char *password,
                          bool locally_confirmed, uint64_t now_ms) {
    if (!locally_confirmed || !driver_initialized || (ssid == NULL) ||
        (password == NULL) || (ssid[0] == '\0') || (password[0] == '\0') ||
        (current.state != PICOPEN_WIFI_READY_UNASSOCIATED)) {
        scrub(password);
        return false;
    }
    current.driver_result = cyw43_arch_wifi_connect_async(
        ssid, password, CYW43_AUTH_WPA2_MIXED_PSK);
    scrub(password);
    if (current.driver_result != 0) return false;
    current.state = PICOPEN_WIFI_CONNECTING;
    connect_deadline_ms = now_ms + 15000u;
    dhcp_deadline_ms = 0u;
    return true;
}

bool picopen_wifi_disconnect(bool locally_confirmed) {
    if (!locally_confirmed || !driver_initialized) return false;
    current.driver_result = cyw43_wifi_leave(&cyw43_state, CYW43_ITF_STA);
    current.state = PICOPEN_WIFI_READY_UNASSOCIATED;
    current.link_status = CYW43_LINK_DOWN;
    clear_ip();
    return current.driver_result == 0;
}

void picopen_wifi_disable(void) {
    if (driver_initialized) {
        cyw43_arch_disable_sta_mode();
        cyw43_arch_deinit();
        driver_initialized = false;
    }
    if (current.state != PICOPEN_WIFI_OFF) {
        ++current.transition_count;
    }
    current.state = PICOPEN_WIFI_OFF;
    current.driver_result = 0;
    clear_ip();
}

void picopen_wifi_poll(void) {
    if (!driver_initialized) return;
    cyw43_arch_poll();
    if ((current.state == PICOPEN_WIFI_SCANNING) &&
        !cyw43_wifi_scan_active(&cyw43_state)) {
        current.state = PICOPEN_WIFI_READY_UNASSOCIATED;
    }
    if (current.state == PICOPEN_WIFI_CONNECTING) {
        current.link_status = cyw43_wifi_link_status(&cyw43_state,
                                                     CYW43_ITF_STA);
        if ((current.link_status == CYW43_LINK_JOIN) ||
            (current.link_status == CYW43_LINK_UP)) {
            current.state = PICOPEN_WIFI_CONNECTED;
            dhcp_deadline_ms = time_us_64() / 1000u + 15000u;
        } else if ((time_us_64() / 1000u >= connect_deadline_ms) ||
                   (current.link_status == CYW43_LINK_BADAUTH) ||
                   (current.link_status == CYW43_LINK_FAIL) ||
                   (current.link_status == CYW43_LINK_NONET)) {
            current.state = PICOPEN_WIFI_ERROR;
        }
    }
    if (current.state == PICOPEN_WIFI_CONNECTED) {
        update_ip();
        if (!current.dhcp_bound && dhcp_deadline_ms != 0u &&
            time_us_64() / 1000u >= dhcp_deadline_ms) {
            (void)cyw43_wifi_leave(&cyw43_state, CYW43_ITF_STA);
            current.driver_result = -2;
            current.state = PICOPEN_WIFI_ERROR;
            clear_ip();
        }
    }
}

void picopen_wifi_get_status(picopen_wifi_status_t *status) {
    if (status != NULL) {
        *status = current;
    }
}

const char *picopen_wifi_state_name(picopen_wifi_state_t state) {
    static const char *const names[] = {
        "OFF", "READY", "SCANNING", "CONNECTING", "CONNECTED", "ERROR",
    };
    return (unsigned int)state < sizeof(names) / sizeof(names[0])
        ? names[state] : "UNKNOWN";
}
