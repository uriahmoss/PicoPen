#include <stdio.h>

#include "hardware/regs/resets.h"
#include "hardware/resets.h"
#include "hardware/structs/watchdog.h"
#include "hardware/watchdog.h"
#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "pico/stdio_usb.h"

#include "picopen/boot_format.h"

#ifndef PICOPEN_VERSION
#define PICOPEN_VERSION "unknown"
#endif

int main(void) {
    watchdog_hw->scratch[PICOPEN_BOOT_ATTEMPT_SCRATCH] =
        PICOPEN_BOOT_ATTEMPT_OS_ENTERED;

    // The preceding ROM loader or chain operation may leave USBCTRL configured.
    // Reset it before TinyUSB becomes the sole owner in the OS.
    reset_block(RESETS_RESET_USBCTRL_BITS);
    unreset_block_wait(RESETS_RESET_USBCTRL_BITS);
    stdio_init_all();

    const absolute_time_t usb_deadline =
        make_timeout_time_ms(PICOPEN_OS_USB_WAIT_MS);
    while (!stdio_usb_connected() && !time_reached(usb_deadline)) {
        sleep_ms(10u);
    }
    if (!stdio_usb_connected()) {
        watchdog_hw->scratch[PICOPEN_BOOT_ATTEMPT_SCRATCH] =
            PICOPEN_BOOT_ATTEMPT_USB_TIMEOUT;
        watchdog_reboot(0u, 0u, 0u);
    }

    watchdog_hw->scratch[PICOPEN_BOOT_ATTEMPT_SCRATCH] = 0u;
    watchdog_disable();

    printf("\r\nPicoPen minimal OS\r\n");
    printf("version: %s\r\n", PICOPEN_VERSION);
    printf("slot: primary\r\n");
    printf("linked-address: 0x%08lx\r\n",
           (unsigned long)(PICOPEN_FLASH_BASE +
                           PICOPEN_PRIMARY_SLOT_OFFSET +
                           PICOPEN_IMAGE_PAYLOAD_OFFSET));
    const int boot_type = rom_get_last_boot_type_with_chained_flag();
    printf("boot-source: %s\r\n",
           ((boot_type >= 0) && ((boot_type & BOOT_TYPE_CHAINED_FLAG) != 0))
               ? "PicoPen bootloader / ROM chain"
               : "direct or unknown");
    printf("status: minimal OS running\r\n");

    for (;;) {
        printf("os-heartbeat: %llu ms\r\n", time_us_64() / 1000u);
        sleep_ms(5000u);
    }
}
