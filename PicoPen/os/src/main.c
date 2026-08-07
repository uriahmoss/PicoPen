#include <stdio.h>

#include "pico/stdlib.h"

#include "picopen/boot_format.h"

#ifndef PICOPEN_VERSION
#define PICOPEN_VERSION "unknown"
#endif

int main(void) {
    stdio_init_all();
    sleep_ms(750u);

    printf("\r\nPicoPen minimal OS\r\n");
    printf("version: %s\r\n", PICOPEN_VERSION);
    printf("slot: primary\r\n");
    printf("linked-address: 0x%08lx\r\n",
           (unsigned long)(PICOPEN_FLASH_BASE +
                           PICOPEN_PRIMARY_SLOT_OFFSET +
                           PICOPEN_IMAGE_PAYLOAD_OFFSET));
    printf("status: OS skeleton waiting for bootloader handoff\r\n");

    for (;;) {
        printf("os-heartbeat: %llu ms\r\n", time_us_64() / 1000u);
        sleep_ms(5000u);
    }
}
