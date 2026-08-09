#include "picopen/skin.h"

static const picopen_skin_t skins[PICOPEN_SKIN_COUNT] = {
    [PICOPEN_SKIN_SYNTHWAVE] = {
        .name = "SYNTHWAVE", .background = 0x080020u, .panel = 0x101040u,
        .header = 0x12002Fu, .text = 0x00E5FFu, .focus = 0x7CFF00u,
        .muted = 0x7590B8u,
        .accents = {0xFF2BD6u, 0x00E5FFu, 0x00E5FFu, 0x8B5CF6u,
                    0xFF2BD6u, 0xFF8A30u},
        .style = PICOPEN_SKIN_STYLE_NEON,
    },
    [PICOPEN_SKIN_CRAYON] = {
        .name = "CRAYON", .background = 0xF2E3BCu, .panel = 0xF2E3BCu,
        .header = 0xE8D4A8u, .text = 0x14284Bu, .focus = 0x18BBD1u,
        .muted = 0x675A49u,
        .accents = {0x14284Bu, 0x70358Fu, 0xC02672u, 0xE66A12u,
                    0x198C38u, 0xD32F2Fu},
        .textured_focus = true,
        .style = PICOPEN_SKIN_STYLE_CRAYON,
    },
    [PICOPEN_SKIN_HIGH_CONTRAST] = {
        .name = "HIGH CONTRAST", .background = 0x000000u, .panel = 0x000000u,
        .header = 0x000000u, .text = 0xFFFFFFu, .focus = 0xFFFF00u,
        .muted = 0xC0C0C0u,
        .accents = {0xFFFFFFu, 0xFFFFFFu, 0xFFFFFFu, 0xFFFFFFu,
                    0xFFFFFFu, 0xFFFFFFu},
        .style = PICOPEN_SKIN_STYLE_CONTRAST,
    },
    [PICOPEN_SKIN_MINIMAL_DARK] = {
        .name = "MINIMAL DARK", .background = 0x101418u, .panel = 0x202830u,
        .header = 0x181E24u, .text = 0xE8EEF2u, .focus = 0x55B8FFu,
        .muted = 0x81909Au,
        .accents = {0x81909Au, 0x81909Au, 0x81909Au, 0x81909Au,
                    0x81909Au, 0x81909Au},
        .style = PICOPEN_SKIN_STYLE_MINIMAL,
    },
};

static picopen_skin_id_t current_skin;

void picopen_skin_init(void) {
    current_skin = PICOPEN_SKIN_SYNTHWAVE;
}

const picopen_skin_t *picopen_skin_current(void) {
    return &skins[current_skin];
}

picopen_skin_id_t picopen_skin_current_id(void) {
    return current_skin;
}

bool picopen_skin_select(picopen_skin_id_t identifier) {
    if ((unsigned int)identifier >= PICOPEN_SKIN_COUNT) {
        return false;
    }
    current_skin = identifier;
    return true;
}

const picopen_skin_t *picopen_skin_get(picopen_skin_id_t identifier) {
    return (unsigned int)identifier < PICOPEN_SKIN_COUNT
        ? &skins[identifier] : NULL;
}
