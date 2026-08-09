#include "picopen/gui.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"

#include "picopen/audit.h"
#include "picopen/crayon_renderer.h"
#include "picopen/display.h"
#include "picopen/keyboard.h"
#include "picopen/skin.h"
#include "picopen/storage.h"
#include "picopen/synthwave_renderer.h"
#include "picopen/terminal.h"
#include "picopen/workbench.h"

#define GUI_SCREEN_SIZE 1024u
#define GUI_HOME_ITEMS 6u
#define GUI_SYSTEM_ITEMS 6u
#define GUI_SECURITY_ITEMS 3u

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
    GUI_WORKBENCH,
    GUI_AUDIT,
    GUI_SYSTEM,
    GUI_SECURITY,
    GUI_UPDATE,
    GUI_SKINS,
    GUI_ABOUT,
    GUI_SHUTDOWN,
    GUI_SHUTDOWN_RESULT,
    GUI_TERMINAL,
} gui_screen_t;

static picopen_shell_state_t gui_state;
static gui_screen_t screen;
static size_t selection;
static size_t home_selection;
static size_t system_selection;
static picopen_storage_listing_t file_listing;
static char file_path[PICOPEN_STORAGE_PATH_SIZE];
static char engagement_reference[PICOPEN_ENGAGEMENT_REFERENCE_SIZE];
static size_t engagement_reference_length;
static size_t engagement_duration_index;
static char canvas[GUI_SCREEN_SIZE];
static char canvas_title[20];
static size_t canvas_length;
static char rendered_lines[20][41];
static bool rendered_page_valid;
static gui_screen_t rendered_page_screen;
static picopen_skin_id_t rendered_page_skin;

static void outline(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                    uint16_t thickness, uint32_t color);

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
        "STATUS", "FILES", "DEVICES", "WORKBENCH", "AUDIT", "SYSTEM",
    };
    if (picopen_skin_current_id() == PICOPEN_SKIN_CRAYON) {
        picopen_crayon_renderer_home(labels, GUI_HOME_ITEMS, selection);
        const bool active = picopen_engagement_session_active(
            time_us_64() / 1000u);
        picopen_display_fill_rect(240u, 4u, 76u, 14u,
                                  active ? 0x145A32u : 0x501020u);
        picopen_terminal_draw_text_at(246u, 7u,
            active ? "SCOPE ON" : "SCOPE OFF", 1u, 0xFFFFFFu,
            active ? 0x145A32u : 0x501020u);
        rendered_page_valid = false;
        return;
    }
    if (picopen_skin_current_id() == PICOPEN_SKIN_SYNTHWAVE) {
        picopen_synthwave_renderer_home(labels, GUI_HOME_ITEMS, selection);
        const bool active = picopen_engagement_session_active(
            time_us_64() / 1000u);
        picopen_display_fill_rect(240u, 4u, 76u, 14u,
                                  active ? 0x145A32u : 0x501020u);
        picopen_terminal_draw_text_at(246u, 7u,
            active ? "SCOPE ON" : "SCOPE OFF", 1u, 0xFFFFFFu,
            active ? 0x145A32u : 0x501020u);
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
                   picopen_engagement_session_active(time_us_64() / 1000u)
                       ? "SCOPE ON" : "SCOPE OFF",
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
        "STATUS", "FILES", "DEVICES", "WORKBENCH", "AUDIT", "SYSTEM",
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
    append("\nUP/DOWN  ENTER OPEN  ESC BACK\n");
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
    begin("WORKBENCH");
    picopen_workbench_snapshot_t snapshot;
    picopen_workbench_snapshot(&snapshot);
    append("JOB %-9s  %3u%%\n", picopen_workbench_job_state_name(snapshot.state),
           snapshot.progress_percent);
    for (size_t index = 0u; index < snapshot.item_count; ++index) {
        append("%-12s %s\n", snapshot.items[index].name,
               picopen_workbench_item_state_name(snapshot.items[index].state));
    }
    append("\nCONFIG/POLICY INVENTORY ONLY\nNO BUS TRAFFIC OR PIN CHANGES\n");
    if (snapshot.state == PICOPEN_WORKBENCH_RUNNING) {
        append("\nESC CANCEL\n");
    } else {
        append("\nENTER RUN  ESC BACK\n");
    }
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
        "SECURITY", "WIFI UPDATE", "SKINS", "TERMINAL", "POWER", "ABOUT",
    };
    begin("SYSTEM");
    append("\n");
    for (size_t index = 0u; index < GUI_SYSTEM_ITEMS; ++index) {
        item(labels[index], index);
    }
    append("\nUP/DOWN  ENTER SELECT  ESC BACK\n");
    present();
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
    append("SCOPE %s  SESSION ONLY\n\n", active ? "ACTIVE" : "INACTIVE");
    append(selection == 0u ? "> REF %-20s\n" : "  REF %-20s\n",
           engagement_reference_length == 0u ? "<TYPE REFERENCE>"
                                             : engagement_reference);
    append(selection == 1u ? "> DURATION %u MIN\n" : "  DURATION %u MIN\n",
           durations_minutes[engagement_duration_index]);
    append(selection == 2u ? "> %s\n" : "  %s\n",
           active ? "END SCOPE" : "ACTIVATE SCOPE");
    if (active) {
        append("\nREF %s\nREMAINING %llu MIN\n", engagement.reference,
               (engagement.expires_ms - now_ms + 59999u) / 60000u);
    }
    append("\nSCOPE DOES NOT GRANT CAPABILITIES\n"
           "ACTIVE OUTPUTS REMAIN DENIED\n\n"
           "UP/DOWN FIELD  LEFT/RIGHT TIME\nENTER SELECT  ESC BACK\n");
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
        case GUI_SKINS: render_skins(); break;
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
    home_selection = selection;
    screen = screens[selection];
    selection = screen == GUI_SYSTEM ? system_selection : 0u;
    render();
}

static void open_system_item(void) {
    static const gui_screen_t screens[GUI_SYSTEM_ITEMS] = {
        GUI_SECURITY, GUI_UPDATE, GUI_SKINS, GUI_TERMINAL, GUI_SHUTDOWN,
        GUI_ABOUT,
    };
    system_selection = selection;
    screen = screens[selection];
    selection = screen == GUI_SKINS ? (size_t)picopen_skin_current_id() : 0u;
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
    if ((current == GUI_SECURITY) || (current == GUI_UPDATE) ||
        (current == GUI_SKINS) || (current == GUI_ABOUT) ||
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
    screen = GUI_HOME;
    selection = 0u;
    home_selection = 0u;
    system_selection = 0u;
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
    } else if (screen == GUI_FILES) {
        if (selection >= file_listing.count) {
            selection = 0u;
        }
        render_files();
    }
}

void picopen_gui_refresh_workbench(void) {
    if (screen == GUI_WORKBENCH) {
        render_workbench();
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
        if (screen == GUI_WORKBENCH) {
            picopen_workbench_snapshot_t snapshot;
            picopen_workbench_snapshot(&snapshot);
            if (snapshot.state == PICOPEN_WORKBENCH_RUNNING) {
                const bool cancelled = picopen_workbench_cancel();
                picopen_audit_record("workbench.cancel", cancelled);
                render_workbench();
                return;
            }
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
    if (screen == GUI_WORKBENCH) {
        if (key == PICOPEN_KEY_ENTER) {
            const bool started = picopen_workbench_start(
                &gui_state.devices, time_us_64() / 1000u);
            picopen_audit_record("workbench.start", started);
            render_workbench();
        }
        return;
    }
    if (screen == GUI_FILES) {
        const size_t count = file_listing.count;
        if ((key == PICOPEN_KEY_UP) && (selection > 0u)) --selection;
        if ((key == PICOPEN_KEY_DOWN) && (selection + 1u < count)) ++selection;
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
        } else if ((selection == 1u) && (key == PICOPEN_KEY_LEFT) &&
                   (engagement_duration_index > 0u)) {
            --engagement_duration_index;
        } else if ((selection == 1u) && (key == PICOPEN_KEY_RIGHT) &&
                   (engagement_duration_index + 1u <
                    sizeof(durations_ms) / sizeof(durations_ms[0]))) {
            ++engagement_duration_index;
        } else if ((selection == 0u) &&
                   ((key == 0x08u) || (key == 0x7Fu)) &&
                   (engagement_reference_length > 0u)) {
            engagement_reference[--engagement_reference_length] = '\0';
        } else if ((selection == 0u) &&
                   (engagement_reference_length + 1u <
                    sizeof(engagement_reference)) &&
                   (((key >= 'A') && (key <= 'Z')) ||
                    ((key >= 'a') && (key <= 'z')) ||
                    ((key >= '0') && (key <= '9')) ||
                    (key == '-') || (key == '_') || (key == '.'))) {
            engagement_reference[engagement_reference_length++] = (char)key;
            engagement_reference[engagement_reference_length] = '\0';
        } else if (key == PICOPEN_KEY_ENTER) {
            if (selection < 2u) {
                ++selection;
            } else {
                const uint64_t now_ms = time_us_64() / 1000u;
                const bool active = picopen_engagement_session_active(now_ms);
                const bool changed = active
                    ? picopen_engagement_session_deactivate(true)
                    : picopen_engagement_session_activate(
                          engagement_reference, now_ms,
                          durations_ms[engagement_duration_index], true);
                picopen_audit_record(active ? "scope.end" : "scope.start",
                                     changed);
                picopen_engagement_session_snapshot(&gui_state.engagement);
                gui_state.security.engagement_active =
                    picopen_engagement_is_active(&gui_state.engagement, now_ms);
            }
        }
        render_security();
        return;
    }
    if (screen == GUI_SKINS) {
        if ((key == PICOPEN_KEY_UP) && (selection > 0u)) --selection;
        if ((key == PICOPEN_KEY_DOWN) && (selection + 1u < PICOPEN_SKIN_COUNT)) ++selection;
        if (key == PICOPEN_KEY_ENTER) {
            (void)picopen_skin_select((picopen_skin_id_t)selection);
            picopen_crayon_renderer_invalidate();
            picopen_synthwave_renderer_invalidate();
            picopen_audit_record("skin.select", true);
            screen = GUI_HOME;
            selection = home_selection;
            render_home();
            return;
        }
        render_skins();
        return;
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
