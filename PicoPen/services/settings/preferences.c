#include "picopen/preferences.h"
#include <stddef.h>
#include "picopen/internal_fs.h"
#define PREFERENCES_VERSION 1u
#define PREFERENCES_FILE "/settings.v1"
static picopen_preferences_t current;
static uint32_t checksum(const picopen_preferences_t *preferences) {
    const uint8_t *bytes = (const uint8_t *)preferences;
    uint32_t hash = UINT32_C(2166136261);
    for (size_t index = 0u; index < offsetof(picopen_preferences_t, checksum); ++index)
        hash = (hash ^ bytes[index]) * UINT32_C(16777619);
    return hash;
}
static picopen_preferences_t defaults(void) {
    picopen_preferences_t value = {.version=PREFERENCES_VERSION,
        .size=sizeof(picopen_preferences_t), .backlight_percent=100u};
    value.checksum = checksum(&value); return value;
}
static bool save(void) {
    if (picopen_internal_fs_state() != PICOPEN_INTERNAL_FS_READY) return false;
    current.checksum = checksum(&current);
    return picopen_internal_fs_replace(PREFERENCES_FILE, &current, sizeof(current));
}
void picopen_preferences_init(void) {
    current = defaults(); picopen_preferences_t stored; size_t length=0u;
    if (!picopen_internal_fs_read(PREFERENCES_FILE,&stored,sizeof(stored),&length) ||
        length != sizeof(stored) || stored.version != PREFERENCES_VERSION ||
        stored.size != sizeof(stored) || stored.skin >= 4u ||
        stored.backlight_percent > 100u || stored.checksum != checksum(&stored)) return;
    current = stored;
}
void picopen_preferences_get(picopen_preferences_t *value) { if (value) *value=current; }
bool picopen_preferences_set_skin(uint8_t skin) { if (skin>=4u) return false; current.skin=skin; return save(); }
bool picopen_preferences_set_menu(size_t home,size_t system,size_t files) {
    if (home>=6u || system>=7u || files>=12u) return false;
    current.home_selection=(uint8_t)home; current.system_selection=(uint8_t)system;
    current.files_selection=(uint8_t)files; return save();
}
bool picopen_preferences_set_backlight(uint8_t percent) { if(percent>100u)return false; current.backlight_percent=percent; return save(); }
bool picopen_preferences_set_wifi_auto_enable(bool enabled) { current.wifi_auto_enable=enabled; return save(); }
