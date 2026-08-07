#include <stdio.h>

#include "hardware/regs/resets.h"
#include "hardware/resets.h"
#include "hardware/structs/watchdog.h"
#include "hardware/watchdog.h"
#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "pico/stdio_usb.h"

#include "picopen/boot_format.h"
#include "picopen/boot_metadata.h"
#include "picopen/display.h"
#include "picopen/terminal.h"

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

    picopen_boot_metadata_v1_t metadata;
    if (!picopen_boot_metadata_load(&metadata) ||
        ((metadata.flags & PICOPEN_BOOT_FLAG_PENDING) == 0u)) {
        watchdog_hw->scratch[PICOPEN_BOOT_ATTEMPT_SCRATCH] =
            PICOPEN_BOOT_ATTEMPT_METADATA_ERROR;
        watchdog_reboot(0u, 0u, 0u);
    }
    metadata.flags = PICOPEN_BOOT_FLAG_CONFIRMED;
    metadata.attempt_count = 0u;
    metadata.last_failure = 0u;
    if (!picopen_boot_metadata_store(&metadata)) {
        watchdog_hw->scratch[PICOPEN_BOOT_ATTEMPT_SCRATCH] =
            PICOPEN_BOOT_ATTEMPT_METADATA_ERROR;
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
    printf("boot-success: confirmed; attempts reset to 0/%u\r\n",
           PICOPEN_BOOT_MAX_ATTEMPTS);
    const bool display_ready = picopen_display_init();
    if (display_ready) {
        picopen_terminal_init();
        picopen_terminal_write(
            "PICOPEN TERMINAL 0.0.1\n"
            "CPI 2.0 DISPLAY: READY\n"
            "BOOT: PRIMARY CONFIRMED\n"
            "STATUS: MINIMAL OS RUNNING\n\n"
            "> SYNTHWAVE CONSOLE ONLINE");
        picopen_terminal_render();
    }
    printf("display: %s\r\n",
           display_ready ? "terminal diagnostic rendered" : "unavailable");

    for (;;) {
        printf("os-heartbeat: %llu ms\r\n", time_us_64() / 1000u);
        sleep_ms(5000u);
    }
}
