#include "picopen/terminal.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "picopen/display.h"
#include "picopen/crayon_font.h"

#define TERMINAL_COLUMNS 40u
#define TERMINAL_ROWS    20u
#define CELL_WIDTH       8u
#define CELL_HEIGHT      16u
#define GLYPH_WIDTH      5u
#define GLYPH_HEIGHT     7u

#define COLOR_BACKGROUND UINT32_C(0x080020)
#define COLOR_FOREGROUND UINT32_C(0x00E5FF)
#define COLOR_CURSOR     UINT32_C(0xFF2BD6)

typedef struct terminal_cell {
    char character;
    bool dirty;
} terminal_cell_t;

static terminal_cell_t cells[TERMINAL_ROWS][TERMINAL_COLUMNS];
static uint8_t cursor_column;
static uint8_t cursor_row;
static uint8_t rendered_cursor_column;
static uint8_t rendered_cursor_row;
static bool rendered_cursor_valid;

static void scroll_up(void) {
    for (size_t row = 1u; row < TERMINAL_ROWS; ++row) {
        for (size_t column = 0u; column < TERMINAL_COLUMNS; ++column) {
            cells[row - 1u][column].character = cells[row][column].character;
            cells[row - 1u][column].dirty = true;
        }
    }
    for (size_t column = 0u; column < TERMINAL_COLUMNS; ++column) {
        cells[TERMINAL_ROWS - 1u][column].character = ' ';
        cells[TERMINAL_ROWS - 1u][column].dirty = true;
    }
    rendered_cursor_valid = false;
}

static const uint8_t glyphs[64][GLYPH_HEIGHT] = {
    ['!'-' '] = {4, 4, 4, 4, 4, 0, 4},
    ['"'-' '] = {10, 10, 10, 0, 0, 0, 0},
    ['#'-' '] = {10, 31, 10, 10, 31, 10, 0},
    ['$'-' '] = {4, 15, 20, 14, 5, 30, 4},
    ['%'-' '] = {24, 25, 2, 4, 8, 19, 3},
    ['&'-' '] = {12, 18, 20, 8, 21, 18, 13},
    ['\''-' '] = {4, 4, 8, 0, 0, 0, 0},
    ['('-' '] = {2, 4, 8, 8, 8, 4, 2},
    [')'-' '] = {8, 4, 2, 2, 2, 4, 8},
    ['*'-' '] = {0, 21, 14, 31, 14, 21, 0},
    ['+'-' '] = {0, 4, 4, 31, 4, 4, 0},
    [','-' '] = {0, 0, 0, 0, 4, 4, 8},
    ['-'-' '] = {0, 0, 0, 31, 0, 0, 0},
    ['.'-' '] = {0, 0, 0, 0, 0, 4, 4},
    ['/'-' '] = {1, 2, 4, 8, 16, 0, 0},
    ['0'-' '] = {14, 17, 19, 21, 25, 17, 14},
    ['1'-' '] = {4, 12, 4, 4, 4, 4, 14},
    ['2'-' '] = {14, 17, 1, 2, 4, 8, 31},
    ['3'-' '] = {30, 1, 1, 14, 1, 1, 30},
    ['4'-' '] = {2, 6, 10, 18, 31, 2, 2},
    ['5'-' '] = {31, 16, 16, 30, 1, 1, 30},
    ['6'-' '] = {14, 16, 16, 30, 17, 17, 14},
    ['7'-' '] = {31, 1, 2, 4, 8, 8, 8},
    ['8'-' '] = {14, 17, 17, 14, 17, 17, 14},
    ['9'-' '] = {14, 17, 17, 15, 1, 1, 14},
    [':'-' '] = {0, 4, 4, 0, 4, 4, 0},
    [';'-' '] = {0, 4, 4, 0, 4, 4, 8},
    ['<'-' '] = {2, 4, 8, 16, 8, 4, 2},
    ['='-' '] = {0, 0, 31, 0, 31, 0, 0},
    ['>'-' '] = {8, 4, 2, 1, 2, 4, 8},
    ['?'-' '] = {14, 17, 1, 2, 4, 0, 4},
    ['@'-' '] = {14, 17, 23, 21, 23, 16, 14},
    ['A'-' '] = {14, 17, 17, 31, 17, 17, 17},
    ['B'-' '] = {30, 17, 17, 30, 17, 17, 30},
    ['C'-' '] = {14, 17, 16, 16, 16, 17, 14},
    ['D'-' '] = {30, 17, 17, 17, 17, 17, 30},
    ['E'-' '] = {31, 16, 16, 30, 16, 16, 31},
    ['F'-' '] = {31, 16, 16, 30, 16, 16, 16},
    ['G'-' '] = {14, 17, 16, 23, 17, 17, 15},
    ['H'-' '] = {17, 17, 17, 31, 17, 17, 17},
    ['I'-' '] = {14, 4, 4, 4, 4, 4, 14},
    ['J'-' '] = {7, 2, 2, 2, 2, 18, 12},
    ['K'-' '] = {17, 18, 20, 24, 20, 18, 17},
    ['L'-' '] = {16, 16, 16, 16, 16, 16, 31},
    ['M'-' '] = {17, 27, 21, 21, 17, 17, 17},
    ['N'-' '] = {17, 25, 21, 19, 17, 17, 17},
    ['O'-' '] = {14, 17, 17, 17, 17, 17, 14},
    ['P'-' '] = {30, 17, 17, 30, 16, 16, 16},
    ['Q'-' '] = {14, 17, 17, 17, 21, 18, 13},
    ['R'-' '] = {30, 17, 17, 30, 20, 18, 17},
    ['S'-' '] = {15, 16, 16, 14, 1, 1, 30},
    ['T'-' '] = {31, 4, 4, 4, 4, 4, 4},
    ['U'-' '] = {17, 17, 17, 17, 17, 17, 14},
    ['V'-' '] = {17, 17, 17, 17, 17, 10, 4},
    ['W'-' '] = {17, 17, 17, 21, 21, 21, 10},
    ['X'-' '] = {17, 17, 10, 4, 10, 17, 17},
    ['Y'-' '] = {17, 17, 10, 4, 4, 4, 4},
    ['Z'-' '] = {31, 1, 2, 4, 8, 16, 31},
    ['['-' '] = {14, 8, 8, 8, 8, 8, 14},
    ['\\'-' '] = {16, 8, 4, 2, 1, 0, 0},
    [']'-' '] = {14, 2, 2, 2, 2, 2, 14},
    ['^'-' '] = {4, 10, 17, 0, 0, 0, 0},
    ['_'-' '] = {0, 0, 0, 0, 0, 0, 31},
};

static void newline(void) {
    cursor_column = 0u;
    if (cursor_row + 1u < TERMINAL_ROWS) {
        ++cursor_row;
        return;
    }
    scroll_up();
}

static size_t word_length(const char *text) {
    size_t length = 0u;
    while ((text[length] >= '!') && (text[length] <= '~')) {
        ++length;
    }
    return length;
}

static const uint8_t *glyph_for(char character) {
    static const uint8_t missing[GLYPH_HEIGHT] = {31, 17, 21, 21, 21, 17, 31};
    if ((character >= 'a') && (character <= 'z')) {
        character = (char)(character - ('a' - 'A'));
    }
    if ((character < ' ') || (character > '_')) {
        return missing;
    }
    if (character == ' ') {
        return glyphs[0];
    }
    const uint8_t *const glyph = glyphs[(size_t)(character - ' ')];
    bool empty = true;
    for (size_t row = 0u; row < GLYPH_HEIGHT; ++row) {
        empty = empty && (glyph[row] == 0u);
    }
    return empty ? missing : glyph;
}

static void draw_cell(uint8_t column, uint8_t row, char character) {
    const uint16_t x = (uint16_t)column * CELL_WIDTH;
    const uint16_t y = (uint16_t)row * CELL_HEIGHT;
    picopen_display_fill_rect(x, y, CELL_WIDTH, CELL_HEIGHT, COLOR_BACKGROUND);
    const uint8_t *const glyph = glyph_for(character);
    for (uint16_t glyph_row = 0u; glyph_row < GLYPH_HEIGHT; ++glyph_row) {
        for (uint16_t glyph_column = 0u; glyph_column < GLYPH_WIDTH;
             ++glyph_column) {
            if ((glyph[glyph_row] & (UINT8_C(1) <<
                 (GLYPH_WIDTH - 1u - glyph_column))) != 0u) {
                picopen_display_fill_rect((uint16_t)(x + 1u + glyph_column),
                    (uint16_t)(y + 1u + glyph_row * 2u), 1u, 2u,
                    COLOR_FOREGROUND);
            }
        }
    }
}

void picopen_terminal_init(void) {
    memset(cells, 0, sizeof(cells));
    for (size_t row = 0u; row < TERMINAL_ROWS; ++row) {
        for (size_t column = 0u; column < TERMINAL_COLUMNS; ++column) {
            cells[row][column].character = ' ';
            cells[row][column].dirty = true;
        }
    }
    cursor_column = 0u;
    cursor_row = 0u;
    rendered_cursor_valid = false;
}

void picopen_terminal_write(const char *text) {
    if (text == NULL) {
        return;
    }
    for (; *text != '\0'; ++text) {
        if (*text == '\n') {
            newline();
            continue;
        }
        if (*text == '\r') {
            cursor_column = 0u;
            continue;
        }
        if (*text == '\b') {
            if (cursor_column != 0u) {
                --cursor_column;
                cells[cursor_row][cursor_column].character = ' ';
                cells[cursor_row][cursor_column].dirty = true;
            }
            continue;
        }
        if ((*text < ' ') || (*text > '~')) {
            continue;
        }
        if (*text != ' ') {
            const size_t length = word_length(text);
            const size_t remaining = TERMINAL_COLUMNS - cursor_column;
            if ((cursor_column != 0u) && (length <= TERMINAL_COLUMNS) &&
                (length > remaining)) {
                newline();
            }
        }
        cells[cursor_row][cursor_column].character = *text;
        cells[cursor_row][cursor_column].dirty = true;
        ++cursor_column;
        if (cursor_column >= TERMINAL_COLUMNS) {
            newline();
        }
    }
}

void picopen_terminal_render(void) {
    if (rendered_cursor_valid &&
        ((rendered_cursor_column != cursor_column) ||
         (rendered_cursor_row != cursor_row))) {
        cells[rendered_cursor_row][rendered_cursor_column].dirty = true;
    }
    for (uint8_t row = 0u; row < TERMINAL_ROWS; ++row) {
        for (uint8_t column = 0u; column < TERMINAL_COLUMNS; ++column) {
            if (cells[row][column].dirty) {
                draw_cell(column, row, cells[row][column].character);
                cells[row][column].dirty = false;
            }
        }
    }
    picopen_display_fill_rect((uint16_t)cursor_column * CELL_WIDTH,
        (uint16_t)(cursor_row + 1u) * CELL_HEIGHT - 2u,
        CELL_WIDTH - 1u, 2u, COLOR_CURSOR);
    rendered_cursor_column = cursor_column;
    rendered_cursor_row = cursor_row;
    rendered_cursor_valid = true;
}

void picopen_terminal_draw_text_at(uint16_t x, uint16_t y, const char *text,
                                   uint8_t scale, uint32_t foreground,
                                   uint32_t background) {
    if ((text == NULL) || (scale == 0u) || (scale > 4u)) {
        return;
    }
    const uint16_t cell_width = (uint16_t)(GLYPH_WIDTH + 1u) * scale;
    const uint16_t cell_height = GLYPH_HEIGHT * scale;
    for (; *text != '\0'; ++text) {
        if (*text == '\n') {
            x = 0u;
            y = (uint16_t)(y + cell_height + scale);
            continue;
        }
        if (((uint32_t)x + cell_width > TERMINAL_COLUMNS * CELL_WIDTH) ||
            ((uint32_t)y + cell_height > TERMINAL_ROWS * CELL_HEIGHT)) {
            return;
        }
        picopen_display_fill_rect(x, y, cell_width, cell_height, background);
        const uint8_t *const glyph = glyph_for(*text);
        for (uint16_t row = 0u; row < GLYPH_HEIGHT; ++row) {
            for (uint16_t column = 0u; column < GLYPH_WIDTH; ++column) {
                if ((glyph[row] & (UINT8_C(1) <<
                     (GLYPH_WIDTH - 1u - column))) == 0u) {
                    continue;
                }
                picopen_display_fill_rect(
                    (uint16_t)(x + column * scale),
                    (uint16_t)(y + row * scale), scale, scale, foreground);
            }
        }
        x = (uint16_t)(x + cell_width);
    }
}

static void draw_crayon_text(uint16_t x, uint16_t y, const char *text,
                             uint8_t scale, uint32_t foreground,
                             uint32_t secondary, uint32_t background,
                             bool clear_background) {
    for (size_t character_index = 0u; *text != '\0'; ++text, ++character_index) {
        char character = *text;
        if ((character >= 'a') && (character <= 'z')) {
            character = (char)(character - ('a' - 'A'));
        }
        const uint16_t cell_width = (uint16_t)(PICOPEN_CRAYON_WIDTH * scale);
        const uint16_t cell_height = (uint16_t)(PICOPEN_CRAYON_HEIGHT * scale);
        if (((uint32_t)x + cell_width > 320u) ||
            ((uint32_t)y + cell_height > 320u)) {
            return;
        }
        if ((character < (char)PICOPEN_CRAYON_FIRST) ||
            (character > (char)PICOPEN_CRAYON_LAST)) {
            character = '?';
        }
        if (clear_background) {
            picopen_display_fill_rect(x, y, cell_width, cell_height,
                                      background);
        }
        const uint16_t *const rows =
            picopen_crayon_glyphs[(unsigned int)character -
                                   PICOPEN_CRAYON_FIRST];
        for (uint16_t row = 0u; row < PICOPEN_CRAYON_HEIGHT; ++row) {
            uint16_t column = 0u;
            while (column < PICOPEN_CRAYON_WIDTH) {
                const uint16_t mask = (uint16_t)(UINT16_C(1) <<
                    (PICOPEN_CRAYON_WIDTH - 1u - column));
                if ((rows[row] & mask) == 0u) {
                    ++column;
                    continue;
                }
                const uint16_t start = column;
                while ((column < PICOPEN_CRAYON_WIDTH) &&
                       ((rows[row] & (UINT16_C(1) <<
                        (PICOPEN_CRAYON_WIDTH - 1u - column))) != 0u)) {
                    ++column;
                }
                picopen_display_fill_rect((uint16_t)(x + start * scale),
                    (uint16_t)(y + row * scale),
                    (uint16_t)((column - start) * scale), scale, foreground);
                if (((row + character_index) % 5u) == 0u) {
                    picopen_display_fill_rect((uint16_t)(x + start * scale),
                        (uint16_t)(y + row * scale), scale, scale, secondary);
                }
            }
        }
        x = (uint16_t)(x + cell_width);
    }
}

void picopen_terminal_draw_crayon_text_transparent_at(
    uint16_t x, uint16_t y, const char *text, uint8_t scale,
    uint32_t foreground, uint32_t secondary) {
    if ((text == NULL) || (scale == 0u) || (scale > 4u)) {
        return;
    }
    draw_crayon_text(x, y, text, scale, foreground, secondary, 0u, false);
}

void picopen_terminal_draw_styled_text_at(
    uint16_t x, uint16_t y, const char *text, uint8_t scale,
    uint32_t foreground, uint32_t secondary, uint32_t background,
    picopen_text_style_t style) {
    if ((text == NULL) || (scale == 0u) || (scale > 4u)) {
        return;
    }
    if (style == PICOPEN_TEXT_CRAYON) {
        draw_crayon_text(x, y, text, scale, foreground, secondary, background,
                          true);
        return;
    }
    const uint16_t cell_width = (uint16_t)(GLYPH_WIDTH + 1u) * scale;
    const uint16_t cell_height = GLYPH_HEIGHT * scale;
    size_t character_index = 0u;
    for (; *text != '\0'; ++text, ++character_index) {
        if (((uint32_t)x + cell_width > TERMINAL_COLUMNS * CELL_WIDTH) ||
            ((uint32_t)y + cell_height > TERMINAL_ROWS * CELL_HEIGHT)) {
            return;
        }
        picopen_display_fill_rect(x, y, cell_width, cell_height, background);
        const uint8_t *const glyph = glyph_for(*text);
        if (style == PICOPEN_TEXT_NEON) {
            for (uint16_t row = 0u; row < GLYPH_HEIGHT; ++row) {
                for (uint16_t column = 0u; column < GLYPH_WIDTH; ++column) {
                    if ((glyph[row] & (UINT8_C(1) <<
                         (GLYPH_WIDTH - 1u - column))) != 0u) {
                        picopen_display_fill_rect(
                            (uint16_t)(x + column * scale),
                            (uint16_t)(y + row * scale),
                            (uint16_t)(scale + 1u), (uint16_t)(scale + 1u),
                            secondary);
                    }
                }
            }
        }
        for (uint16_t row = 0u; row < GLYPH_HEIGHT; ++row) {
            for (uint16_t column = 0u; column < GLYPH_WIDTH; ++column) {
                if ((glyph[row] & (UINT8_C(1) <<
                     (GLYPH_WIDTH - 1u - column))) == 0u) {
                    continue;
                }
                uint16_t px = (uint16_t)(x + column * scale);
                uint16_t py = (uint16_t)(y + row * scale);
                uint16_t width = scale;
                uint16_t height = scale;
                if (style == PICOPEN_TEXT_CRAYON) {
                    const uint16_t variation = (uint16_t)(character_index +
                        row * 3u + column * 5u);
                    px = (uint16_t)(px + (variation & 1u));
                    if ((scale > 1u) && ((variation % 5u) == 0u)) {
                        width = (uint16_t)(scale - 1u);
                    }
                }
                picopen_display_fill_rect(px, py, width, height, foreground);
                if ((style == PICOPEN_TEXT_CRAYON) && (scale > 1u) &&
                    (((row + column + character_index) % 4u) == 0u)) {
                    picopen_display_fill_rect(px, py, 1u, 1u, secondary);
                }
            }
        }
        x = (uint16_t)(x + cell_width);
    }
}
