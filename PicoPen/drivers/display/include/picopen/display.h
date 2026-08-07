#ifndef PICOPEN_DISPLAY_H
#define PICOPEN_DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

bool picopen_display_init(void);
void picopen_display_draw_diagnostic(void);
void picopen_display_fill_rect(uint16_t x, uint16_t y, uint16_t width,
                               uint16_t height, uint32_t rgb);

#endif
