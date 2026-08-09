#ifndef PICOPEN_TERMINAL_H
#define PICOPEN_TERMINAL_H

#include <stddef.h>
#include <stdint.h>

typedef enum picopen_text_style {
    PICOPEN_TEXT_PIXEL = 0,
    PICOPEN_TEXT_NEON,
    PICOPEN_TEXT_CRAYON,
} picopen_text_style_t;

void picopen_terminal_init(void);
void picopen_terminal_write(const char *text);
void picopen_terminal_render(void);
void picopen_terminal_draw_text_at(uint16_t x, uint16_t y, const char *text,
                                   uint8_t scale, uint32_t foreground,
                                   uint32_t background);
void picopen_terminal_draw_styled_text_at(
    uint16_t x, uint16_t y, const char *text, uint8_t scale,
    uint32_t foreground, uint32_t secondary, uint32_t background,
    picopen_text_style_t style);
void picopen_terminal_draw_crayon_text_transparent_at(
    uint16_t x, uint16_t y, const char *text, uint8_t scale,
    uint32_t foreground, uint32_t secondary);

#endif
