#include <stdbool.h>
#include <stdio.h>

#include "hardware/watchdog.h"
#include "hardware/structs/watchdog.h"
#include "pico/bootrom.h"
#include "pico/stdlib.h"

#include "picopen/bringup.h"
#include "picopen/boot_format.h"
#include "picopen/console.h"

#ifndef PICOPEN_VERSION
#define PICOPEN_VERSION "unknown"
#endif

#ifndef PICOPEN_BUILD_TYPE
#define PICOPEN_BUILD_TYPE "unknown"
#endif

static void print_boot_report(bool watchdog_reset) {
    printf("\r\n%s\r\n", PICOPEN_BRINGUP_BANNER);
    printf("version: %s\r\n", PICOPEN_VERSION);
    printf("build: %s\r\n", PICOPEN_BUILD_TYPE);
    printf("target: Raspberry Pi Pico 2 W\r\n");
    printf("reset: %s\r\n", watchdog_reset ? "watchdog" : "power-on/external");
    printf("cyw43: disabled in minimal recovery image\r\n");
    printf("policy: no PicoCalc GPIO or external transmitter enabled\r\n");
    printf("status: bring-up heartbeat running\r\n");
}

int main(void) {
    const bool watchdog_reset = watchdog_caused_reboot();
    absolute_time_t next_heartbeat;

    if (watchdog_hw->scratch[PICOPEN_BOOTSEL_REQUEST_SCRATCH] ==
        PICOPEN_BOOTSEL_REQUEST_MAGIC) {
        watchdog_hw->scratch[PICOPEN_BOOTSEL_REQUEST_SCRATCH] = 0u;
        reset_usb_boot(0u, 0u);
    }

    stdio_init_all();

    // A short bounded pause gives a newly enumerating USB serial interface a
    // chance to become visible. Boot never waits indefinitely for a host.
    sleep_ms(PICOPEN_USB_SETTLE_MS);

    print_boot_report(watchdog_reset);

    const picopen_console_state_t console_state = {
        .watchdog_reset = watchdog_reset,
    };
    picopen_console_init(&console_state);
    next_heartbeat = make_timeout_time_ms(PICOPEN_HEARTBEAT_PERIOD_MS);

    for (;;) {
        picopen_console_poll();

        if (time_reached(next_heartbeat)) {
            printf("\r\nheartbeat: %llu ms\r\npicopen> ",
                   time_us_64() / 1000u);
            fflush(stdout);
            next_heartbeat = make_timeout_time_ms(PICOPEN_HEARTBEAT_PERIOD_MS);
        }
        sleep_ms(1u);
    }
}
