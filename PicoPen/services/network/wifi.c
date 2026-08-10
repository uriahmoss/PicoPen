#include "picopen/wifi.h"

#include <stddef.h>

#include "pico/cyw43_arch.h"

static picopen_wifi_status_t current;
static bool driver_initialized;

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
}

void picopen_wifi_poll(void) {
    // pico_cyw43_arch_none owns a bounded background async context. Network
    // protocol stacks and sockets are intentionally not linked in this slice.
}

void picopen_wifi_get_status(picopen_wifi_status_t *status) {
    if (status != NULL) {
        *status = current;
    }
}

const char *picopen_wifi_state_name(picopen_wifi_state_t state) {
    static const char *const names[] = {
        "OFF", "READY-NO-NET", "ERROR",
    };
    return (unsigned int)state < sizeof(names) / sizeof(names[0])
        ? names[state] : "UNKNOWN";
}
