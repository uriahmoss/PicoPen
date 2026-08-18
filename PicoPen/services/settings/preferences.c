#include "picopen/preferences.h"
#include <stddef.h>
#include "picopen/internal_fs.h"
#define PREFERENCES_VERSION 3u
#define PREFERENCES_FILE "/settings.v1"
static picopen_preferences_t current;
typedef struct legacy_preferences_v2 {
    uint16_t version, size;
    uint8_t skin, home_selection, system_selection, files_selection;
    uint8_t backlight_percent;
    bool wifi_auto_enable;
    uint8_t security_mode;
    uint32_t checksum;
} legacy_preferences_v2_t;
_Static_assert(sizeof(legacy_preferences_v2_t)==16u,
               "preferences v2 migration layout changed");

static uint32_t checksum_bytes(const void *value, size_t length) {
    const uint8_t *bytes = value;
    uint32_t hash = UINT32_C(2166136261);
    for (size_t index=0u; index<length; ++index)
        hash = (hash ^ bytes[index]) * UINT32_C(16777619);
    return hash;
}
static uint32_t checksum(const picopen_preferences_t *preferences) {
    return checksum_bytes(preferences, offsetof(picopen_preferences_t, checksum));
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
    if (!picopen_internal_fs_read(PREFERENCES_FILE,&stored,sizeof(stored),&length)) return;
    if (length == sizeof(legacy_preferences_v2_t)) {
        const legacy_preferences_v2_t *legacy=(const legacy_preferences_v2_t *)&stored;
        if (legacy->version==2u && legacy->size==sizeof(*legacy) &&
            legacy->skin<4u && legacy->security_mode<PICOPEN_SECURITY_MODE_COUNT &&
            legacy->backlight_percent<=100u && legacy->checksum==checksum_bytes(
                legacy,offsetof(legacy_preferences_v2_t,checksum))) {
            current.skin=legacy->skin;current.home_selection=legacy->home_selection;
            current.system_selection=legacy->system_selection;
            current.files_selection=legacy->files_selection;
            current.backlight_percent=legacy->backlight_percent;
            current.wifi_auto_enable=legacy->wifi_auto_enable;
            current.security_mode=legacy->security_mode;
        }
        return;
    }
    if (length != sizeof(stored) || stored.version != PREFERENCES_VERSION ||
        stored.size != sizeof(stored) || stored.skin >= 4u ||
        stored.security_mode >= PICOPEN_SECURITY_MODE_COUNT ||
        stored.apps_selection >= 16u || stored.apps_filter >= 6u ||
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
bool picopen_preferences_set_security_mode(picopen_security_mode_t mode) {
    if (mode >= PICOPEN_SECURITY_MODE_COUNT) return false;
    current.security_mode = (uint8_t)mode;
    return save();
}
bool picopen_preferences_set_apps(size_t selection, uint8_t filter,
                                  uint16_t favorites) {
    if (selection >= 16u || filter >= 6u) return false;
    current.apps_selection = (uint8_t)selection;
    current.apps_filter = filter;
    current.favorite_apps = favorites;
    return save();
}
