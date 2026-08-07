#include <stdio.h>
#include <string.h>

#include "pico/bootrom.h"
#include "pico/stdlib.h"

#include "picopen/boot_format.h"
#include "picopen/image_validator.h"

#ifndef PICOPEN_VERSION
#define PICOPEN_VERSION "unknown"
#endif

int main(void) {
    stdio_init_all();
    sleep_ms(750u);

    printf("\r\nPicoPen stage-1 bootloader\r\n");
    printf("version: %s\r\n", PICOPEN_VERSION);
    printf("flash-region: 0x%08lx + 0x%08lx\r\n",
           (unsigned long)(PICOPEN_FLASH_BASE + PICOPEN_BOOTLOADER_OFFSET),
           (unsigned long)PICOPEN_BOOTLOADER_SIZE);
    const picopen_image_status_t image_status =
        picopen_validate_primary_image();
    printf("primary-image: %s\r\n",
           picopen_image_status_string(image_status));
    printf("safety: validation only; no OS transfer attempted\r\n");
    printf("commands: bootsel\r\nbootloader> ");
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
            } else if (strcmp(command, "bootsel") == 0) {
                printf("entering RP2350 ROM USB loader\r\n");
                fflush(stdout);
                sleep_ms(25u);
                reset_usb_boot(0u, 0u);
            } else {
                printf("error: only 'bootsel' is available in this slice\r\n");
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
