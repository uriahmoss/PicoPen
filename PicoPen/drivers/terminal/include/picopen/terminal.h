#ifndef PICOPEN_TERMINAL_H
#define PICOPEN_TERMINAL_H

#include <stddef.h>
#include <stdint.h>

void picopen_terminal_init(void);
void picopen_terminal_write(const char *text);
void picopen_terminal_render(void);
void picopen_terminal_draw_text_at(uint16_t x, uint16_t y, const char *text,
                                   uint8_t scale, uint32_t foreground,
                                   uint32_t background);

#endif
