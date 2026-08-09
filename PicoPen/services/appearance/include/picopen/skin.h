#ifndef PICOPEN_SKIN_H
#define PICOPEN_SKIN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum picopen_skin_id {
    PICOPEN_SKIN_SYNTHWAVE = 0,
    PICOPEN_SKIN_CRAYON,
    PICOPEN_SKIN_HIGH_CONTRAST,
    PICOPEN_SKIN_MINIMAL_DARK,
    PICOPEN_SKIN_COUNT,
} picopen_skin_id_t;

typedef enum picopen_skin_style {
    PICOPEN_SKIN_STYLE_NEON = 0,
    PICOPEN_SKIN_STYLE_CRAYON,
    PICOPEN_SKIN_STYLE_CONTRAST,
    PICOPEN_SKIN_STYLE_MINIMAL,
} picopen_skin_style_t;

typedef struct picopen_skin {
    const char *name;
    uint32_t background;
    uint32_t panel;
    uint32_t header;
    uint32_t text;
    uint32_t focus;
    uint32_t muted;
    uint32_t accents[6];
    picopen_skin_style_t style;
    bool textured_focus;
} picopen_skin_t;

void picopen_skin_init(void);
const picopen_skin_t *picopen_skin_current(void);
picopen_skin_id_t picopen_skin_current_id(void);
bool picopen_skin_select(picopen_skin_id_t identifier);
const picopen_skin_t *picopen_skin_get(picopen_skin_id_t identifier);

#endif
