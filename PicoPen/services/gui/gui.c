#include "picopen/gui.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"

#include "picopen/audit.h"
#include "picopen/apps.h"
#include "picopen/crayon_renderer.h"
#include "picopen/display.h"
#include "picopen/keyboard.h"
#include "picopen/internal_fs.h"
#include "picopen/preferences.h"
#include "picopen/recovery.h"
#include "picopen/recon.h"
#include "picopen/evidence.h"
#include "picopen/skin.h"
#include "picopen/storage.h"
#include "picopen/synthwave_renderer.h"
#include "picopen/terminal.h"
#include "picopen/workbench.h"
#include "picopen/wifi.h"
#include "picopen/wifi_vault.h"

#define GUI_SCREEN_SIZE 1024u
#define GUI_HOME_ITEMS 6u
#define GUI_SYSTEM_ITEMS 7u
#define GUI_SECURITY_ITEMS 5u
#define GUI_WIFI_ITEMS 9u
#define GUI_APPS_VISIBLE_ITEMS 9u

#define GUI_COLOR_BACKGROUND UINT32_C(0x080020)
#define GUI_COLOR_PANEL      UINT32_C(0x101040)
#define GUI_COLOR_HEADER     UINT32_C(0x12002F)
#define GUI_COLOR_CYAN       UINT32_C(0x00E5FF)
#define GUI_COLOR_MAGENTA    UINT32_C(0xFF2BD6)
#define GUI_COLOR_VIOLET     UINT32_C(0x8B5CF6)
#define GUI_COLOR_ORANGE     UINT32_C(0xFF8A30)
#define GUI_COLOR_GOLD       UINT32_C(0xFFCC00)
#define GUI_COLOR_FOCUS      UINT32_C(0x7CFF00)
#define GUI_COLOR_MUTED      UINT32_C(0x7590B8)

typedef enum gui_screen {
    GUI_HOME = 0,
    GUI_STATUS,
    GUI_FILES,
    GUI_FILE_VIEW,
    GUI_DEVICES,
    GUI_APPS,
    GUI_APP_DEVICES,
    GUI_APP_CONFIG,
    GUI_RECON_CONFIRM,
    GUI_RECON,
    GUI_RECON_HISTORY,
    GUI_EVIDENCE_PICKER,
    GUI_EVIDENCE,
    GUI_AUDIT,
    GUI_SYSTEM,
    GUI_SECURITY,
    GUI_UPDATE,
    GUI_SKINS,
    GUI_ABOUT,
    GUI_RECOVERY,
    GUI_SHUTDOWN,
    GUI_SHUTDOWN_RESULT,
    GUI_TERMINAL,
} gui_screen_t;

static picopen_shell_state_t gui_state;
static gui_screen_t screen;
static size_t selection;
static size_t home_selection;
static size_t system_selection;
static size_t workbench_selection;
static size_t security_selection;
static size_t wifi_selection;
static size_t pending_recon_selection;
static picopen_app_catalog_t app_catalog;
static char task_target[PICOPEN_RECON_TARGET_SIZE];
static size_t task_target_length;
static uint16_t task_port = 80u;
static size_t task_field;
static char task_error[48];
static picopen_security_mode_t security_mode;
static size_t files_selection;
static picopen_storage_listing_t file_listing;
static char file_path[PICOPEN_STORAGE_PATH_SIZE];
static char engagement_reference[PICOPEN_ENGAGEMENT_REFERENCE_SIZE];
static size_t engagement_reference_length;
static size_t engagement_duration_index;
static char engagement_target[PICOPEN_ENGAGEMENT_TARGET_SIZE];
static size_t engagement_target_length;
static char wifi_ssid[PICOPEN_WIFI_SSID_SIZE];
static size_t wifi_ssid_length;
static char wifi_password[64];
static size_t wifi_password_length;
static char wifi_pin[PICOPEN_VAULT_PIN_SIZE];
static size_t wifi_pin_length;
static size_t wifi_ap_selection;
static picopen_vault_result_t wifi_vault_result = PICOPEN_VAULT_EMPTY;
static picopen_gui_storage_action_t storage_action;
static char canvas[GUI_SCREEN_SIZE];
static char canvas_title[20];
static size_t canvas_length;
static char rendered_lines[20][41];
static bool rendered_page_valid;
static gui_screen_t rendered_page_screen;
static picopen_skin_id_t rendered_page_skin;

static void outline(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                    uint16_t thickness, uint32_t color);

static void scrub_secret(char *value, size_t size) {
    volatile char *cursor = value;
    while (size-- > 0u) *cursor++ = 0;
}

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
    snprintf(canvas_title, sizeof(canvas_title), "%s", title);
    append("PICOPEN %-12s SD:RO SCOPE:%s\n", title,
           picopen_engagement_session_active(time_us_64() / 1000u)
               ? "ON" : "OFF");
    append("========================================\n");
}

static void draw_page_texture(const picopen_skin_t *skin, uint16_t y,
                              uint16_t height) {
    picopen_display_fill_rect(0u, y, 320u, height, skin->background);
    const uint16_t end = (uint16_t)(y + height);
    if (skin->style == PICOPEN_SKIN_STYLE_NEON) {
        for (uint16_t line_y = 48u; line_y < 288u; line_y += 24u) {
            if ((line_y >= y) && (line_y < end)) {
                picopen_display_fill_rect(0u, line_y, 320u, 1u, 0x240046u);
            }
        }
        for (uint16_t x = 0u; x < 320u; x += 40u) {
            picopen_display_fill_rect(x, y, 1u, height, 0x180036u);
        }
    } else if (skin->style == PICOPEN_SKIN_STYLE_CRAYON) {
        uint16_t line_y = (uint16_t)(y + ((13u - (y % 13u)) % 13u));
        for (; line_y < end; line_y += 13u) {
            for (uint16_t x = (uint16_t)(line_y % 9u); x < 320u; x += 17u) {
                picopen_display_fill_rect(x, line_y, 1u, 1u, 0xD9C79Fu);
            }
        }
    }
}

static void draw_page_line(const picopen_skin_t *skin, uint16_t row,
                           const char *line) {
    const size_t length = strlen(line);
    const uint16_t y = (uint16_t)(row * 16u);
    draw_page_texture(skin, y, 16u);
    uint32_t background = skin->background;
    if (row == 0u) {
        picopen_display_fill_rect(0u, 0u, 320u, 16u, skin->header);
        background = skin->header;
    } else if ((length > 0u) && (line[0] == '>')) {
        picopen_display_fill_rect(2u, y, 316u, 16u, skin->panel);
        if (skin->textured_focus) {
            for (uint16_t x = 4u; x < 316u; x += 7u) {
                const uint16_t sy = (uint16_t)(y + 2u + ((x / 7u) % 9u));
                picopen_display_fill_rect(x, sy, 9u, 2u, skin->focus);
            }
        }
        outline(2u, y, 316u, 16u, 2u, skin->focus);
        background = skin->panel;
    } else if ((length >= 8u) && (line[0] == '=')) {
        picopen_display_fill_rect(0u, (uint16_t)(y + 7u), 320u, 2u,
                                  skin->accents[3]);
        return;
    }
    const picopen_text_style_t text_style =
        skin->style == PICOPEN_SKIN_STYLE_CRAYON ? PICOPEN_TEXT_CRAYON :
        (skin->style == PICOPEN_SKIN_STYLE_NEON ? PICOPEN_TEXT_NEON
                                                : PICOPEN_TEXT_PIXEL);
    picopen_terminal_draw_styled_text_at(4u, (uint16_t)(y + 3u), line, 1u,
        skin->text, skin->style == PICOPEN_SKIN_STYLE_CRAYON
            ? skin->accents[row % 6u] : skin->accents[3],
        background, text_style);
}

static void present(void) {
    const picopen_skin_t *const skin = picopen_skin_current();
    char lines[20][41] = {{0}};
    size_t offset = 0u;
    for (uint16_t row = 0u; (row < 20u) && (canvas[offset] != '\0'); ++row) {
        size_t length = 0u;
        while ((canvas[offset] != '\0') && (canvas[offset] != '\n')) {
            if (length < sizeof(lines[row]) - 1u) {
                lines[row][length++] = canvas[offset];
            }
            ++offset;
        }
        if (canvas[offset] == '\n') {
            ++offset;
        }
        lines[row][length] = '\0';
    }
    if (picopen_skin_current_id() == PICOPEN_SKIN_CRAYON) {
        picopen_crayon_renderer_page((uint8_t)screen, canvas_title, lines);
        rendered_page_valid = false;
        return;
    }
    if (picopen_skin_current_id() == PICOPEN_SKIN_SYNTHWAVE) {
        picopen_synthwave_renderer_page((uint8_t)screen, lines);
        rendered_page_valid = false;
        return;
    }
    const bool full_redraw = !rendered_page_valid ||
        (rendered_page_screen != screen) ||
        (rendered_page_skin != picopen_skin_current_id());
    if (full_redraw) {
        draw_page_texture(skin, 0u, 320u);
    }
    for (uint16_t row = 0u; row < 20u; ++row) {
        if (full_redraw || (strcmp(lines[row], rendered_lines[row]) != 0)) {
            draw_page_line(skin, row, lines[row]);
            strcpy(rendered_lines[row], lines[row]);
        }
    }
    rendered_page_valid = true;
    rendered_page_screen = screen;
    rendered_page_skin = picopen_skin_current_id();
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

static void draw_skin_text(uint16_t x, uint16_t y, const char *text,
                           uint8_t scale, uint32_t foreground,
                           uint32_t background) {
    const picopen_skin_t *const skin = picopen_skin_current();
    const picopen_text_style_t style =
        skin->style == PICOPEN_SKIN_STYLE_CRAYON ? PICOPEN_TEXT_CRAYON :
        (skin->style == PICOPEN_SKIN_STYLE_NEON ? PICOPEN_TEXT_NEON
                                                : PICOPEN_TEXT_PIXEL);
    picopen_terminal_draw_styled_text_at(x, y, text, scale, foreground,
                                         skin->accents[3], background, style);
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
                           uint16_t y, bool focused) {
    const picopen_skin_t *const skin = picopen_skin_current();
    const uint32_t accent = focused ? skin->focus : skin->accents[index];
    picopen_display_fill_rect(x, y, 148u, 66u, skin->panel);
    if (focused && skin->textured_focus) {
        for (uint16_t stroke = 0u; stroke < 7u; ++stroke) {
            for (uint16_t step = 0u; step < 17u; ++step) {
                const uint16_t sx = (uint16_t)(x + 5u + step * 8u);
                const uint16_t sy = (uint16_t)(y + 5u + stroke * 8u +
                    ((step + stroke) % 4u));
                picopen_display_fill_rect(sx, sy, 10u, 2u, skin->focus);
            }
        }
    }
    if (skin->style == PICOPEN_SKIN_STYLE_CRAYON) {
        outline((uint16_t)(x + 1u), y, 146u, 66u, 2u, accent);
        picopen_display_fill_rect((uint16_t)(x + 5u), (uint16_t)(y + 3u),
                                  55u, 1u, skin->accents[index]);
        picopen_display_fill_rect((uint16_t)(x + 78u),
                                  (uint16_t)(y + 62u), 62u, 1u,
                                  skin->accents[index]);
    } else {
        outline(x, y, 148u, 66u, focused ? 3u : 2u, accent);
    }
    if (skin->style == PICOPEN_SKIN_STYLE_NEON) {
        outline((uint16_t)(x + 4u), (uint16_t)(y + 4u), 140u, 58u, 1u,
                focused ? skin->accents[0] : 0x240046u);
    }
    const bool crayon = skin->style == PICOPEN_SKIN_STYLE_CRAYON;
    tile_icon(index, (uint16_t)(x + (crayon ? 32u : 74u)),
              (uint16_t)(y + (crayon ? 21u : 8u)),
              crayon ? skin->accents[index] : accent);
    const size_t length = strlen(label);
    const uint8_t text_scale = crayon ? 1u : 2u;
    const uint16_t text_width = crayon ? (uint16_t)(length * 8u)
                                       : (uint16_t)(length * 12u);
    const uint16_t text_x = crayon
        ? (uint16_t)(x + 61u + ((83u - text_width) / 2u))
        : (uint16_t)(x + ((148u - text_width) / 2u));
    const uint16_t text_y = crayon
        ? (uint16_t)(y + 25u)
        : (uint16_t)(y + 39u);
    draw_skin_text(
        text_x, text_y, label, text_scale, skin->text, skin->panel);
}

static void render_home(void) {
    static const char *const labels[GUI_HOME_ITEMS] = {
        "STATUS", "FILES", "DEVICES", "APPS", "AUDIT", "SYSTEM",
    };
    const bool scope_active = picopen_engagement_session_active(
        time_us_64() / 1000u);
    if (picopen_skin_current_id() == PICOPEN_SKIN_CRAYON) {
        picopen_crayon_renderer_home(labels, GUI_HOME_ITEMS, selection,
                                     scope_active);
        rendered_page_valid = false;
        return;
    }
    if (picopen_skin_current_id() == PICOPEN_SKIN_SYNTHWAVE) {
        picopen_synthwave_renderer_home(labels, GUI_HOME_ITEMS, selection,
                                        scope_active);
        rendered_page_valid = false;
        return;
    }
    const picopen_skin_t *const skin = picopen_skin_current();
    rendered_page_valid = false;
    picopen_display_fill_rect(0u, 0u, 320u, 320u, skin->background);
    if (skin->style == PICOPEN_SKIN_STYLE_NEON) {
        for (uint16_t y = 48u; y < 282u; y += 24u) {
            picopen_display_fill_rect(0u, y, 320u, 1u, 0x240046u);
        }
        for (uint16_t x = 0u; x < 320u; x += 40u) {
            picopen_display_fill_rect(x, 44u, 1u, 238u, 0x180036u);
        }
    } else if (skin->style == PICOPEN_SKIN_STYLE_CRAYON) {
        for (uint16_t y = 3u; y < 282u; y += 13u) {
            for (uint16_t x = (uint16_t)(y % 9u); x < 320u; x += 17u) {
                picopen_display_fill_rect(x, y, 1u, 1u, 0xD9C79Fu);
            }
        }
    }
    picopen_display_fill_rect(0u, 0u, 320u, 44u, skin->header);
    picopen_display_fill_rect(0u, 43u, 320u, 1u, skin->accents[3]);
    draw_skin_text(8u, 10u, "PICOPEN", 2u, skin->text, skin->header);
    draw_skin_text(176u, 8u, "SD RO", 1u, skin->accents[5], skin->header);
    draw_skin_text(216u, 8u,
                   scope_active ? "SCOPE ON" : "SCOPE OFF",
                   1u, skin->accents[1], skin->header);
    draw_skin_text(216u, 23u, "LOCKED", 1u, skin->muted, skin->header);
    for (size_t index = 0u; index < GUI_HOME_ITEMS; ++index) {
        const uint16_t x = (index & 1u) == 0u ? 8u : 164u;
        const uint16_t y = (uint16_t)(50u + (index / 2u) * 76u);
        graphical_tile(index, labels[index], x, y, selection == index);
    }
    picopen_display_fill_rect(0u, 282u, 320u, 38u, skin->header);
    picopen_display_fill_rect(0u, 282u, 320u, 1u, skin->accents[3]);
    draw_skin_text(10u, 294u, "ARROWS MOVE", 1u, skin->text, skin->header);
    draw_skin_text(116u, 294u, "ENTER SELECT", 1u, skin->text, skin->header);
    draw_skin_text(244u, 294u, "ESC BACK", 1u, skin->text, skin->header);
}

static void redraw_home_focus(size_t previous, size_t current) {
    static const char *const labels[GUI_HOME_ITEMS] = {
        "STATUS", "FILES", "DEVICES", "APPS", "AUDIT", "SYSTEM",
    };
    if (picopen_skin_current_id() == PICOPEN_SKIN_CRAYON) {
        picopen_crayon_renderer_home_focus(labels, previous, current);
        return;
    }
    if (picopen_skin_current_id() == PICOPEN_SKIN_SYNTHWAVE) {
        picopen_synthwave_renderer_home_focus(labels, previous, current);
        return;
    }
    const uint16_t previous_x = (previous & 1u) == 0u ? 8u : 164u;
    const uint16_t previous_y = (uint16_t)(50u + (previous / 2u) * 76u);
    const uint16_t current_x = (current & 1u) == 0u ? 8u : 164u;
    const uint16_t current_y = (uint16_t)(50u + (current / 2u) * 76u);
    graphical_tile(previous, labels[previous], previous_x, previous_y, false);
    graphical_tile(current, labels[current], current_x, current_y, true);
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
    if (!gui_state.storage_ready || (file_listing.count == 0u)) {
        append("\nNO FILES FOUND\n\nPATH %.30s\nSD REMAINS READ-ONLY\n\nESC BACK\n",
               file_path);
        present();
        return;
    }
    append("%.28s   READ-ONLY\n\n", file_path);
    for (size_t index = 0u; index < file_listing.count; ++index) {
        const picopen_storage_entry_t *const entry =
            &file_listing.entries[index];
        append(selection == index ? "> %-24.24s%s\n" : "  %-24.24s%s\n",
               entry->name, entry->directory ? "/" : "");
    }
    append("\nUP/DOWN ENTER OPEN S SAFE REMOVE\nR RESCAN  ESC BACK\n");
    present();
}

static void render_file(void) {
    begin("FILE VIEW");
    const picopen_storage_entry_t *const entry =
        &file_listing.entries[selection];
    char path[PICOPEN_STORAGE_PATH_SIZE];
    const int path_length = snprintf(path, sizeof(path),
        strcmp(file_path, "/") == 0 ? "/%s" : "%s/%s", file_path,
        entry->name);
    append("%.24s  %lu B\n----------------------------------------\n",
           entry->name, (unsigned long)entry->size);
    uint8_t bytes[PICOPEN_STORAGE_READ_LIMIT];
    size_t bytes_read = 0u;
    bool truncated = false;
    const bool authorized = picopen_security_authorize(
        &gui_state.security, PICOPEN_CAP_STORAGE_READ, false);
    if (!authorized || entry->directory ||
        (path_length <= 0) || ((size_t)path_length >= sizeof(path)) ||
        (picopen_storage_read_file(&gui_state.storage_service, path, 0u,
             bytes, sizeof(bytes), &bytes_read, &truncated) !=
         PICOPEN_STORAGE_OK)) {
        append("FILE UNAVAILABLE OR DENIED\n");
        picopen_audit_record("gui.file.read", false);
    } else {
        size_t text_bytes = 0u;
        for (size_t index = 0u; index < bytes_read; ++index) {
            const uint8_t value = bytes[index];
            if ((value == '\n') || (value == '\r') || (value == '\t') ||
                ((value >= ' ') && (value <= '~'))) {
                ++text_bytes;
            }
        }
        const bool text_view = (bytes_read == 0u) ||
            ((text_bytes * 100u) / bytes_read >= 85u);
        if (text_view) {
            append("TEXT\n");
            for (size_t index = 0u; index < bytes_read; ++index) {
                const uint8_t value = bytes[index];
                append("%c", ((value == '\n') || (value == '\r') ||
                               ((value >= ' ') && (value <= '~'))) ? value : '.');
            }
        } else {
            append("HEX (FIRST 96 BYTES)\n");
            const size_t shown = bytes_read < 96u ? bytes_read : 96u;
            for (size_t index = 0u; index < shown; index += 8u) {
                append("%04X ", (unsigned int)index);
                for (size_t column = 0u;
                     (column < 8u) && (index + column < shown); ++column) {
                    append("%02X ", bytes[index + column]);
                }
                append("\n");
            }
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
    begin("APPS");
    append("MODE %s  BOUNDARY %s\n\n",
        security_mode==PICOPEN_SECURITY_OWNER?"OWNER":security_mode==PICOPEN_SECURITY_GUARDED?"GUARDED":"DEVELOPER",
        picopen_engagement_session_active(time_us_64()/1000u)?"ACTIVE":"OPTIONAL");
    const size_t shown=app_catalog.count<GUI_APPS_VISIBLE_ITEMS?app_catalog.count:GUI_APPS_VISIBLE_ITEMS;
    for(size_t index=0u;index<shown;++index){
        const picopen_app_descriptor_t *app=&app_catalog.apps[index];
        append(selection==index?"> %-22.22s%s\n":"  %-22.22s%s\n",app->name,
               app->built_in?"":" SD");
    }
    append("\nENTER OPEN  ESC BACK\nSD APPS: /PicoPen/apps\n");
    present();
}

static picopen_recon_kind_t recon_kind_for_selection(size_t index) {
    const picopen_app_kind_t kind=app_catalog.apps[index].kind;
    if(kind==PICOPEN_APP_HTTP_INSPECTOR)return PICOPEN_RECON_HTTP_HEAD;
    if(kind==PICOPEN_APP_SSH_BANNER)return PICOPEN_RECON_SSH_BANNER;
    if(kind==PICOPEN_APP_TLS_INSPECTOR)return PICOPEN_RECON_TLS_METADATA;
    return PICOPEN_RECON_TCP;
}

static void render_app_config(void){
    const picopen_app_descriptor_t *app=&app_catalog.apps[pending_recon_selection];
    begin("APP TASK");
    if(app->kind==PICOPEN_APP_NETWORK_DISCOVERY){
        picopen_wifi_status_t wifi;picopen_wifi_get_status(&wifi);
        append("\nNETWORK DISCOVERY\nPASSIVE LOCAL VIEW\n\nIP %s\nGATEWAY %s\nDNS %s\nAPS OBSERVED %u%s\n\nACTIVE DISCOVERY NOT YET ENABLED\nESC BACK\n",
            wifi.ipv4[0]?wifi.ipv4:"-",wifi.gateway[0]?wifi.gateway:"-",
            wifi.dns[0]?wifi.dns:"-",(unsigned)wifi.ap_count,wifi.ap_truncated?"+":"");
        present();return;
    }
    if(app->kind==PICOPEN_APP_SESSION_REPORTS){
        append("\nSESSION REPORTS\n\nRESULTS %u\nAUDIT RECORDS %u\nBOUNDARY %s\n\nEXPORT UNAVAILABLE: SD IS READ-ONLY\nSESSION REMAINS IN MEMORY\nESC BACK\n",
            (unsigned)picopen_recon_history_count(),(unsigned)picopen_audit_count(),
            picopen_engagement_session_active(time_us_64()/1000u)?"ACTIVE":"INACTIVE");
        present();return;
    }
    if(app->kind==PICOPEN_APP_SD_PACKAGE){
        append("\n%.23s\n\nPACKAGE DISCOVERED ON SD\nRUNTIME/MANIFEST VALIDATOR PENDING\nNOT EXECUTED\n\nESC BACK\n",app->name);
        present();return;
    }
    append("\n%s\n\n",app->name);
    append(task_field==0u?"> TARGET %.28s\n":"  TARGET %.28s\n",task_target_length?task_target:"<REQUIRED>");
    append(task_field==1u?"> PORT %u\n":"  PORT %u\n",task_port);
    if (task_error[0]) append("\n%s\n", task_error);
    append("\nLEFT/RIGHT PORT  ENTER CONTINUE\nESC BACK\n");present();
}

static void render_recon_confirm(void) {
    picopen_engagement_t scope;
    picopen_engagement_session_snapshot(&scope);
    const picopen_recon_kind_t kind = recon_kind_for_selection(
        pending_recon_selection);
    begin("CONFIRM ACTION");
    append("\nOP       %s\nTARGET   %.30s\nPORT     %u\nLIMIT    %.28s\nSESSION  %s\n\n",
           picopen_recon_kind_name(kind), task_target, task_port,
           scope.boundary_configured ? scope.target : "ANY TASK TARGET",
           picopen_engagement_session_active(time_us_64() / 1000u)
               ? "ACTIVE" : "INACTIVE");
    append("ONE REQUEST, 7 SECOND DEADLINE\nNO CREDENTIALS OR EXPLOIT PAYLOAD\n\nENTER CONFIRM  ESC CANCEL\n");
    present();
}

static void render_recon_history(void) {
    begin("RECENT RESULTS");
    const size_t count = picopen_recon_history_count();
    if (count == 0u) append("\nNO NETWORK RESULTS THIS BOOT\n");
    for (size_t index = 0u; index < count; ++index) {
        picopen_recon_snapshot_t entry;
        if (picopen_recon_history_get(index, &entry)) {
            append("%-9s %-10s %.14s:%u\n",
                   picopen_recon_kind_name(entry.kind),
                   picopen_recon_state_name(entry.state), entry.target,
                   entry.port);
        }
    }
    append("\nVOLATILE / SANITIZED\nESC BACK\n");
    present();
}

static void render_evidence_picker(void) {
    begin("EVIDENCE PICKER");
    if (!gui_state.storage_ready || file_listing.count == 0u) {
        append("\nNO FILES AVAILABLE\nUSE FILES OR RESCAN SD\n\nESC BACK\n");
        present();
        return;
    }
    append("%.26s  READ-ONLY\n\n", file_path);
    for (size_t index = 0u; index < file_listing.count; ++index) {
        const picopen_storage_entry_t *entry = &file_listing.entries[index];
        append(selection == index ? "> %-23.23s%s\n" : "  %-23.23s%s\n",
               entry->name, entry->directory ? "/" : "");
    }
    append("\nENTER ANALYZE  ESC BACK\n");
    present();
}

static void render_recon(void) {
    begin("NETWORK RECON"); picopen_recon_snapshot_t snapshot; picopen_recon_snapshot(&snapshot);
    append("\nOP %s\nSTATE %s\nTARGET %.30s\nADDRESS %s\nPORT %u  SERVICE %s\nELAPSED %lu MS  RX %lu\nDETAIL %.80s\nRESULT %d\n",
        picopen_recon_kind_name(snapshot.kind),
        picopen_recon_state_name(snapshot.state),snapshot.target,
        snapshot.address[0]?snapshot.address:"-",snapshot.port,snapshot.service,
        (unsigned long)snapshot.elapsed_ms,(unsigned long)snapshot.bytes_received,
        snapshot.detail[0]?snapshot.detail:"-",snapshot.result);
    append("\nONE REQUEST RATE LIMITED\nESC CANCEL/BACK\n");present();
}

static void render_evidence(void){
    begin("EVIDENCE");picopen_evidence_snapshot_t snapshot;picopen_evidence_snapshot(&snapshot);
    append("\nSTATE %s\nFILE %.30s\nSIZE %lu  PROCESSED %lu\nSHA256\n%.32s\n%.32s\nSTRINGS %lu  %.38s\nCAPTURE %s  PACKETS %lu\nIP4:%lu IP6:%lu ARP:%lu\nTCP:%lu UDP:%lu ICMP:%lu\nRESULT %d\n",
        picopen_evidence_state_name(snapshot.state),snapshot.path,(unsigned long)snapshot.size,
        (unsigned long)snapshot.processed,snapshot.sha256,&snapshot.sha256[32],
        (unsigned long)snapshot.string_count,snapshot.string_preview,
        snapshot.capture==PICOPEN_CAPTURE_PCAP?"PCAP":snapshot.capture==PICOPEN_CAPTURE_PCAPNG?"PCAPNG":"NONE",
        (unsigned long)snapshot.packet_count,(unsigned long)snapshot.ipv4_count,
        (unsigned long)snapshot.ipv6_count,(unsigned long)snapshot.arp_count,
        (unsigned long)snapshot.tcp_count,(unsigned long)snapshot.udp_count,
        (unsigned long)snapshot.icmp_count,snapshot.result);
    append("\nREAD ONLY  MAX 256 KIB\nESC CANCEL/BACK\n");present();
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
        "SECURITY", "WIFI UPDATE", "SKINS", "TERMINAL", "RECOVERY", "POWER", "ABOUT",
    };
    begin("SYSTEM");
    append("\n");
    for (size_t index = 0u; index < GUI_SYSTEM_ITEMS; ++index) {
        item(labels[index], index);
    }
    append("\nUP/DOWN  ENTER SELECT  ESC BACK\n");
    present();
}

static void render_recovery(void) {
    begin("RECOVERY");
    picopen_crash_record_t crash;
    append("\nINTERNAL STORE %s\n",
        picopen_internal_fs_state()==PICOPEN_INTERNAL_FS_READY ? "READY" :
        picopen_internal_fs_state()==PICOPEN_INTERNAL_FS_UNINITIALIZED ? "UNINITIALIZED" : "ERROR");
    append("FLASH BLOCKS %u/%u USED\n",
           (unsigned int)picopen_internal_fs_used_blocks(),
           (unsigned int)picopen_internal_fs_total_blocks());
    if (picopen_recovery_get(&crash))
        append("CRASH RECORDS %lu\nLAST REASON 0X%08lX\n",(unsigned long)crash.count,(unsigned long)crash.reason);
    else append("CRASH RECORDS NONE\n");
    append("SD STATE %d GEN %lu\n\n",(int)gui_state.storage_service.media_state,
           (unsigned long)gui_state.storage_service.media_generation);
    append(selection==0u ? "> CLEAR CRASH RECORD\n" : "  CLEAR CRASH RECORD\n");
    append("\nENTER SELECT  ESC BACK\n"); present();
}

static void render_skins(void) {
    begin("SKINS");
    append("FACTORY DEFAULT: SYNTHWAVE\n\n");
    for (size_t index = 0u; index < PICOPEN_SKIN_COUNT; ++index) {
        const picopen_skin_t *const skin = picopen_skin_get((picopen_skin_id_t)index);
        append(selection == index ? "> %-16s%s\n" : "  %-16s%s\n",
               skin->name,
               picopen_skin_current_id() == (picopen_skin_id_t)index ? " ACTIVE" : "");
    }
    append("\nENTER SET SESSION DEFAULT\nPERSISTENCE: NOT YET ENABLED\nESC BACK\n");
    present();
}

static void render_security(void) {
    begin("SECURITY");
    picopen_engagement_t engagement;
    picopen_engagement_session_snapshot(&engagement);
    const uint64_t now_ms = time_us_64() / 1000u;
    const bool active = picopen_engagement_is_active(&engagement, now_ms);
    static const uint16_t durations_minutes[] = {15u, 60u, 240u};
    append("BOUNDARY %s  MODE %s\n\n", active ? "ACTIVE" : "OPTIONAL",
        security_mode==PICOPEN_SECURITY_OWNER?"OWNER":security_mode==PICOPEN_SECURITY_GUARDED?"GUARDED":"DEVELOPER");
    append(selection == 0u ? "> REF %-20s\n" : "  REF %-20s\n",
           engagement_reference_length == 0u ? "<TYPE REFERENCE>"
                                             : engagement_reference);
    append(selection == 1u ? "> LIMIT %-20.20s\n" : "  LIMIT %-20.20s\n",
           engagement_target_length == 0u ? "<OPTIONAL>" : engagement_target);
    append(selection == 2u ? "> DURATION %u MIN\n" : "  DURATION %u MIN\n",
           durations_minutes[engagement_duration_index]);
    append(selection == 3u ? "> %s\n" : "  %s\n",
           active ? "END SCOPE" : "ACTIVATE SCOPE");
    append(selection == 4u ? "> MODE %s\n" : "  MODE %s\n",
        security_mode==PICOPEN_SECURITY_OWNER?"OWNER":security_mode==PICOPEN_SECURITY_GUARDED?"GUARDED":"DEVELOPER");
    if (active) {
        append("\nREF %s\nLIMIT %s\nREMAINING %llu MIN\n", engagement.reference,
               engagement.boundary_configured ? engagement.target : "NONE",
               (engagement.expires_ms - now_ms + 59999u) / 60000u);
    }
    append("\nSCOPE DOES NOT GRANT CAPABILITIES\nPORTS ARE CHOSEN INSIDE APPS\nOWNER MODE MINIMIZES PROMPTS\n");
    present();
}

static void render_update(void) {
    begin("WIFI UPDATE");
    picopen_wifi_status_t wifi;
    picopen_wifi_get_status(&wifi);
    append("INTERFACE %-10s D%d L%d\n",
           picopen_wifi_state_name(wifi.state), wifi.driver_result,
           wifi.link_status);
    append(selection == 0u ? "> STORE %s\n" : "  STORE %s\n",
           picopen_internal_fs_state() == PICOPEN_INTERNAL_FS_READY ? "READY" : "INITIALIZE");
    append(selection == 1u ? "> POWER %s\n" : "  POWER %s\n",
           wifi.state == PICOPEN_WIFI_OFF ? "ENABLE" : "DISABLE");
    append(selection == 2u ? "> PASSIVE SCAN\n" : "  PASSIVE SCAN\n");
    append(selection == 3u ? "> SSID %-24.24s\n" : "  SSID %-24.24s\n",
           wifi_ssid_length ? wifi_ssid : "<TYPE>");
    append(selection == 4u ? "> PASS %.*s\n" : "  PASS %.*s\n",
           (int)wifi_password_length, "********************************");
    append(selection == 5u ? "> PIN  %.*s\n" : "  PIN  %.*s\n",
           (int)wifi_pin_length, "****************");
    append(selection == 6u ? "> %s\n" : "  %s\n",
           picopen_wifi_vault_present() ? "LOAD SAVED" : "REMEMBER");
    append(selection == 7u ? "> %s\n" : "  %s\n",
           wifi.state == PICOPEN_WIFI_CONNECTED ? "DISCONNECT" :
           wifi.state == PICOPEN_WIFI_CONNECTING ? "CANCEL" : "CONNECT");
    append(selection == 8u ? "> FORGET SAVED\n" : "  FORGET SAVED\n");
    append("APS %u%s", (unsigned int)wifi.ap_count,
           wifi.ap_truncated ? "+" : "");
    if (wifi.ap_count) {
        if (wifi_ap_selection >= wifi.ap_count) wifi_ap_selection = 0u;
        append(" %u:%-14.14s %ddBm C%u", (unsigned int)(wifi_ap_selection + 1u),
               wifi.aps[wifi_ap_selection].ssid,
               wifi.aps[wifi_ap_selection].rssi,
               wifi.aps[wifi_ap_selection].channel);
    }
    append("\nIP:%-15s GW:%-15s\nDNS:%-15s RSSI:%ld DHCP:%s\n",
           wifi.ipv4[0] ? wifi.ipv4 : "WAITING",
           wifi.gateway[0] ? wifi.gateway : "-",
           wifi.dns[0] ? wifi.dns : "-", (long)wifi.rssi,
           wifi.dhcp_bound ? "BOUND" : "WAIT");
    append("VAULT:%s RETRY:%us NO LISTENERS\n",
           picopen_wifi_vault_result_name(wifi_vault_result),
           picopen_wifi_vault_retry_seconds(time_us_64() / 1000u));
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
        case GUI_APPS: render_workbench(); break;
        case GUI_APP_DEVICES: render_devices(); break;
        case GUI_APP_CONFIG: render_app_config(); break;
        case GUI_RECON_CONFIRM: render_recon_confirm(); break;
        case GUI_RECON: render_recon(); break;
        case GUI_RECON_HISTORY: render_recon_history(); break;
        case GUI_EVIDENCE_PICKER: render_evidence_picker(); break;
        case GUI_EVIDENCE: render_evidence(); break;
        case GUI_AUDIT: render_audit(); break;
        case GUI_SYSTEM: render_system(); break;
        case GUI_SECURITY: render_security(); break;
        case GUI_UPDATE: render_update(); break;
        case GUI_SKINS: render_skins(); break;
        case GUI_ABOUT: render_about(); break;
        case GUI_RECOVERY: render_recovery(); break;
        case GUI_SHUTDOWN: render_shutdown(); break;
        case GUI_SHUTDOWN_RESULT: break;
        case GUI_TERMINAL: break;
    }
}

static void open_home_item(void) {
    static const gui_screen_t screens[GUI_HOME_ITEMS] = {
        GUI_STATUS, GUI_FILES, GUI_DEVICES, GUI_APPS, GUI_AUDIT, GUI_SYSTEM,
    };
    home_selection = selection;
    (void)picopen_preferences_set_menu(home_selection, system_selection,
                                       files_selection);
    screen = screens[selection];
    if (screen == GUI_SYSTEM) {
        selection = system_selection;
    } else if (screen == GUI_FILES) {
        selection = files_selection < file_listing.count ? files_selection : 0u;
    } else if (screen == GUI_APPS) {
        selection = workbench_selection;
    } else {
        selection = 0u;
    }
    render();
}

static void open_system_item(void) {
    static const gui_screen_t screens[GUI_SYSTEM_ITEMS] = {
        GUI_SECURITY, GUI_UPDATE, GUI_SKINS, GUI_TERMINAL, GUI_RECOVERY,
        GUI_SHUTDOWN, GUI_ABOUT,
    };
    system_selection = selection;
    (void)picopen_preferences_set_menu(home_selection, system_selection,
                                       files_selection);
    screen = screens[selection];
    if (screen == GUI_SKINS) selection = (size_t)picopen_skin_current_id();
    else if (screen == GUI_SECURITY) selection = security_selection;
    else if (screen == GUI_UPDATE) selection = wifi_selection;
    else selection = 0u;
    if (screen == GUI_TERMINAL) {
        rendered_page_valid = false;
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
    if(current==GUI_APP_DEVICES||current==GUI_APP_CONFIG||current==GUI_RECON_CONFIRM||current==GUI_RECON||
       current==GUI_RECON_HISTORY||current==GUI_EVIDENCE_PICKER||
       current==GUI_EVIDENCE)return GUI_APPS;
    if ((current == GUI_SECURITY) || (current == GUI_UPDATE) ||
        (current == GUI_SKINS) || (current == GUI_ABOUT) ||
        (current == GUI_RECOVERY) ||
        (current == GUI_SHUTDOWN) ||
        (current == GUI_SHUTDOWN_RESULT)) {
        return GUI_SYSTEM;
    }
    return GUI_HOME;
}

void picopen_gui_init(const picopen_shell_state_t *state) {
    if (state != NULL) {
        gui_state = *state;
        file_listing = state->storage;
    }
    strcpy(file_path, "/");
    picopen_preferences_t preferences;
    picopen_preferences_get(&preferences);
    security_mode=(picopen_security_mode_t)preferences.security_mode;
    picopen_apps_init();
    picopen_apps_scan_sd(&gui_state.storage_service);
    picopen_apps_snapshot(&app_catalog);
    screen = GUI_HOME;
    home_selection = preferences.home_selection < GUI_HOME_ITEMS
        ? preferences.home_selection : 0u;
    system_selection = preferences.system_selection < GUI_SYSTEM_ITEMS
        ? preferences.system_selection : 0u;
    files_selection = preferences.files_selection < PICOPEN_STORAGE_MAX_ENTRIES
        ? preferences.files_selection : 0u;
    workbench_selection = 0u;
    security_selection = 0u;
    wifi_selection = 0u;
    selection = home_selection;
    picopen_audit_record("gui.start", true);
    render();
}

void picopen_gui_show_boot_status(const char *stage, const char *status) {
    rendered_page_valid = false;
    picopen_display_fill_rect(0u, 0u, 320u, 320u, GUI_COLOR_BACKGROUND);
    picopen_display_fill_rect(0u, 0u, 320u, 54u, GUI_COLOR_HEADER);
    picopen_terminal_draw_text_at(58u, 16u, "PICOPEN", 3u,
                                  GUI_COLOR_MAGENTA, GUI_COLOR_HEADER);
    outline(20u, 105u, 280u, 100u, 2u, GUI_COLOR_VIOLET);
    picopen_terminal_draw_text_at(38u, 126u,
        stage != NULL ? stage : "STARTING", 2u, GUI_COLOR_CYAN,
        GUI_COLOR_BACKGROUND);
    picopen_terminal_draw_text_at(38u, 164u,
        status != NULL ? status : "PLEASE WAIT", 1u, GUI_COLOR_MUTED,
        GUI_COLOR_BACKGROUND);
    picopen_terminal_draw_text_at(61u, 270u, "SECURE CORE READY", 1u,
                                  GUI_COLOR_GOLD, GUI_COLOR_BACKGROUND);
}

void picopen_gui_update_state(const picopen_shell_state_t *state) {
    if (state == NULL) {
        return;
    }
    gui_state = *state;
    if ((strcmp(file_path, "/") == 0) || !state->storage_ready) {
        file_listing = state->storage;
    }
    if (screen == GUI_STATUS) {
        render_status();
    } else if (screen == GUI_HOME) {
        render_home();
    } else if (screen == GUI_DEVICES) {
        render_devices();
    } else if (screen == GUI_SECURITY) {
        render_security();
    } else if (screen == GUI_UPDATE) {
        render_update();
    } else if (screen == GUI_FILES) {
        if (selection >= file_listing.count) {
            selection = 0u;
        }
        render_files();
    }
}

void picopen_gui_refresh_workbench(void) {
    if (screen == GUI_APPS) {
        render_workbench();
    } else if(screen==GUI_RECON){render_recon();
    } else if(screen==GUI_EVIDENCE){render_evidence();
    }
}

void picopen_gui_handle_key(uint8_t key) {
    if (screen == GUI_TERMINAL) {
        if (key == PICOPEN_KEY_ESCAPE) {
            screen = GUI_SYSTEM;
            selection = system_selection;
            render();
        } else {
            picopen_shell_handle_key(key);
        }
        return;
    }
    if (key == PICOPEN_KEY_ESCAPE) {
        if (screen == GUI_RECON_CONFIRM || screen == GUI_RECON_HISTORY ||
            screen == GUI_EVIDENCE_PICKER) {
            screen = GUI_APPS;
            selection = workbench_selection;
            render_workbench();
            return;
        }
        if (screen == GUI_RECON) {
            picopen_recon_snapshot_t snapshot;
            picopen_recon_snapshot(&snapshot);
            if (snapshot.state == PICOPEN_RECON_RUNNING) {
                const bool cancelled = picopen_recon_cancel();
                picopen_audit_record("recon.cancel", cancelled);
            }
            screen = GUI_APPS;
            selection = workbench_selection;
            render_workbench();
            return;
        }
        if (screen == GUI_EVIDENCE) {
            picopen_evidence_snapshot_t snapshot;
            picopen_evidence_snapshot(&snapshot);
            if ((snapshot.state == PICOPEN_EVIDENCE_HASHING) ||
                (snapshot.state == PICOPEN_EVIDENCE_PARSING)) {
                const bool cancelled = picopen_evidence_cancel();
                picopen_audit_record("evidence.cancel", cancelled);
            }
            screen = GUI_APPS;
            selection = workbench_selection;
            render_workbench();
            return;
        }
        if ((screen == GUI_FILES) && (strcmp(file_path, "/") != 0)) {
            char *const separator = strrchr(file_path, '/');
            if ((separator == NULL) || (separator == file_path)) {
                strcpy(file_path, "/");
            } else {
                *separator = '\0';
            }
            const picopen_storage_result_t result =
                picopen_storage_list_directory(&gui_state.storage_service,
                                               file_path, &file_listing);
            if ((result != PICOPEN_STORAGE_OK) &&
                (result != PICOPEN_STORAGE_LIMIT_REACHED)) {
                file_listing = (picopen_storage_listing_t){0};
            }
            selection = 0u;
            render_files();
            return;
        }
        const gui_screen_t previous_screen = screen;
        if ((previous_screen == GUI_FILES) && (strcmp(file_path, "/") == 0)) {
            files_selection = selection;
            (void)picopen_preferences_set_menu(home_selection,
                                               system_selection,
                                               files_selection);
        }
        screen = parent_screen(screen);
        if ((screen == GUI_FILES) && (selection >= file_listing.count)) {
            selection = 0u;
        } else if (screen == GUI_HOME) {
            selection = home_selection;
        } else if (screen == GUI_SYSTEM) {
            selection = system_selection;
        } else if (previous_screen != GUI_FILE_VIEW) {
            selection = 0u;
        }
        render();
        return;
    }
    if (screen == GUI_HOME) {
        const size_t previous = selection;
        if ((key == PICOPEN_KEY_LEFT) && ((selection & 1u) != 0u)) --selection;
        if ((key == PICOPEN_KEY_RIGHT) && ((selection & 1u) == 0u)) ++selection;
        if ((key == PICOPEN_KEY_UP) && (selection >= 2u)) selection -= 2u;
        if ((key == PICOPEN_KEY_DOWN) && (selection + 2u < GUI_HOME_ITEMS)) selection += 2u;
        if (key == PICOPEN_KEY_ENTER) { open_home_item(); return; }
        if (selection != previous) {
            redraw_home_focus(previous, selection);
        }
        return;
    }
    if (screen == GUI_APPS) {
        const size_t previous = selection;
        if((key==PICOPEN_KEY_UP)&&selection>0u)--selection;
        else if((key==PICOPEN_KEY_DOWN)&&selection+1u<app_catalog.count&&selection+1u<GUI_APPS_VISIBLE_ITEMS)++selection;
        else if(key==PICOPEN_KEY_ENTER && selection<app_catalog.count){
            workbench_selection = selection;
            pending_recon_selection = selection;
            const picopen_app_kind_t kind=app_catalog.apps[selection].kind;
            if(kind==PICOPEN_APP_DEVICE_INVENTORY){selection=0u;screen=GUI_APP_DEVICES;render_devices();return;}
            if(kind==PICOPEN_APP_EVIDENCE_ANALYZER){selection=files_selection<file_listing.count?files_selection:0u;screen=GUI_EVIDENCE_PICKER;render_evidence_picker();return;}
            if(kind==PICOPEN_APP_RECENT_RESULTS){screen=GUI_RECON_HISTORY;render_recon_history();return;}
            task_port=kind==PICOPEN_APP_SSH_BANNER?22u:
                kind==PICOPEN_APP_TLS_INSPECTOR?443u:
                kind==PICOPEN_APP_HTTP_INSPECTOR?80u:80u;
            task_field=0u;task_error[0]='\0';screen=GUI_APP_CONFIG;render_app_config();
            return;
        }
        if (selection != previous) {
            workbench_selection = selection;
        }
        render_workbench();
        return;
    }
    if(screen==GUI_APP_CONFIG){
        const picopen_app_kind_t kind=app_catalog.apps[pending_recon_selection].kind;
        if(kind==PICOPEN_APP_NETWORK_DISCOVERY||kind==PICOPEN_APP_SESSION_REPORTS||kind==PICOPEN_APP_SD_PACKAGE)return;
        if(key==PICOPEN_KEY_UP&&task_field>0u)--task_field;
        else if(key==PICOPEN_KEY_DOWN&&task_field<1u)++task_field;
        else if(task_field==1u&&key==PICOPEN_KEY_LEFT&&task_port>1u)--task_port;
        else if(task_field==1u&&key==PICOPEN_KEY_RIGHT&&task_port<65535u)++task_port;
        else if(task_field==0u&&(key==0x08u||key==0x7fu)&&task_target_length>0u)task_target[--task_target_length]='\0';
        else if(task_field==0u&&key>=' '&&key<='~'&&task_target_length+1u<sizeof(task_target)){task_target[task_target_length++]=(char)key;task_target[task_target_length]='\0';}
        else if(key==PICOPEN_KEY_ENTER){if(task_field==0u){task_field=1u;}else if(task_target_length>0u){screen=GUI_RECON_CONFIRM;render_recon_confirm();return;}}
        render_app_config();return;
    }
    if (screen == GUI_RECON_CONFIRM) {
        if (key == PICOPEN_KEY_ENTER) {
            picopen_security_context_t task_security = gui_state.security;
            if (security_mode != PICOPEN_SECURITY_GUARDED) {
                task_security.engagement_active = true;
            }
            const bool authorized = picopen_security_authorize(
                &task_security, PICOPEN_CAP_NETWORK_PROBE, true);
            const bool started = authorized && picopen_recon_start(
                recon_kind_for_selection(pending_recon_selection), task_target,
                task_port, time_us_64() / 1000u, true,
                security_mode == PICOPEN_SECURITY_GUARDED);
            picopen_audit_record("recon.start", started);
            if (started) {
                screen = GUI_RECON;
                render_recon();
            } else {
                snprintf(task_error, sizeof(task_error), "%s",
                    authorized ? "START FAILED: CHECK WIFI / RETRY"
                               : "DENIED: GUARDED MODE NEEDS SESSION");
                screen = GUI_APP_CONFIG;
                render_app_config();
            }
        }
        return;
    }
    if (screen == GUI_RECON_HISTORY) return;
    if (screen == GUI_EVIDENCE_PICKER) {
        if ((key == PICOPEN_KEY_UP) && selection > 0u) --selection;
        else if ((key == PICOPEN_KEY_DOWN) && selection + 1u < file_listing.count)
            ++selection;
        else if (key == PICOPEN_KEY_ENTER && selection < file_listing.count) {
            const picopen_storage_entry_t *entry = &file_listing.entries[selection];
            if (!entry->directory && picopen_security_authorize(
                    &gui_state.security, PICOPEN_CAP_STORAGE_READ, false)) {
                char path[PICOPEN_STORAGE_PATH_SIZE];
                const int length = snprintf(path, sizeof(path),
                    strcmp(file_path, "/") == 0 ? "/%s" : "%s/%s",
                    file_path, entry->name);
                const bool started = length > 0 && (size_t)length < sizeof(path) &&
                    picopen_evidence_start(&gui_state.storage_service, path,
                                            entry->size, true);
                picopen_audit_record("evidence.start", started);
                if (started) {
                    files_selection = selection;
                    screen = GUI_EVIDENCE;
                    render_evidence();
                    return;
                }
            }
        }
        render_evidence_picker();
        return;
    }
    if((screen==GUI_RECON)||(screen==GUI_EVIDENCE))return;
    if (screen == GUI_FILES) {
        if ((key == 's') || (key == 'S')) {
            storage_action = PICOPEN_GUI_STORAGE_SAFE_REMOVE;
            return;
        }
        if ((key == 'r') || (key == 'R')) {
            storage_action = PICOPEN_GUI_STORAGE_RESCAN;
            return;
        }
        const size_t count = file_listing.count;
        if ((key == PICOPEN_KEY_UP) && (selection > 0u)) --selection;
        if ((key == PICOPEN_KEY_DOWN) && (selection + 1u < count)) ++selection;
        files_selection = selection;
        if ((key == PICOPEN_KEY_ENTER) && (count != 0u)) {
            const picopen_storage_entry_t *const entry =
                &file_listing.entries[selection];
            if (entry->directory) {
                char next_path[PICOPEN_STORAGE_PATH_SIZE];
                const int length = snprintf(next_path, sizeof(next_path),
                    strcmp(file_path, "/") == 0 ? "/%s" : "%s/%s",
                    file_path, entry->name);
                picopen_storage_listing_t next_listing;
                const picopen_storage_result_t result =
                    ((length > 0) && ((size_t)length < sizeof(next_path)))
                        ? picopen_storage_list_directory(
                              &gui_state.storage_service, next_path,
                              &next_listing)
                        : PICOPEN_STORAGE_INVALID_REQUEST;
                if ((result == PICOPEN_STORAGE_OK) ||
                    (result == PICOPEN_STORAGE_LIMIT_REACHED)) {
                    strcpy(file_path, next_path);
                    file_listing = next_listing;
                    selection = 0u;
                    picopen_audit_record("gui.directory.open", true);
                } else {
                    picopen_audit_record("gui.directory.open", false);
                }
                render_files();
                return;
            }
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
    if (screen == GUI_SECURITY) {
        static const uint64_t durations_ms[] = {
            UINT64_C(900000), UINT64_C(3600000), UINT64_C(14400000),
        };
        if ((key == PICOPEN_KEY_UP) && (selection > 0u)) {
            --selection;
        } else if ((key == PICOPEN_KEY_DOWN) &&
                   (selection + 1u < GUI_SECURITY_ITEMS)) {
            ++selection;
        } else if ((selection == 2u) && (key == PICOPEN_KEY_LEFT) &&
                   (engagement_duration_index > 0u)) {
            --engagement_duration_index;
        } else if ((selection == 2u) && (key == PICOPEN_KEY_RIGHT) &&
                   (engagement_duration_index + 1u <
                    sizeof(durations_ms) / sizeof(durations_ms[0]))) {
            ++engagement_duration_index;
        } else if (((selection == 0u) || (selection == 1u)) &&
                   ((key == 0x08u) || (key == 0x7Fu)) &&
                   ((selection==0u ? engagement_reference_length : engagement_target_length) > 0u)) {
            char *value=selection==0u ? engagement_reference : engagement_target;
            size_t *length=selection==0u ? &engagement_reference_length : &engagement_target_length;
            value[--(*length)]='\0';
        } else if (((selection == 0u) || (selection == 1u)) &&
                   ((selection==0u ? engagement_reference_length+1u<sizeof(engagement_reference)
                                   : engagement_target_length+1u<sizeof(engagement_target))) &&
                   (((key >= 'A') && (key <= 'Z')) ||
                    ((key >= 'a') && (key <= 'z')) ||
                    ((key >= '0') && (key <= '9')) ||
                    (key == '-') || (key == '_') || (key == '.') || (key=='/'))) {
            char *value=selection==0u ? engagement_reference : engagement_target;
            size_t *length=selection==0u ? &engagement_reference_length : &engagement_target_length;
            value[(*length)++]=(char)key; value[*length]='\0';
        } else if (key == PICOPEN_KEY_ENTER) {
            if (selection < 3u) {
                ++selection;
            } else if(selection==3u) {
                const uint64_t now_ms = time_us_64() / 1000u;
                const bool active = picopen_engagement_session_active(now_ms);
                const bool changed = active
                    ? picopen_engagement_session_deactivate(true)
                    : picopen_engagement_session_activate_optional_boundary(
                          engagement_reference, engagement_target,
                          now_ms,
                          durations_ms[engagement_duration_index], true);
                picopen_audit_record(active ? "scope.end" : "scope.start",
                                     changed);
                picopen_engagement_session_snapshot(&gui_state.engagement);
                gui_state.security.engagement_active =
                    picopen_engagement_is_active(&gui_state.engagement, now_ms);
            } else {
                security_mode=(picopen_security_mode_t)((security_mode+1u)%PICOPEN_SECURITY_MODE_COUNT);
                (void)picopen_preferences_set_security_mode(security_mode);
                picopen_audit_record("security.mode",true);
            }
        }
        render_security();
        security_selection = selection;
        return;
    }
    if (screen == GUI_UPDATE) {
        if ((key == PICOPEN_KEY_UP) && (selection > 0u)) --selection;
        else if ((key == PICOPEN_KEY_DOWN) && (selection + 1u < GUI_WIFI_ITEMS)) ++selection;
        else if ((selection == 3u) &&
                 ((key == PICOPEN_KEY_LEFT) || (key == PICOPEN_KEY_RIGHT))) {
            picopen_wifi_status_t wifi;
            picopen_wifi_get_status(&wifi);
            if (wifi.ap_count > 0u) {
                if (key == PICOPEN_KEY_LEFT) {
                    wifi_ap_selection = wifi_ap_selection == 0u
                        ? wifi.ap_count - 1u : wifi_ap_selection - 1u;
                } else {
                    wifi_ap_selection = (wifi_ap_selection + 1u) % wifi.ap_count;
                }
                if (picopen_wifi_select_ap(wifi_ap_selection, wifi_ssid,
                                           sizeof(wifi_ssid))) {
                    wifi_ssid_length = strlen(wifi_ssid);
                }
            }
        } else if (((key == 0x08u) || (key == 0x7Fu)) &&
                 ((selection >= 3u) && (selection <= 5u))) {
            char *value = selection == 3u ? wifi_ssid : selection == 4u ? wifi_password : wifi_pin;
            size_t *length = selection == 3u ? &wifi_ssid_length : selection == 4u ? &wifi_password_length : &wifi_pin_length;
            if (*length > 0u) value[--(*length)] = '\0';
        } else if ((key >= ' ') && (key <= '~') &&
                   ((selection >= 3u) && (selection <= 5u))) {
            char *value = selection == 3u ? wifi_ssid : selection == 4u ? wifi_password : wifi_pin;
            size_t *length = selection == 3u ? &wifi_ssid_length : selection == 4u ? &wifi_password_length : &wifi_pin_length;
            const size_t capacity = selection == 3u ? sizeof(wifi_ssid) : selection == 4u ? sizeof(wifi_password) : sizeof(wifi_pin);
            if (*length + 1u < capacity) {
                value[(*length)++] = (char)key;
                value[*length] = '\0';
            }
        } else if (key == PICOPEN_KEY_ENTER) {
            picopen_wifi_status_t wifi;
            picopen_wifi_get_status(&wifi);
            bool changed = false;
            if (selection == 0u) {
                changed = picopen_internal_fs_format(true);
                picopen_audit_record("store.init", changed);
            } else if (selection == 1u && wifi.state == PICOPEN_WIFI_OFF) {
                const bool authorized = picopen_security_authorize(
                    &gui_state.security, PICOPEN_CAP_NETWORK_CONNECT, true);
                changed = authorized && picopen_wifi_enable(true);
                picopen_audit_record("wifi.enable", changed);
            } else if (selection == 1u) {
                picopen_wifi_disable();
                picopen_audit_record("wifi.disable", true);
            } else if (selection == 2u) {
                const bool authorized = picopen_security_authorize(
                    &gui_state.security, PICOPEN_CAP_RADIO_RECEIVE, false);
                changed = authorized && picopen_wifi_scan_passive(true);
                picopen_audit_record("wifi.scan", changed);
            } else if (selection == 6u && picopen_wifi_vault_present()) {
                wifi_vault_result = picopen_wifi_vault_load(
                    wifi_pin, wifi_ssid, sizeof(wifi_ssid), wifi_password,
                    sizeof(wifi_password), time_us_64() / 1000u);
                changed = wifi_vault_result == PICOPEN_VAULT_OK;
                if (changed) { wifi_ssid_length = strlen(wifi_ssid); wifi_password_length = strlen(wifi_password); }
                scrub_secret(wifi_pin, sizeof(wifi_pin)); wifi_pin_length = 0u;
                picopen_audit_record("vault.load", changed);
            } else if (selection == 6u) {
                wifi_vault_result = picopen_wifi_vault_save(wifi_pin, wifi_ssid, wifi_password);
                changed = wifi_vault_result == PICOPEN_VAULT_OK;
                scrub_secret(wifi_pin, sizeof(wifi_pin)); wifi_pin_length = 0u;
                picopen_audit_record("vault.save", changed);
            } else if (selection == 7u &&
                       (wifi.state == PICOPEN_WIFI_CONNECTED ||
                        wifi.state == PICOPEN_WIFI_CONNECTING)) {
                changed = picopen_wifi_disconnect(true);
                picopen_audit_record("wifi.disconnect", changed);
            } else if (selection == 7u) {
                const bool authorized = picopen_security_authorize(
                    &gui_state.security, PICOPEN_CAP_NETWORK_CONNECT, true);
                changed = authorized && picopen_wifi_connect(
                    wifi_ssid, wifi_password, true, time_us_64() / 1000u);
                scrub_secret(wifi_password, sizeof(wifi_password));
                wifi_password_length = 0u;
                picopen_audit_record("wifi.connect", changed);
            } else if (selection == 8u) {
                changed = picopen_wifi_vault_forget(true);
                scrub_secret(wifi_password, sizeof(wifi_password));
                scrub_secret(wifi_pin, sizeof(wifi_pin));
                wifi_password_length = 0u;
                wifi_pin_length = 0u;
                wifi_vault_result = PICOPEN_VAULT_EMPTY;
                picopen_audit_record("vault.forget", changed);
            }
            (void)changed;
        }
        render_update();
        wifi_selection = selection;
        return;
    }
    if (screen == GUI_SKINS) {
        if ((key == PICOPEN_KEY_UP) && (selection > 0u)) --selection;
        if ((key == PICOPEN_KEY_DOWN) && (selection + 1u < PICOPEN_SKIN_COUNT)) ++selection;
        if (key == PICOPEN_KEY_ENTER) {
            (void)picopen_skin_select((picopen_skin_id_t)selection);
            const bool persisted = picopen_preferences_set_skin((uint8_t)selection);
            picopen_crayon_renderer_invalidate();
            picopen_synthwave_renderer_invalidate();
            picopen_audit_record("skin.select", true);
            picopen_audit_record("skin.persist", persisted);
            screen = GUI_HOME;
            selection = home_selection;
            render_home();
            return;
        }
        render_skins();
        return;
    }
    if (screen == GUI_RECOVERY) {
        if (key == PICOPEN_KEY_ENTER) {
            const bool cleared=picopen_recovery_clear(true);
            picopen_audit_record("crash.clear",cleared);
        }
        render_recovery(); return;
    }
    if (screen == GUI_SHUTDOWN) {
        if ((key == PICOPEN_KEY_UP) || (key == PICOPEN_KEY_DOWN)) selection ^= 1u;
        if (key == PICOPEN_KEY_ENTER) {
            if (selection == 0u) {
                screen = GUI_SYSTEM;
                selection = system_selection;
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

picopen_gui_storage_action_t picopen_gui_take_storage_action(void) {
    const picopen_gui_storage_action_t action=storage_action;
    storage_action=PICOPEN_GUI_STORAGE_NONE; return action;
}
