#include "picopen/shell.h"

#include <stdio.h>
#include <string.h>

#include "picopen/terminal.h"
#include "picopen/audit.h"
#include "pico/stdlib.h"

#define SHELL_COMMAND_SIZE 32u

static picopen_shell_state_t shell_state;
static char command[SHELL_COMMAND_SIZE];
static size_t command_length;
static bool shutdown_pending;
static uint64_t shutdown_deadline_ms;

static void output(const char *text) {
    picopen_terminal_write(text);
    printf("%s", text);
}

static void show_prompt(void) {
    output("\n> ");
}

static void show_status(void) {
    char text[96];
    snprintf(text, sizeof(text),
             "KBD:%s SD:%s FS:%s BAT:%s\n",
             shell_state.keyboard_ready ? "READY" : "DOWN",
             picopen_sd_status_name(shell_state.sd.status),
             shell_state.storage_ready ? "FAT-RO" : "NONE",
             shell_state.battery_ready ? "READY" : "UNKNOWN");
    output(text);
    if (shell_state.battery_ready) {
        snprintf(text, sizeof(text), "BATTERY:%u%%%s\n",
                 shell_state.battery.percent,
                 shell_state.battery.charging ? " CHARGING" : "");
        output(text);
    }
}

static void show_root(void) {
    if (!shell_state.storage_ready) {
        output("FILESYSTEM UNAVAILABLE\n");
        return;
    }
    for (size_t index = 0u; index < shell_state.storage.count; ++index) {
        output(shell_state.storage.entries[index].name);
        output(shell_state.storage.entries[index].directory ? "/\n" : "\n");
    }
    if (shell_state.storage.truncated) {
        output("... LIST TRUNCATED\n");
    }
}

static void show_devices(void) {
    char text[64];
    for (size_t index = 0u; index < shell_state.devices.count; ++index) {
        const picopen_device_record_t *const device =
            &shell_state.devices.records[index];
        snprintf(text, sizeof(text), "%s %s\n", device->name,
                 picopen_device_state_name(device->state));
        output(text);
    }
    output(shell_state.ipc_ready ? "IPC V1 READY\n" : "IPC SELF-TEST FAILED\n");
}

static void show_audit(void) {
    char text[96];
    picopen_audit_record_t record;
    if (!picopen_audit_latest(&record)) {
        output("AUDIT EMPTY\n");
        return;
    }
    snprintf(text, sizeof(text), "AUDIT:%u LAST:%llu %s %s\n",
             (unsigned int)picopen_audit_count(), record.sequence,
             record.action, record.allowed ? "ALLOW" : "DENY");
    output(text);
}

static void show_file(const char *name) {
    if (!shell_state.storage_ready ||
        !picopen_security_authorize(&shell_state.security,
                                    PICOPEN_CAP_STORAGE_READ, false)) {
        output("STORAGE READ DENIED\n");
        picopen_audit_record("storage.read", false);
        return;
    }
    uint8_t bytes[PICOPEN_STORAGE_READ_LIMIT];
    size_t bytes_read = 0u;
    bool truncated = false;
    if (!picopen_storage_read_root_file(name, bytes, sizeof(bytes), &bytes_read,
                                        &truncated)) {
        output("FILE UNAVAILABLE\n");
        picopen_audit_record("storage.read", false);
        return;
    }
    char text[PICOPEN_STORAGE_READ_LIMIT + 1u];
    for (size_t index = 0u; index < bytes_read; ++index) {
        const uint8_t value = bytes[index];
        text[index] = ((value == '\n') || (value == '\r') ||
                       ((value >= ' ') && (value <= '~')))
                          ? (char)value
                          : '.';
    }
    text[bytes_read] = '\0';
    output(text);
    output(truncated ? "\n... FILE TRUNCATED AT 256 BYTES\n" : "\n");
    picopen_audit_record("storage.read", true);
}

static void execute(void) {
    command[command_length] = '\0';
    output("\n");
    picopen_audit_record("shell.command", true);
    if ((command_length == 0u) || (strcmp(command, "help") == 0)) {
        output("COMMANDS: HELP STATUS DEVICES LS CAT SCOPE SECURITY AUDIT\n"
               "WORKBENCH SHUTDOWN\n");
    } else if (strcmp(command, "status") == 0) {
        show_status();
    } else if (strcmp(command, "ls") == 0) {
        show_root();
    } else if (strcmp(command, "devices") == 0) {
        show_devices();
    } else if (strcmp(command, "audit") == 0) {
        show_audit();
    } else if (strncmp(command, "cat ", 4u) == 0) {
        show_file(&command[4]);
    } else if (strcmp(command, "scope") == 0) {
        const bool active = picopen_engagement_is_active(
            &shell_state.engagement, time_us_64() / 1000u);
        output(active ? "ENGAGEMENT: ACTIVE\n"
                      : "ENGAGEMENT: INACTIVE\nACTIVE OPERATIONS: DENIED\n");
    } else if (strcmp(command, "workbench") == 0) {
        output("I2C1: KEYBOARD CLAIMED\nSPI0: SD READ-ONLY\n"
               "SPI1: DISPLAY CLAIMED\nGPIO READ: NOT EXPOSED\n"
               "GPIO DRIVE: DENIED\nTRANSMIT: DENIED\n");
    } else if (strcmp(command, "shutdown") == 0) {
        shutdown_pending = true;
        shutdown_deadline_ms = time_us_64() / 1000u + 15000u;
        output("LOCAL SHUTDOWN REQUESTED\n"
               "TYPE SHUTDOWN CONFIRM WITHIN 15 SECONDS\n");
    } else if (strcmp(command, "shutdown cancel") == 0) {
        shutdown_pending = false;
        output("SHUTDOWN CANCELLED\n");
    } else if (strcmp(command, "shutdown confirm") == 0) {
        const uint64_t now_ms = time_us_64() / 1000u;
        const bool authorized = shutdown_pending &&
            (now_ms <= shutdown_deadline_ms) &&
            picopen_security_authorize(&shell_state.security,
                                       PICOPEN_CAP_SYSTEM_SHUTDOWN, true);
        shutdown_pending = false;
        if (!authorized) {
            output("SHUTDOWN DENIED OR EXPIRED\n");
            picopen_audit_record("shutdown", false);
        } else if (!picopen_keyboard_request_shutdown(6u)) {
            output("SHUTDOWN CONTROLLER ERROR\n");
            picopen_audit_record("shutdown", false);
        } else {
            output("SHUTDOWN CONFIRMED; POWER OFF IN 6 SECONDS\n");
            picopen_audit_record("shutdown", true);
        }
    } else if (strcmp(command, "security") == 0) {
        const bool active_denied =
            !picopen_security_authorize(&shell_state.security,
                                        PICOPEN_CAP_RADIO_TRANSMIT, true) &&
            !picopen_security_authorize(&shell_state.security,
                                        PICOPEN_CAP_GPIO_DRIVE, true) &&
            !picopen_security_authorize(&shell_state.security,
                                        PICOPEN_CAP_USB_HID, true) &&
            !picopen_security_authorize(&shell_state.security,
                                        PICOPEN_CAP_REMOTE_CONTROL, true);
        output(active_denied ? "SECURE DEFAULT: ACTIVE\n"
                               "TX GPIO-OUT USB-HID REMOTE: DENIED\n"
                               "SD: READ-ONLY  AUTO-RUN: DENIED\n"
                             : "SECURITY POLICY ERROR\n");
    } else {
        output("UNKNOWN COMMAND\n");
    }
    command_length = 0u;
    show_prompt();
    picopen_terminal_render();
}

void picopen_shell_init(const picopen_shell_state_t *state) {
    if (state != NULL) {
        shell_state = *state;
    }
    command_length = 0u;
    shutdown_pending = false;
    shutdown_deadline_ms = 0u;
    picopen_audit_record("shell.start", true);
}

void picopen_shell_update_state(const picopen_shell_state_t *state) {
    if (state != NULL) {
        shell_state = *state;
    }
}

void picopen_shell_handle_key(uint8_t key) {
    if ((key == '\n') || (key == '\r')) {
        execute();
        return;
    }
    if ((key == '\b') || (key == 0x7Fu)) {
        if (command_length > 0u) {
            --command_length;
            picopen_terminal_write("\b");
            picopen_terminal_render();
        }
        return;
    }
    if ((key < 0x20u) || (key > 0x7Eu) ||
        (command_length >= sizeof(command) - 1u)) {
        return;
    }
    command[command_length++] = (char)key;
    char text[2] = {(char)key, '\0'};
    picopen_terminal_write(text);
    picopen_terminal_render();
}
