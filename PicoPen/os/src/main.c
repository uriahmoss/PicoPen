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
#include "picopen/audit.h"
#include "picopen/capability.h"
#include "picopen/device.h"
#include "picopen/engagement.h"
#include "picopen/ipc.h"
#include "picopen/display.h"
#include "picopen/keyboard.h"
#include "picopen/gui.h"
#include "picopen/sd.h"
#include "picopen/shell.h"
#include "picopen/storage.h"
#include "picopen/terminal.h"
#include "picopen/work_queue.h"

#ifndef PICOPEN_VERSION
#define PICOPEN_VERSION "unknown"
#endif

typedef struct heartbeat_context {
    picopen_work_queue_t *queue;
} heartbeat_context_t;

static void heartbeat(void *context_pointer) {
    heartbeat_context_t *const context = context_pointer;
    const uint64_t now_ms = time_us_64() / 1000u;
    printf("os-heartbeat: %llu ms\r\n", now_ms);
    (void)picopen_work_schedule(context->queue, now_ms + 5000u, heartbeat,
                                context);
}

static bool system_status_handler(const picopen_ipc_message_t *request,
                                  picopen_ipc_message_t *response,
                                  void *owner) {
    (void)owner;
    if ((request == NULL) || (response == NULL) || (request->operation != 1u)) {
        return false;
    }
    response->payload[0] = 1u;
    response->payload_size = 1u;
    return true;
}

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
    picopen_audit_init();

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
    picopen_battery_info_t battery_info = {0};
    const bool battery_ready =
        keyboard_ready && picopen_keyboard_read_battery(&battery_info);
    picopen_sd_info_t sd_info;
    const bool sd_ready = picopen_sd_identify(&sd_info);
    picopen_storage_listing_t storage_listing = {0};
    const bool storage_ready =
        sd_ready && picopen_storage_list_root(&storage_listing);
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
            snprintf(sd_status, sizeof(sd_status), "SD: %s %s %s LBA:%lu\n",
                     sd_info.high_capacity ? "SDHC/XC" : "SDSC",
                     picopen_sd_filesystem_name(sd_info.filesystem),
                     sd_info.version_2 ? "2" : "1",
                     (unsigned long)sd_info.first_partition_lba);
        } else {
            snprintf(sd_status, sizeof(sd_status), "SD: %s %s D%u R1:%02X\n",
                     picopen_sd_status_name(sd_info.status),
                     sd_info.software_spi ? "BB" : "HW",
                     sd_info.card_detected ? 1u : 0u,
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
        if (battery_ready) {
            char battery_status[40];
            snprintf(battery_status, sizeof(battery_status),
                     "BATTERY: %u%%%s\n", battery_info.percent,
                     battery_info.charging ? " CHARGING" : "");
            picopen_terminal_write(battery_status);
        } else {
            picopen_terminal_write("BATTERY: UNAVAILABLE\n");
        }
        if (storage_ready) {
            char storage_status[40];
            snprintf(storage_status, sizeof(storage_status),
                     "ROOT: %u ENTRIES%s\n",
                     (unsigned int)storage_listing.count,
                     storage_listing.truncated ? "+" : "");
            picopen_terminal_write(storage_status);
        } else {
            picopen_terminal_write("ROOT: UNAVAILABLE\n");
        }
        picopen_terminal_write("\nSTARTING GUI...\n");
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
    printf("sd: %s; detected=%u; transport=%s; type=%s; version=%u; r1=0x%02x; "
           "ocr=0x%08lx; sector=%u; partitioned=%u; first-lba=%lu; fs=%s\r\n",
           picopen_sd_status_name(sd_info.status),
           sd_info.card_detected ? 1u : 0u,
           sd_info.software_spi ? "software" : "hardware",
           sd_info.high_capacity ? "SDHC/XC" : "SDSC",
           sd_info.version_2 ? 2u : 1u, sd_info.last_response,
           (unsigned long)sd_info.ocr, sd_info.sector_read ? 1u : 0u,
           sd_info.partitioned ? 1u : 0u,
           (unsigned long)sd_info.first_partition_lba,
           picopen_sd_filesystem_name(sd_info.filesystem));
    printf("storage: %s; result=%d; entries=%u; truncated=%u\r\n",
           storage_ready ? "ready" : "unavailable", storage_listing.result,
           (unsigned int)storage_listing.count,
           storage_listing.truncated ? 1u : 0u);
    printf("battery: %s; percent=%u; charging=%u\r\n",
           battery_ready ? "ready" : "unavailable",
           battery_ready ? battery_info.percent : 0u,
           battery_ready && battery_info.charging ? 1u : 0u);
    for (size_t index = 0u; storage_ready && index < storage_listing.count;
         ++index) {
        printf("root[%u]: %s%s\r\n", (unsigned int)index,
               storage_listing.entries[index].name,
               storage_listing.entries[index].directory ? "/" : "");
    }

    picopen_device_manager_t devices;
    picopen_device_manager_init(&devices);
    (void)picopen_device_register(&devices, 1u, "DISPLAY",
        display_ready ? PICOPEN_DEVICE_READY : PICOPEN_DEVICE_UNAVAILABLE,
        false);
    (void)picopen_device_register(&devices, 2u, "KEYBOARD",
        keyboard_ready ? PICOPEN_DEVICE_READY : PICOPEN_DEVICE_UNAVAILABLE,
        false);
    (void)picopen_device_register(&devices, 3u, "SD",
        sd_ready ? PICOPEN_DEVICE_READY_READ_ONLY : PICOPEN_DEVICE_UNAVAILABLE,
        false);
    (void)picopen_device_register(&devices, 4u, "FATFS",
        storage_ready ? PICOPEN_DEVICE_READY_READ_ONLY
                      : PICOPEN_DEVICE_UNAVAILABLE, false);
    (void)picopen_device_register(&devices, 5u, "BATTERY",
        battery_ready ? PICOPEN_DEVICE_READY : PICOPEN_DEVICE_UNAVAILABLE,
        false);
    (void)picopen_device_register(&devices, 6u, "PSRAM",
        PICOPEN_DEVICE_UNVERIFIED, false);
    (void)picopen_device_register(&devices, 7u, "WIFI",
        PICOPEN_DEVICE_DISABLED_POLICY, false);
    (void)picopen_device_register(&devices, 8u, "BLE",
        PICOPEN_DEVICE_DISABLED_POLICY, false);
    (void)picopen_device_register(&devices, 9u, "ATTACHMENTS",
        PICOPEN_DEVICE_DISABLED_POLICY, true);

    picopen_engagement_t engagement;
    picopen_engagement_init(&engagement);
    picopen_security_context_t shell_security = picopen_security_default();
    shell_security.grants =
        (UINT64_C(1) << PICOPEN_CAP_STORAGE_READ) |
        (UINT64_C(1) << PICOPEN_CAP_SYSTEM_SHUTDOWN);

    picopen_ipc_bus_t ipc;
    picopen_ipc_init(&ipc);
    const bool ipc_registered =
        picopen_ipc_register(&ipc, 1u, PICOPEN_IPC_NO_CAPABILITY,
                             system_status_handler, NULL) &&
        picopen_ipc_register(&ipc, 2u, PICOPEN_CAP_RADIO_TRANSMIT,
                             system_status_handler, NULL);
    const picopen_ipc_message_t ipc_request = {
        .version = PICOPEN_IPC_VERSION,
        .service = 1u,
        .operation = 1u,
        .request_id = 1u,
    };
    picopen_ipc_message_t ipc_response;
    const picopen_ipc_message_t denied_request = {
        .version = PICOPEN_IPC_VERSION,
        .service = 2u,
        .operation = 1u,
        .request_id = 2u,
    };
    picopen_ipc_message_t denied_response;
    const bool ipc_ready = ipc_registered &&
        (picopen_ipc_dispatch(&ipc, &shell_security, false, &ipc_request,
                              &ipc_response) == PICOPEN_IPC_OK) &&
        (ipc_response.payload_size == 1u) && (ipc_response.payload[0] == 1u) &&
        (picopen_ipc_dispatch(&ipc, &shell_security, true, &denied_request,
                              &denied_response) == PICOPEN_IPC_DENIED);

    const picopen_shell_state_t shell_state = {
        .keyboard_ready = keyboard_ready,
        .battery_ready = battery_ready,
        .storage_ready = storage_ready,
        .battery = battery_info,
        .sd = sd_info,
        .storage = storage_listing,
        .security = shell_security,
        .devices = devices,
        .engagement = engagement,
        .ipc_ready = ipc_ready,
    };
    picopen_shell_init(&shell_state);
    if (display_ready) {
        picopen_gui_init(&shell_state);
    }

    picopen_work_queue_t work_queue;
    picopen_work_queue_init(&work_queue);
    heartbeat_context_t heartbeat_context = {.queue = &work_queue};
    (void)picopen_work_schedule(&work_queue, time_us_64() / 1000u, heartbeat,
                                &heartbeat_context);

    for (;;) {
        picopen_key_event_t event;
        if (display_ready && keyboard_ready && picopen_keyboard_poll(&event) &&
            (event.state == PICOPEN_KEY_PRESSED)) {
            picopen_gui_handle_key(event.key);
            printf("key: 0x%02x state=%u\r\n", event.key, event.state);
        }
        const uint64_t now_ms = time_us_64() / 1000u;
        (void)picopen_work_run_due(&work_queue, now_ms, 2u);
        sleep_ms(4u);
    }
}
