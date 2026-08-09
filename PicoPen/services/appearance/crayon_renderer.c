#include "picopen/crayon_renderer.h"

#include <stdbool.h>
#include <string.h>

#include "picopen/display.h"
#include "picopen/crayon_screen_asset.h"
#include "picopen/terminal.h"

#define PAPER       UINT32_C(0xF4E6C1)
#define PAPER_DARK  UINT32_C(0xE5CFA0)
#define INK         UINT32_C(0x263653)
#define MUTED_INK   UINT32_C(0x71634E)

static const uint32_t crayons[] = {
    UINT32_C(0x315B9A), UINT32_C(0x7E3F98), UINT32_C(0xC43E78),
    UINT32_C(0xE66A24), UINT32_C(0x288D4B), UINT32_C(0xD4463F),
};

static bool page_valid;
static uint8_t cached_screen;
static char cached_entries[12][PICOPEN_RENDER_LINE_SIZE];
static bool cached_focus[12];
static size_t cached_entry_count;

static const uint16_t home_focus_x[6] = {3u, 171u, 3u, 171u, 3u, 171u};
static const uint16_t home_focus_y[6] = {52u, 52u, 125u, 125u, 200u, 200u};

static void paper(uint16_t y, uint16_t height) {
    picopen_display_fill_rect(0u, y, 320u, height, PAPER);
    const uint16_t end = (uint16_t)(y + height);
    for (uint16_t py = (uint16_t)(y + 3u); py < end; py += 11u) {
        for (uint16_t px = (uint16_t)((py * 7u) % 19u); px < 320u;
             px += 23u) {
            picopen_display_fill_rect(px, py, 1u, 1u, PAPER_DARK);
        }
    }
}

static void restore_home_focus(size_t index) {
    const uint16_t x = home_focus_x[index];
    const uint16_t y = home_focus_y[index];
    picopen_display_blit_indexed8(
        x, y, PICOPEN_CRAYON_FOCUS_WIDTH, PICOPEN_CRAYON_FOCUS_HEIGHT,
        &picopen_crayon_screen_pixels[(size_t)y * PICOPEN_CRAYON_SCREEN_WIDTH + x],
        PICOPEN_CRAYON_SCREEN_WIDTH, picopen_crayon_screen_palette,
        PICOPEN_CRAYON_SCREEN_COLORS);
}

static void draw_home_focus(size_t index) {
    picopen_display_blit_indexed8(
        home_focus_x[index], home_focus_y[index], PICOPEN_CRAYON_FOCUS_WIDTH,
        PICOPEN_CRAYON_FOCUS_HEIGHT, picopen_crayon_selected_pixels[index],
        PICOPEN_CRAYON_FOCUS_WIDTH, picopen_crayon_screen_palette,
        PICOPEN_CRAYON_SCREEN_COLORS);
}

void picopen_crayon_renderer_invalidate(void) {
    page_valid = false;
}

void picopen_crayon_renderer_home(const char *const labels[], size_t count,
                                  size_t selected, bool scope_active) {
    (void)labels;
    (void)count;
    page_valid = false;
    picopen_display_blit_indexed8(
        0u, 0u, PICOPEN_CRAYON_SCREEN_WIDTH, PICOPEN_CRAYON_SCREEN_HEIGHT,
        picopen_crayon_screen_pixels, PICOPEN_CRAYON_SCREEN_WIDTH,
        picopen_crayon_screen_palette, PICOPEN_CRAYON_SCREEN_COLORS);
    picopen_terminal_draw_crayon_text_transparent_at(
        235u, 10u, scope_active ? "SCOPE ON" : "SCOPE OFF", 1u,
        scope_active ? crayons[4] : crayons[2], crayons[0]);
    draw_home_focus(selected);
}

void picopen_crayon_renderer_home_focus(const char *const labels[],
                                        size_t previous, size_t selected) {
    (void)labels;
    restore_home_focus(previous);
    restore_home_focus(selected);
    draw_home_focus(selected);
}

static bool footer_line(const char *line) {
    return strstr(line, "ESC BACK") != NULL ||
           strstr(line, "ENTER SELECT") != NULL ||
           strstr(line, "UP/DOWN") != NULL;
}

static void clean_entry(char output[PICOPEN_RENDER_LINE_SIZE],
                        const char *line) {
    size_t output_length = 0u;
    while ((*line == ' ') || (*line == '>')) {
        ++line;
    }
    for (; (*line != '\0') && (output_length < PICOPEN_RENDER_LINE_SIZE - 1u);
         ++line) {
        if ((*line == '[') || (*line == ']')) {
            continue;
        }
        output[output_length++] = *line;
    }
    while ((output_length > 0u) && (output[output_length - 1u] == ' ')) {
        --output_length;
    }
    output[output_length] = '\0';
}

static size_t build_entries(
    const char lines[PICOPEN_RENDER_PAGE_ROWS][PICOPEN_RENDER_LINE_SIZE],
    char entries[12][PICOPEN_RENDER_LINE_SIZE], bool focused[12]) {
    size_t count = 0u;
    for (size_t row = 2u; (row < PICOPEN_RENDER_PAGE_ROWS) && (count < 12u);
         ++row) {
        if ((lines[row][0] == '\0') || footer_line(lines[row])) {
            continue;
        }
        focused[count] = lines[row][0] == '>';
        clean_entry(entries[count], lines[row]);
        if (entries[count][0] != '\0') {
            ++count;
        }
    }
    return count;
}

static void draw_page_header(const char *title) {
    paper(0u, 54u);
    picopen_terminal_draw_crayon_text_transparent_at(
        14u, 10u, title, 2u, crayons[2], crayons[3]);
    for (uint16_t x = 12u; x < 306u; x += 11u) {
        picopen_display_fill_rect(x, (uint16_t)(46u + ((x / 11u) % 3u)),
                                  13u, 2u, crayons[3]);
    }
    picopen_display_fill_rect(276u, 12u, 5u, 24u, crayons[0]);
    picopen_display_fill_rect(286u, 9u, 5u, 27u, crayons[4]);
    picopen_display_fill_rect(296u, 14u, 5u, 22u, crayons[5]);
}

static void draw_page_entry(size_t slot, const char *entry, bool focused) {
    const uint16_t y = (uint16_t)(60u + slot * 20u);
    paper(y, 19u);
    if (focused) {
        picopen_display_blit_indexed8_keyed(
            9u, y, PICOPEN_CRAYON_FOCUS_WIDTH, 19u,
            picopen_crayon_focus_pixels[slot % 6u],
            PICOPEN_CRAYON_FOCUS_WIDTH, picopen_crayon_focus_palette, 5u,
            255u);
        picopen_display_blit_indexed8_keyed(
            155u, y, PICOPEN_CRAYON_FOCUS_WIDTH, 19u,
            picopen_crayon_focus_pixels[(slot + 2u) % 6u],
            PICOPEN_CRAYON_FOCUS_WIDTH, picopen_crayon_focus_palette, 5u,
            255u);
    } else {
        for (uint16_t x = 16u; x < 300u; x += 17u) {
            picopen_display_fill_rect(x,
                (uint16_t)(y + 16u + ((x / 17u + slot) & 1u)), 19u, 1u,
                crayons[slot % 6u]);
        }
    }
    picopen_terminal_draw_crayon_text_transparent_at(
        18u, (uint16_t)(y + 2u), entry, 1u, INK,
        focused ? UINT32_C(0xFFFFFF) : crayons[slot % 6u]);
}

static void draw_page_footer(void) {
    paper(288u, 32u);
    picopen_terminal_draw_crayon_text_transparent_at(
        12u, 298u, "ARROWS MOVE", 1u, MUTED_INK, crayons[0]);
    picopen_terminal_draw_crayon_text_transparent_at(
        222u, 298u, "ESC BACK", 1u, MUTED_INK, crayons[5]);
}

void picopen_crayon_renderer_page(
    uint8_t screen_key, const char *title,
    const char lines[PICOPEN_RENDER_PAGE_ROWS][PICOPEN_RENDER_LINE_SIZE]) {
    char entries[12][PICOPEN_RENDER_LINE_SIZE] = {{0}};
    bool focused[12] = {false};
    const size_t entry_count = build_entries(lines, entries, focused);
    const bool full = !page_valid || cached_screen != screen_key;
    if (full) {
        paper(0u, 320u);
        draw_page_header(title);
        draw_page_footer();
    }
    const size_t redraw_count = entry_count > cached_entry_count
        ? entry_count : cached_entry_count;
    for (size_t slot = 0u; slot < redraw_count; ++slot) {
        const bool exists = slot < entry_count;
        const bool changed = full || !exists || slot >= cached_entry_count ||
            strcmp(entries[slot], cached_entries[slot]) != 0 ||
            focused[slot] != cached_focus[slot];
        if (!changed) {
            continue;
        }
        if (exists) {
            draw_page_entry(slot, entries[slot], focused[slot]);
            strcpy(cached_entries[slot], entries[slot]);
            cached_focus[slot] = focused[slot];
        } else {
            paper((uint16_t)(60u + slot * 20u), 19u);
            cached_entries[slot][0] = '\0';
            cached_focus[slot] = false;
        }
    }
    cached_entry_count = entry_count;
    page_valid = true;
    cached_screen = screen_key;
}
