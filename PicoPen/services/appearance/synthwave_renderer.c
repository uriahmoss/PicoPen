#include "picopen/synthwave_renderer.h"

#include <stdbool.h>
#include <string.h>

#include "picopen/display.h"
#include "picopen/terminal.h"

#define VOID_COLOR  UINT32_C(0x080020)
#define PANEL_COLOR UINT32_C(0x101040)
#define CYAN_COLOR  UINT32_C(0x00E5FF)
#define PINK_COLOR  UINT32_C(0xFF2BD6)
#define LIME_COLOR  UINT32_C(0x7CFF00)
#define GRID_COLOR  UINT32_C(0x240046)
#define GRID_DARK   UINT32_C(0x180036)
#define MUTED_COLOR UINT32_C(0x7590B8)

static bool page_valid;
static uint8_t cached_screen;
static char cached_lines[PICOPEN_RENDER_PAGE_ROWS][PICOPEN_RENDER_LINE_SIZE];

static void rect_outline(uint16_t x, uint16_t y, uint16_t width,
                         uint16_t height, uint16_t thickness,
                         uint32_t color) {
    picopen_display_fill_rect(x, y, width, thickness, color);
    picopen_display_fill_rect(x, (uint16_t)(y + height - thickness), width,
                              thickness, color);
    picopen_display_fill_rect(x, y, thickness, height, color);
    picopen_display_fill_rect((uint16_t)(x + width - thickness), y, thickness,
                              height, color);
}

static void text(uint16_t x, uint16_t y, const char *value, uint8_t scale,
                 uint32_t color, uint32_t background) {
    picopen_terminal_draw_styled_text_at(x, y, value, scale, color,
        PINK_COLOR, background, PICOPEN_TEXT_NEON);
}

static void grid(uint16_t y, uint16_t height) {
    picopen_display_fill_rect(0u, y, 320u, height, VOID_COLOR);
    const uint16_t end = (uint16_t)(y + height);
    for (uint16_t gy = 44u; gy < 284u; gy += 24u) {
        if ((gy >= y) && (gy < end)) {
            picopen_display_fill_rect(0u, gy, 320u, 1u, GRID_COLOR);
        }
    }
    for (uint16_t gx = 0u; gx < 320u; gx += 40u) {
        picopen_display_fill_rect(gx, y, 1u, height, GRID_DARK);
    }
}

static void icon(size_t index, uint16_t x, uint16_t y, uint32_t color) {
    if (index == 0u) {
        for (uint16_t bar = 0u; bar < 4u; ++bar) {
            picopen_display_fill_rect((uint16_t)(x + bar * 9u),
                (uint16_t)(y + 18u - bar * 4u), 5u,
                (uint16_t)(5u + bar * 4u), color);
        }
        return;
    }
    rect_outline(x, y, 34u, 23u, 2u, color);
    picopen_display_fill_rect((uint16_t)(x + 7u), (uint16_t)(y + 7u),
                              (uint16_t)(10u + (index % 3u) * 5u), 2u, color);
    picopen_display_fill_rect((uint16_t)(x + 7u), (uint16_t)(y + 14u),
                              14u, 2u, color);
}

static void tile(const char *label, size_t index, bool focused) {
    const uint16_t x = (index & 1u) == 0u ? 8u : 164u;
    const uint16_t y = (uint16_t)(50u + (index / 2u) * 76u);
    picopen_display_fill_rect(x, y, 148u, 66u, PANEL_COLOR);
    rect_outline(x, y, 148u, 66u, focused ? 3u : 2u,
                 focused ? LIME_COLOR : ((index & 1u) ? CYAN_COLOR : PINK_COLOR));
    rect_outline((uint16_t)(x + 4u), (uint16_t)(y + 4u), 140u, 58u, 1u,
                 focused ? PINK_COLOR : GRID_COLOR);
    icon(index, (uint16_t)(x + 57u), (uint16_t)(y + 8u),
         focused ? LIME_COLOR : CYAN_COLOR);
    const uint16_t width = (uint16_t)(strlen(label) * 12u);
    text((uint16_t)(x + (148u - width) / 2u), (uint16_t)(y + 41u), label, 2u,
         focused ? LIME_COLOR : CYAN_COLOR, PANEL_COLOR);
}

void picopen_synthwave_renderer_invalidate(void) {
    page_valid = false;
}

void picopen_synthwave_renderer_home(const char *const labels[], size_t count,
                                     size_t selected, bool scope_active) {
    page_valid = false;
    grid(0u, 320u);
    picopen_display_fill_rect(0u, 0u, 320u, 44u, UINT32_C(0x12002F));
    picopen_display_fill_rect(0u, 43u, 320u, 1u, PINK_COLOR);
    text(8u, 10u, "PICOPEN", 2u, CYAN_COLOR, UINT32_C(0x12002F));
    text(216u, 8u, scope_active ? "SCOPE ON" : "SCOPE OFF", 1u,
         scope_active ? LIME_COLOR : PINK_COLOR, UINT32_C(0x12002F));
    text(216u, 23u, scope_active ? "SESSION" : "LOCKED", 1u, MUTED_COLOR,
         UINT32_C(0x12002F));
    for (size_t index = 0u; index < count; ++index) {
        tile(labels[index], index, index == selected);
    }
    picopen_display_fill_rect(0u, 282u, 320u, 38u, UINT32_C(0x12002F));
    text(10u, 294u, "ARROWS MOVE", 1u, CYAN_COLOR, UINT32_C(0x12002F));
    text(220u, 294u, "SELECT", 1u, CYAN_COLOR, UINT32_C(0x12002F));
}

void picopen_synthwave_renderer_home_focus(const char *const labels[],
                                           size_t previous, size_t selected) {
    tile(labels[previous], previous, false);
    tile(labels[selected], selected, true);
}

static void page_row(uint16_t row, const char *line) {
    const uint16_t y = (uint16_t)(row * 16u);
    grid(y, 16u);
    uint32_t background = VOID_COLOR;
    if (row == 0u) {
        picopen_display_fill_rect(0u, y, 320u, 16u, UINT32_C(0x12002F));
        background = UINT32_C(0x12002F);
    } else if (line[0] == '>') {
        picopen_display_fill_rect(2u, y, 316u, 16u, PANEL_COLOR);
        rect_outline(2u, y, 316u, 16u, 2u, LIME_COLOR);
        background = PANEL_COLOR;
    } else if ((strlen(line) >= 8u) && line[0] == '=') {
        picopen_display_fill_rect(0u, (uint16_t)(y + 7u), 320u, 2u,
                                  PINK_COLOR);
        return;
    }
    text(4u, (uint16_t)(y + 3u), line, 1u, CYAN_COLOR, background);
}

void picopen_synthwave_renderer_page(
    uint8_t screen_key,
    const char lines[PICOPEN_RENDER_PAGE_ROWS][PICOPEN_RENDER_LINE_SIZE]) {
    const bool full = !page_valid || cached_screen != screen_key;
    if (full) {
        grid(0u, 320u);
    }
    for (uint16_t row = 0u; row < PICOPEN_RENDER_PAGE_ROWS; ++row) {
        if (full || strcmp(lines[row], cached_lines[row]) != 0) {
            page_row(row, lines[row]);
            strcpy(cached_lines[row], lines[row]);
        }
    }
    page_valid = true;
    cached_screen = screen_key;
}
