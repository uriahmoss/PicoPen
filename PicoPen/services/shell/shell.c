#include "picopen/shell.h"

#include <stdio.h>
#include <string.h>

#include "picopen/terminal.h"
#include "picopen/audit.h"

#define SHELL_COMMAND_SIZE 32u

static picopen_shell_state_t shell_state;
static char command[SHELL_COMMAND_SIZE];
static size_t command_length;

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
    output(shell_state.keyboard_ready ? "KEYBOARD READY\n" : "KEYBOARD DOWN\n");
    output(shell_state.sd.status == PICOPEN_SD_READY ? "SD READY RO\n"
                                                     : "SD DOWN\n");
    output(shell_state.storage_ready ? "FATFS READY RO\n" : "FATFS DOWN\n");
    output(shell_state.battery_ready ? "BATTERY READY\n" : "BATTERY UNKNOWN\n");
    output("RADIO DISABLED\nUSB-HID DISABLED\nGPIO-OUT DISABLED\n");
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

static void execute(void) {
    command[command_length] = '\0';
    output("\n");
    if ((command_length == 0u) || (strcmp(command, "help") == 0)) {
        output("COMMANDS: HELP STATUS DEVICES LS SECURITY AUDIT\n");
    } else if (strcmp(command, "status") == 0) {
        show_status();
    } else if (strcmp(command, "ls") == 0) {
        show_root();
    } else if (strcmp(command, "devices") == 0) {
        show_devices();
    } else if (strcmp(command, "audit") == 0) {
        show_audit();
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
    picopen_audit_record(command, true);
    command_length = 0u;
    show_prompt();
    picopen_terminal_render();
}

void picopen_shell_init(const picopen_shell_state_t *state) {
    if (state != NULL) {
        shell_state = *state;
    }
    command_length = 0u;
    picopen_audit_record("shell.start", true);
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
