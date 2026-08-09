#ifndef PICOPEN_DISPLAY_H
#define PICOPEN_DISPLAY_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

bool picopen_display_init(void);
void picopen_display_draw_diagnostic(void);
void picopen_display_fill_rect(uint16_t x, uint16_t y, uint16_t width,
                               uint16_t height, uint32_t rgb);
void picopen_display_blit_indexed8(uint16_t x, uint16_t y, uint16_t width,
                                   uint16_t height, const uint8_t *pixels,
                                   size_t stride, const uint32_t *palette,
                                   size_t palette_size);
void picopen_display_blit_indexed8_keyed(
    uint16_t x, uint16_t y, uint16_t width, uint16_t height,
    const uint8_t *pixels, size_t stride, const uint32_t *palette,
    size_t palette_size, uint8_t transparent_index);

#endif
