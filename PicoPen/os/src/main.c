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
#include "picopen/keyboard.h"
#include "picopen/sd.h"
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
    picopen_keyboard_info_t keyboard_info;
    const bool keyboard_ready = picopen_keyboard_init(&keyboard_info);
    picopen_sd_info_t sd_info;
    const bool sd_ready = picopen_sd_identify(&sd_info);
    if (display_ready) {
        char keyboard_status[40];
        char sd_status[40];
        if (keyboard_ready) {
            snprintf(keyboard_status, sizeof(keyboard_status),
                     "KBD FW: 0X%02X READY %luK\n",
                     keyboard_info.response[1],
                     (unsigned long)(keyboard_info.baud_hz / 1000u));
        } else {
            snprintf(keyboard_status, sizeof(keyboard_status),
                     "KBD NACK L:%u%u ADDR:%02X\n",
                     keyboard_info.sda_high ? 1u : 0u,
                     keyboard_info.scl_high ? 1u : 0u,
                     keyboard_info.found_address);
        }
        if (sd_ready) {
            snprintf(sd_status, sizeof(sd_status), "SD: %s V%s OCR:%08lX\n",
                     sd_info.high_capacity ? "SDHC/XC" : "SDSC",
                     sd_info.version_2 ? "2" : "1",
                     (unsigned long)sd_info.ocr);
        } else {
            snprintf(sd_status, sizeof(sd_status), "SD: %s R1:%02X\n",
                     picopen_sd_status_name(sd_info.status),
                     sd_info.last_response);
        }
        picopen_terminal_init();
        picopen_terminal_write(
            "PICOPEN TERMINAL 0.0.1\n"
            "CPI 2.0 DISPLAY: READY\n"
            "BOOT: PRIMARY CONFIRMED\n"
            "STATUS: MINIMAL OS RUNNING\n");
        picopen_terminal_write(keyboard_status);
        picopen_terminal_write(sd_status);
        picopen_terminal_write("\n> ");
        picopen_terminal_render();
    }
    printf("display: %s\r\n",
           display_ready ? "terminal diagnostic rendered" : "unavailable");
    printf("keyboard: %s; firmware=0x%02x; baud=%lu; write=%d; read=%d; "
           "raw=%02x%02x; lines=%u%u; found=0x%02x\r\n",
           keyboard_ready ? "ready" : "unavailable",
           keyboard_info.response[1], (unsigned long)keyboard_info.baud_hz,
           keyboard_info.write_result, keyboard_info.read_result,
           keyboard_info.response[0], keyboard_info.response[1],
           keyboard_info.sda_high ? 1u : 0u,
           keyboard_info.scl_high ? 1u : 0u,
           keyboard_info.found_address);
    printf("sd: %s; type=%s; version=%u; r1=0x%02x; ocr=0x%08lx\r\n",
           picopen_sd_status_name(sd_info.status),
           sd_info.high_capacity ? "SDHC/XC" : "SDSC",
           sd_info.version_2 ? 2u : 1u, sd_info.last_response,
           (unsigned long)sd_info.ocr);

    for (;;) {
        picopen_key_event_t event;
        if (keyboard_ready && picopen_keyboard_poll(&event) &&
            (event.state == PICOPEN_KEY_PRESSED)) {
            char key_text[8];
            if ((event.key >= 0x20u) && (event.key <= 0x7Eu)) {
                key_text[0] = (char)event.key;
                key_text[1] = '\0';
            } else if (event.key == 0x0Au) {
                key_text[0] = '\n';
                key_text[1] = '\0';
            } else if (event.key == 0x08u) {
                key_text[0] = '\b';
                key_text[1] = '\0';
            } else {
                snprintf(key_text, sizeof(key_text), "[%02X]", event.key);
            }
            picopen_terminal_write(key_text);
            picopen_terminal_render();
            printf("key: 0x%02x state=%u\r\n", event.key, event.state);
        }
        static uint64_t next_heartbeat_ms = 0u;
        const uint64_t now_ms = time_us_64() / 1000u;
        if (now_ms >= next_heartbeat_ms) {
            printf("os-heartbeat: %llu ms\r\n", now_ms);
            next_heartbeat_ms = now_ms + 5000u;
        }
        sleep_ms(4u);
    }
}
