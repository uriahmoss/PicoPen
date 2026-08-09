#ifndef PICOPEN_SYNTHWAVE_RENDERER_H
#define PICOPEN_SYNTHWAVE_RENDERER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "picopen/renderer_model.h"

void picopen_synthwave_renderer_invalidate(void);
void picopen_synthwave_renderer_home(const char *const labels[], size_t count,
                                     size_t selected, bool scope_active);
void picopen_synthwave_renderer_home_focus(const char *const labels[],
                                           size_t previous, size_t selected);
void picopen_synthwave_renderer_page(
    uint8_t screen_key,
    const char lines[PICOPEN_RENDER_PAGE_ROWS][PICOPEN_RENDER_LINE_SIZE]);

#endif
