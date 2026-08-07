#include "picopen/console.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "hardware/watchdog.h"
#include "hardware/structs/watchdog.h"
#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "pico/unique_id.h"

#include "picopen/boot_format.h"

#ifndef PICOPEN_VERSION
#define PICOPEN_VERSION "unknown"
#endif

#define PICOPEN_CONSOLE_LINE_CAPACITY 64u

static picopen_console_state_t state;
static char line_buffer[PICOPEN_CONSOLE_LINE_CAPACITY];
static size_t line_length;

static void print_prompt(void) {
    printf("picopen> ");
    fflush(stdout);
}

static void print_help(void) {
    printf("commands:\r\n");
    printf("  help           show this command list\r\n");
    printf("  info           show board and boot information\r\n");
    printf("  uptime         show milliseconds since boot\r\n");
    printf("  reboot         perform a watchdog reset\r\n");
    printf("  bootsel        return to the RP2350 ROM USB loader\r\n");
}

static void print_info(void) {
    char board_id[2u * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1u];

    pico_get_unique_board_id_string(board_id, sizeof(board_id));
    printf("product: PicoPen\r\n");
    printf("version: %s\r\n", PICOPEN_VERSION);
    printf("target: pico2_w / rp2350-arm-s\r\n");
    printf("board-id: %s\r\n", board_id);
    printf("reset: %s\r\n",
           state.watchdog_reset ? "watchdog" : "power-on/external");
    printf("cyw43: disabled in minimal recovery image\r\n");
    printf("transmit-policy: disabled\r\n");
}

static void normalize_line(char *line) {
    char *read_cursor = line;
    char *write_cursor = line;
    bool previous_space = true;

    while (*read_cursor != '\0') {
        const unsigned char current = (unsigned char)*read_cursor++;
        if (isspace(current)) {
            if (!previous_space) {
                *write_cursor++ = ' ';
                previous_space = true;
            }
        } else {
            *write_cursor++ = (char)tolower(current);
            previous_space = false;
        }
    }

    if ((write_cursor > line) && (write_cursor[-1] == ' ')) {
        --write_cursor;
    }
    *write_cursor = '\0';
}

static void execute_line(char *line) {
    normalize_line(line);

    if (line[0] == '\0') {
        return;
    } else if (strcmp(line, "help") == 0) {
        print_help();
    } else if (strcmp(line, "info") == 0) {
        print_info();
    } else if (strcmp(line, "uptime") == 0) {
        printf("uptime-ms: %llu\r\n", time_us_64() / 1000u);
    } else if (strcmp(line, "reboot") == 0) {
        printf("rebooting via watchdog\r\n");
        fflush(stdout);
        sleep_ms(25u);
        watchdog_reboot(0u, 0u, 0u);
        for (;;) {
            tight_loop_contents();
        }
    } else if (strcmp(line, "bootsel") == 0) {
        printf("rebooting into RP2350 ROM USB loader\r\n");
        fflush(stdout);
        sleep_ms(50u);
        watchdog_hw->scratch[PICOPEN_BOOTSEL_REQUEST_SCRATCH] =
            PICOPEN_BOOTSEL_REQUEST_MAGIC;
        watchdog_reboot(0u, 0u, 0u);
        for (;;) {
            tight_loop_contents();
        }
    } else {
        printf("error: unknown command; type 'help'\r\n");
    }
}

void picopen_console_init(const picopen_console_state_t *initial_state) {
    state = *initial_state;
    line_length = 0u;
    print_help();
    print_prompt();
}

void picopen_console_poll(void) {
    int input;

    while ((input = getchar_timeout_us(0u)) != PICO_ERROR_TIMEOUT) {
        if ((input == '\r') || (input == '\n')) {
            printf("\r\n");
            line_buffer[line_length] = '\0';
            execute_line(line_buffer);
            line_length = 0u;
            print_prompt();
        } else if ((input == '\b') || (input == 0x7f)) {
            if (line_length > 0u) {
                --line_length;
                printf("\b \b");
            }
        } else if (isprint((unsigned char)input)) {
            if (line_length < (sizeof(line_buffer) - 1u)) {
                line_buffer[line_length++] = (char)input;
                putchar(input);
            } else {
                putchar('\a');
            }
        }
    }
}
