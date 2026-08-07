#include <stdio.h>
#include <string.h>

#include "hardware/watchdog.h"
#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "pico/stdio_usb.h"
#include "hardware/structs/watchdog.h"

#include "picopen/boot_format.h"
#include "picopen/handoff.h"
#include "picopen/image_validator.h"

#ifndef PICOPEN_VERSION
#define PICOPEN_VERSION "unknown"
#endif

static const char *handoff_checkpoint(uint32_t previous_attempt) {
    if (previous_attempt == PICOPEN_BOOT_ATTEMPT_OS_ENTERED) {
        return "os-entered";
    }
    if (previous_attempt == PICOPEN_BOOT_ATTEMPT_USB_TIMEOUT) {
        return "os-usb-timeout";
    }
    return "chain-started";
}

static void print_recovery_report(picopen_image_status_t image_status,
                                  bool failed_attempt,
                                  uint32_t previous_attempt,
                                  int chain_status) {
    printf("primary-image: %s\r\n",
           picopen_image_status_string(image_status));
    if (failed_attempt) {
        printf("previous-handoff: watchdog reset at %s\r\n",
               handoff_checkpoint(previous_attempt));
    } else if (image_status == PICOPEN_IMAGE_OK) {
        printf("handoff: image transfer returned unexpectedly (%d)\r\n",
               chain_status);
    }
    printf("status: recovery mode; no unvalidated transfer attempted\r\n");
}

int main(void) {
    picopen_validated_image_t image;
    int chain_status = 0;
    const bool watchdog_reset = watchdog_caused_reboot();
    const uint32_t previous_attempt =
        watchdog_hw->scratch[PICOPEN_BOOT_ATTEMPT_SCRATCH];

    if (watchdog_hw->scratch[PICOPEN_BOOTSEL_REQUEST_SCRATCH] ==
        PICOPEN_BOOTSEL_REQUEST_MAGIC) {
        watchdog_hw->scratch[PICOPEN_BOOTSEL_REQUEST_SCRATCH] = 0u;
        reset_usb_boot(0u, 0u);
    }

    watchdog_disable();
    watchdog_hw->scratch[PICOPEN_BOOT_ATTEMPT_SCRATCH] = 0u;

    const picopen_image_status_t image_status =
        picopen_validate_primary_image(&image);
    const bool failed_attempt = watchdog_reset &&
        ((previous_attempt == PICOPEN_BOOT_ATTEMPT_CHAINING) ||
         (previous_attempt == PICOPEN_BOOT_ATTEMPT_OS_ENTERED) ||
         (previous_attempt == PICOPEN_BOOT_ATTEMPT_USB_TIMEOUT));

    if ((image_status == PICOPEN_IMAGE_OK) && !failed_attempt) {
        watchdog_hw->scratch[PICOPEN_BOOT_ATTEMPT_SCRATCH] =
            PICOPEN_BOOT_ATTEMPT_CHAINING;
        watchdog_enable(PICOPEN_BOOT_ATTEMPT_TIMEOUT_MS, true);
        chain_status = picopen_chain_image(&image);
        watchdog_disable();
        watchdog_hw->scratch[PICOPEN_BOOT_ATTEMPT_SCRATCH] = 0u;
    }

    // A valid image normally never reaches this point. USB belongs exclusively
    // to recovery so the chained OS receives peripheral hardware that has not
    // already been configured by a previous TinyUSB instance.
    stdio_init_all();
    const absolute_time_t console_deadline = make_timeout_time_ms(10000u);
    while (!stdio_usb_connected() && !time_reached(console_deadline)) {
        sleep_ms(10u);
    }
    printf("\r\nPicoPen stage-1 bootloader\r\n");
    printf("version: %s\r\n", PICOPEN_VERSION);
    printf("flash-region: 0x%08lx + 0x%08lx\r\n",
           (unsigned long)(PICOPEN_FLASH_BASE + PICOPEN_BOOTLOADER_OFFSET),
           (unsigned long)PICOPEN_BOOTLOADER_SIZE);
    print_recovery_report(image_status, failed_attempt, previous_attempt,
                          chain_status);
    printf("commands: status, bootsel\r\nbootloader> ");
    fflush(stdout);

    char command[8] = {0};
    size_t length = 0u;
    for (;;) {
        const int input = getchar_timeout_us(0u);
        if ((input == '\r') || (input == '\n')) {
            command[length] = '\0';
            printf("\r\n");
            if (length == 0u) {
                // Print only the prompt.
            } else if (strcmp(command, "status") == 0) {
                print_recovery_report(image_status, failed_attempt,
                                      previous_attempt, chain_status);
            } else if (strcmp(command, "bootsel") == 0) {
                printf("rebooting into RP2350 ROM USB loader\r\n");
                fflush(stdout);
                sleep_ms(50u);
                watchdog_hw->scratch[PICOPEN_BOOTSEL_REQUEST_SCRATCH] =
                    PICOPEN_BOOTSEL_REQUEST_MAGIC;
                watchdog_reboot(0u, 0u, 0u);
            } else {
                printf("error: available commands are 'status' and 'bootsel'\r\n");
            }
            length = 0u;
            printf("bootloader> ");
            fflush(stdout);
        } else if ((input == '\b') || (input == 0x7f)) {
            if (length > 0u) {
                --length;
                printf("\b \b");
            }
        } else if ((input >= 0x20) && (input <= 0x7e)) {
            if (length < (sizeof(command) - 1u)) {
                command[length++] = (char)input;
                putchar(input);
            }
        }
        tight_loop_contents();
    }
}
