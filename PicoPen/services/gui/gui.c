#include "picopen/gui.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "picopen/audit.h"
#include "picopen/display.h"
#include "picopen/keyboard.h"
#include "picopen/storage.h"
#include "picopen/terminal.h"

#define GUI_SCREEN_SIZE 1024u
#define GUI_HOME_ITEMS 6u
#define GUI_SYSTEM_ITEMS 5u

#define GUI_COLOR_BACKGROUND UINT32_C(0x080020)
#define GUI_COLOR_PANEL      UINT32_C(0x101040)
#define GUI_COLOR_HEADER     UINT32_C(0x12002F)
#define GUI_COLOR_CYAN       UINT32_C(0x00E5FF)
#define GUI_COLOR_MAGENTA    UINT32_C(0xFF2BD6)
#define GUI_COLOR_VIOLET     UINT32_C(0x8B5CF6)
#define GUI_COLOR_ORANGE     UINT32_C(0xFF8A30)
#define GUI_COLOR_GOLD       UINT32_C(0xFFCC00)
#define GUI_COLOR_MUTED      UINT32_C(0x7590B8)

typedef enum gui_screen {
    GUI_HOME = 0,
    GUI_STATUS,
    GUI_FILES,
    GUI_FILE_VIEW,
    GUI_DEVICES,
    GUI_WORKBENCH,
    GUI_AUDIT,
    GUI_SYSTEM,
    GUI_SECURITY,
    GUI_UPDATE,
    GUI_ABOUT,
    GUI_SHUTDOWN,
    GUI_SHUTDOWN_RESULT,
    GUI_TERMINAL,
} gui_screen_t;

static picopen_shell_state_t gui_state;
static gui_screen_t screen;
static size_t selection;
static char canvas[GUI_SCREEN_SIZE];
static size_t canvas_length;

static void append(const char *format, ...) {
    if (canvas_length >= sizeof(canvas) - 1u) {
        return;
    }
    va_list arguments;
    va_start(arguments, format);
    const int written = vsnprintf(&canvas[canvas_length],
                                  sizeof(canvas) - canvas_length,
                                  format, arguments);
    va_end(arguments);
    if (written <= 0) {
        return;
    }
    const size_t available = sizeof(canvas) - canvas_length - 1u;
    canvas_length += (size_t)written > available ? available : (size_t)written;
}

static void begin(const char *title) {
    canvas_length = 0u;
    canvas[0] = '\0';
    append("PICOPEN %-12s SD:RO SCOPE:OFF\n", title);
    append("========================================\n");
}

static void present(void) {
    picopen_terminal_init();
    picopen_terminal_write(canvas);
    picopen_terminal_render();
}

static void item(const char *label, size_t index) {
    append(selection == index ? "> [ %-14s ]\n" : "  [ %-14s ]\n", label);
}

static void outline(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                    uint16_t thickness, uint32_t color) {
    picopen_display_fill_rect(x, y, width, thickness, color);
    picopen_display_fill_rect(x, (uint16_t)(y + height - thickness), width,
                              thickness, color);
    picopen_display_fill_rect(x, y, thickness, height, color);
    picopen_display_fill_rect((uint16_t)(x + width - thickness), y, thickness,
                              height, color);
}

static void tile_icon(size_t index, uint16_t center_x, uint16_t y,
                      uint32_t color) {
    if (index == 0u) {
        for (uint16_t bar = 0u; bar < 4u; ++bar) {
            const uint16_t height = (uint16_t)(5u + bar * 3u);
            picopen_display_fill_rect((uint16_t)(center_x - 18u + bar * 10u),
                (uint16_t)(y + 17u - height), 5u, height, color);
        }
    } else if (index == 1u) {
        outline((uint16_t)(center_x - 18u), (uint16_t)(y + 4u), 36u, 18u,
                2u, color);
        picopen_display_fill_rect((uint16_t)(center_x - 14u), y, 16u, 5u,
                                  color);
    } else if (index == 2u) {
        outline((uint16_t)(center_x - 13u), y, 26u, 22u, 2u, color);
        for (uint16_t pin = 0u; pin < 3u; ++pin) {
            picopen_display_fill_rect((uint16_t)(center_x - 18u + pin * 12u),
                                      (uint16_t)(y + 5u), 5u, 2u, color);
            picopen_display_fill_rect((uint16_t)(center_x - 18u + pin * 12u),
                                      (uint16_t)(y + 15u), 5u, 2u, color);
        }
    } else if (index == 3u) {
        outline((uint16_t)(center_x - 20u), y, 40u, 22u, 2u, color);
        picopen_display_fill_rect((uint16_t)(center_x - 14u),
                                  (uint16_t)(y + 6u), 20u, 2u, color);
        picopen_display_fill_rect((uint16_t)(center_x - 14u),
                                  (uint16_t)(y + 12u), 12u, 2u, color);
    } else if (index == 4u) {
        outline((uint16_t)(center_x - 14u), y, 28u, 22u, 2u, color);
        picopen_display_fill_rect((uint16_t)(center_x - 7u),
                                  (uint16_t)(y + 10u), 5u, 2u, color);
        picopen_display_fill_rect((uint16_t)(center_x - 2u),
                                  (uint16_t)(y + 13u), 5u, 2u, color);
        picopen_display_fill_rect((uint16_t)(center_x + 3u),
                                  (uint16_t)(y + 7u), 5u, 2u, color);
    } else {
        picopen_display_fill_rect((uint16_t)(center_x - 2u), y, 4u, 9u,
                                  color);
        outline((uint16_t)(center_x - 12u), (uint16_t)(y + 6u), 24u, 17u,
                2u, color);
    }
}

static void graphical_tile(size_t index, const char *label, uint16_t x,
                           uint16_t y) {
    static const uint32_t accents[GUI_HOME_ITEMS] = {
        GUI_COLOR_MAGENTA, GUI_COLOR_CYAN, GUI_COLOR_CYAN,
        GUI_COLOR_VIOLET, GUI_COLOR_MAGENTA, GUI_COLOR_ORANGE,
    };
    const bool focused = selection == index;
    const uint32_t accent = focused ? GUI_COLOR_CYAN : accents[index];
    picopen_display_fill_rect(x, y, 148u, 66u, GUI_COLOR_PANEL);
    outline(x, y, 148u, 66u, focused ? 3u : 2u, accent);
    tile_icon(index, (uint16_t)(x + 74u), (uint16_t)(y + 8u), accent);
    const size_t length = strlen(label);
    const uint16_t text_width = (uint16_t)(length * 12u);
    picopen_terminal_draw_text_at(
        (uint16_t)(x + (148u - text_width) / 2u), (uint16_t)(y + 39u), label,
        2u, accent, GUI_COLOR_PANEL);
}

static void render_home(void) {
    static const char *const labels[GUI_HOME_ITEMS] = {
        "STATUS", "FILES", "DEVICES", "WORKBENCH", "AUDIT", "SYSTEM",
    };
    picopen_display_fill_rect(0u, 0u, 320u, 320u, GUI_COLOR_BACKGROUND);
    picopen_display_fill_rect(0u, 0u, 320u, 44u, GUI_COLOR_HEADER);
    picopen_display_fill_rect(0u, 43u, 320u, 1u, GUI_COLOR_VIOLET);
    picopen_terminal_draw_text_at(8u, 10u, "PICOPEN", 2u,
                                  GUI_COLOR_MAGENTA, GUI_COLOR_HEADER);
    picopen_terminal_draw_text_at(176u, 8u, "SD RO", 1u, GUI_COLOR_GOLD,
                                  GUI_COLOR_HEADER);
    picopen_terminal_draw_text_at(216u, 8u, "SCOPE OFF", 1u, GUI_COLOR_CYAN,
                                  GUI_COLOR_HEADER);
    picopen_terminal_draw_text_at(216u, 23u, "LOCKED", 1u, GUI_COLOR_MUTED,
                                  GUI_COLOR_HEADER);
    for (size_t index = 0u; index < GUI_HOME_ITEMS; ++index) {
        const uint16_t x = (index & 1u) == 0u ? 8u : 164u;
        const uint16_t y = (uint16_t)(50u + (index / 2u) * 76u);
        graphical_tile(index, labels[index], x, y);
    }
    picopen_display_fill_rect(0u, 282u, 320u, 38u, GUI_COLOR_HEADER);
    picopen_display_fill_rect(0u, 282u, 320u, 1u, GUI_COLOR_VIOLET);
    picopen_terminal_draw_text_at(10u, 294u, "ARROWS MOVE", 1u,
                                  GUI_COLOR_MAGENTA, GUI_COLOR_HEADER);
    picopen_terminal_draw_text_at(116u, 294u, "ENTER SELECT", 1u,
                                  GUI_COLOR_CYAN, GUI_COLOR_HEADER);
    picopen_terminal_draw_text_at(244u, 294u, "ESC BACK", 1u,
                                  GUI_COLOR_VIOLET, GUI_COLOR_HEADER);
}

static void render_status(void) {
    begin("STATUS");
    append("\nKEYBOARD  %s\n", gui_state.keyboard_ready ? "READY" : "DOWN");
    append("SD CARD   %s\n", picopen_sd_status_name(gui_state.sd.status));
    append("FILES     %s\n", gui_state.storage_ready ? "FAT READ-ONLY" : "NONE");
    append("BATTERY   %s", gui_state.battery_ready ? "READY" : "UNKNOWN");
    if (gui_state.battery_ready) {
        append(" %u%%%s", gui_state.battery.percent,
               gui_state.battery.charging ? " CHARGING" : "");
    }
    append("\nIPC        %s\n\nESC BACK\n",
           gui_state.ipc_ready ? "V1 READY" : "SELF-TEST FAILED");
    present();
}

static void render_files(void) {
    begin("FILES");
    if (!gui_state.storage_ready || (gui_state.storage.count == 0u)) {
        append("\nNO ROOT FILES FOUND\n\nSD REMAINS READ-ONLY\n\nESC BACK\n");
        present();
        return;
    }
    append("ROOT /   READ-ONLY\n\n");
    for (size_t index = 0u; index < gui_state.storage.count; ++index) {
        const picopen_storage_entry_t *const entry =
            &gui_state.storage.entries[index];
        append(selection == index ? "> %-12s%s\n" : "  %-12s%s\n",
               entry->name, entry->directory ? "/" : "");
    }
    append("\nUP/DOWN  ENTER OPEN  ESC BACK\n");
    present();
}

static void render_file(void) {
    begin("FILE VIEW");
    const picopen_storage_entry_t *const entry =
        &gui_state.storage.entries[selection];
    append("%s  MAX 256 BYTES\n----------------------------------------\n",
           entry->name);
    uint8_t bytes[PICOPEN_STORAGE_READ_LIMIT];
    size_t bytes_read = 0u;
    bool truncated = false;
    const bool authorized = picopen_security_authorize(
        &gui_state.security, PICOPEN_CAP_STORAGE_READ, false);
    if (!authorized || entry->directory ||
        !picopen_storage_read_root_file(entry->name, bytes, sizeof(bytes),
                                        &bytes_read, &truncated)) {
        append("FILE UNAVAILABLE OR DENIED\n");
        picopen_audit_record("gui.file.read", false);
    } else {
        for (size_t index = 0u; index < bytes_read; ++index) {
            const uint8_t value = bytes[index];
            append("%c", ((value == '\n') || (value == '\r') ||
                           ((value >= ' ') && (value <= '~'))) ? value : '.');
        }
        append(truncated ? "\n-- TRUNCATED --\n" : "\n");
        picopen_audit_record("gui.file.read", true);
    }
    append("\nESC BACK\n");
    present();
}

static void render_devices(void) {
    begin("DEVICES");
    for (size_t index = 0u; index < gui_state.devices.count; ++index) {
        const picopen_device_record_t *const device =
            &gui_state.devices.records[index];
        append("%-16s %s\n", device->name,
               picopen_device_state_name(device->state));
    }
    append("\nESC BACK\n");
    present();
}

static void render_workbench(void) {
    begin("WORKBENCH");
    append("\nI2C1  KEYBOARD CLAIMED\nSPI0  SD READ-ONLY\n"
           "SPI1  DISPLAY CLAIMED\n\nGPIO READ       NOT EXPOSED\n"
           "GPIO DRIVE      DENIED\nTRANSMIT        DENIED\n\nESC BACK\n");
    present();
}

static void render_audit(void) {
    begin("AUDIT");
    picopen_audit_record_t record;
    if (!picopen_audit_latest(&record)) {
        append("\nNO AUDIT RECORDS\n");
    } else {
        append("\nRECORDS   %u\nSEQUENCE  %llu\nACTION    %s\nRESULT    %s\n",
               (unsigned int)picopen_audit_count(), record.sequence,
               record.action, record.allowed ? "ALLOW" : "DENY");
    }
    append("\nMEMORY ONLY - NO SECRET PAYLOADS\n\nESC BACK\n");
    present();
}

static void render_system(void) {
    static const char *const labels[GUI_SYSTEM_ITEMS] = {
        "SECURITY", "WIFI UPDATE", "TERMINAL", "POWER", "ABOUT",
    };
    begin("SYSTEM");
    append("\n");
    for (size_t index = 0u; index < GUI_SYSTEM_ITEMS; ++index) {
        item(labels[index], index);
    }
    append("\nUP/DOWN  ENTER SELECT  ESC BACK\n");
    present();
}

static void render_security(void) {
    begin("SECURITY");
    append("\nSECURE DEFAULT       ACTIVE\nENGAGEMENT SCOPE     INACTIVE\n"
           "SD ACCESS           READ-ONLY\nAUTO-RUN            DENIED\n"
           "RADIO TRANSMIT      DENIED\nGPIO OUTPUT         DENIED\n"
           "USB HID            DENIED\nREMOTE CONTROL      DENIED\n\nESC BACK\n");
    present();
}

static void render_update(void) {
    begin("WIFI UPDATE");
    append("\n        NOT CONFIGURED\n\nWIFI REMAINS DISABLED BY POLICY\n\n"
           "REQUIRES:\n- SIGNATURE POLICY\n- INACTIVE UPDATE SLOT\n"
           "- BOOTLOADER ROLLBACK CONTRACT\n- LOCAL APPROVAL\n\n"
           "UNSIGNED UPDATES: DENIED\n\nESC BACK\n");
    present();
}

static void render_about(void) {
    begin("ABOUT");
    append("\nPICOPEN OS 0.0.1\nTARGET PICO 2 W / CPI 2.0\n"
           "AUTHORIZED SECURITY RESEARCH\nSECURE-BY-DEFAULT DEVELOPMENT BUILD\n\n"
           "BOOT RECOVERY REMAINS AVAILABLE\n\nESC BACK\n");
    present();
}

static void render_shutdown(void) {
    begin("POWER");
    append("\n\n      POWER OFF PICOCALC?\n\n"
           "UNSAVED VOLATILE STATE WILL BE LOST\nSD CARD IS READ-ONLY\n\n");
    append(selection == 0u ? "       > [ CANCEL ]\n\n" : "         [ CANCEL ]\n\n");
    append(selection == 1u ? "       > [ POWER OFF ]\n" : "         [ POWER OFF ]\n");
    append("\nNO TIME LIMIT  ENTER SELECT  ESC CANCEL\n");
    present();
}

static void render_shutdown_result(bool accepted) {
    begin("POWER");
    append(accepted ? "\nSHUTDOWN CONFIRMED\nPOWER OFF IN ABOUT 6 SECONDS\n"
                    : "\nSHUTDOWN CONTROLLER ERROR\nPOWER REMAINS ON\n");
    append("\nESC BACK\n");
    present();
}

static void render(void) {
    switch (screen) {
        case GUI_HOME: render_home(); break;
        case GUI_STATUS: render_status(); break;
        case GUI_FILES: render_files(); break;
        case GUI_FILE_VIEW: render_file(); break;
        case GUI_DEVICES: render_devices(); break;
        case GUI_WORKBENCH: render_workbench(); break;
        case GUI_AUDIT: render_audit(); break;
        case GUI_SYSTEM: render_system(); break;
        case GUI_SECURITY: render_security(); break;
        case GUI_UPDATE: render_update(); break;
        case GUI_ABOUT: render_about(); break;
        case GUI_SHUTDOWN: render_shutdown(); break;
        case GUI_SHUTDOWN_RESULT: break;
        case GUI_TERMINAL: break;
    }
}

static void open_home_item(void) {
    static const gui_screen_t screens[GUI_HOME_ITEMS] = {
        GUI_STATUS, GUI_FILES, GUI_DEVICES, GUI_WORKBENCH, GUI_AUDIT, GUI_SYSTEM,
    };
    screen = screens[selection];
    selection = 0u;
    render();
}

static void open_system_item(void) {
    static const gui_screen_t screens[GUI_SYSTEM_ITEMS] = {
        GUI_SECURITY, GUI_UPDATE, GUI_TERMINAL, GUI_SHUTDOWN, GUI_ABOUT,
    };
    screen = screens[selection];
    selection = 0u;
    if (screen == GUI_TERMINAL) {
        picopen_terminal_init();
        picopen_terminal_write("PICOPEN ADVANCED TERMINAL\nESC RETURNS TO GUI\n\n> ");
        picopen_terminal_render();
        picopen_shell_init(&gui_state);
        return;
    }
    render();
}

static gui_screen_t parent_screen(gui_screen_t current) {
    if (current == GUI_FILE_VIEW) {
        return GUI_FILES;
    }
    if ((current == GUI_SECURITY) || (current == GUI_UPDATE) ||
        (current == GUI_ABOUT) || (current == GUI_SHUTDOWN) ||
        (current == GUI_SHUTDOWN_RESULT)) {
        return GUI_SYSTEM;
    }
    return GUI_HOME;
}

void picopen_gui_init(const picopen_shell_state_t *state) {
    if (state != NULL) {
        gui_state = *state;
    }
    screen = GUI_HOME;
    selection = 0u;
    picopen_audit_record("gui.start", true);
    render();
}

void picopen_gui_handle_key(uint8_t key) {
    if (screen == GUI_TERMINAL) {
        if (key == PICOPEN_KEY_ESCAPE) {
            screen = GUI_SYSTEM;
            selection = 2u;
            render();
        } else {
            picopen_shell_handle_key(key);
        }
        return;
    }
    if (key == PICOPEN_KEY_ESCAPE) {
        screen = parent_screen(screen);
        if ((screen == GUI_FILES) && (selection >= gui_state.storage.count)) {
            selection = 0u;
        } else if (screen != GUI_FILES) {
            selection = 0u;
        }
        render();
        return;
    }
    if (screen == GUI_HOME) {
        if ((key == PICOPEN_KEY_LEFT) && ((selection & 1u) != 0u)) --selection;
        if ((key == PICOPEN_KEY_RIGHT) && ((selection & 1u) == 0u)) ++selection;
        if ((key == PICOPEN_KEY_UP) && (selection >= 2u)) selection -= 2u;
        if ((key == PICOPEN_KEY_DOWN) && (selection + 2u < GUI_HOME_ITEMS)) selection += 2u;
        if (key == PICOPEN_KEY_ENTER) { open_home_item(); return; }
        render_home();
        return;
    }
    if (screen == GUI_FILES) {
        const size_t count = gui_state.storage.count;
        if ((key == PICOPEN_KEY_UP) && (selection > 0u)) --selection;
        if ((key == PICOPEN_KEY_DOWN) && (selection + 1u < count)) ++selection;
        if ((key == PICOPEN_KEY_ENTER) && (count != 0u)) {
            screen = GUI_FILE_VIEW;
            render_file();
            return;
        }
        render_files();
        return;
    }
    if (screen == GUI_SYSTEM) {
        if ((key == PICOPEN_KEY_UP) && (selection > 0u)) --selection;
        if ((key == PICOPEN_KEY_DOWN) && (selection + 1u < GUI_SYSTEM_ITEMS)) ++selection;
        if (key == PICOPEN_KEY_ENTER) { open_system_item(); return; }
        render_system();
        return;
    }
    if (screen == GUI_SHUTDOWN) {
        if ((key == PICOPEN_KEY_UP) || (key == PICOPEN_KEY_DOWN)) selection ^= 1u;
        if (key == PICOPEN_KEY_ENTER) {
            if (selection == 0u) {
                screen = GUI_SYSTEM;
                selection = 3u;
                render_system();
            } else {
                const bool accepted = picopen_security_authorize(
                    &gui_state.security, PICOPEN_CAP_SYSTEM_SHUTDOWN, true) &&
                    picopen_keyboard_request_shutdown(6u);
                picopen_audit_record("gui.shutdown", accepted);
                screen = GUI_SHUTDOWN_RESULT;
                render_shutdown_result(accepted);
            }
            return;
        }
        render_shutdown();
    }
}
