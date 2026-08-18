#ifndef PICOPEN_PREFERENCES_H
#define PICOPEN_PREFERENCES_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef enum picopen_security_mode { PICOPEN_SECURITY_OWNER=0,
    PICOPEN_SECURITY_GUARDED, PICOPEN_SECURITY_DEVELOPER,
    PICOPEN_SECURITY_MODE_COUNT } picopen_security_mode_t;
typedef struct picopen_preferences {
    uint16_t version, size;
    uint8_t skin, home_selection, system_selection, files_selection;
    uint8_t backlight_percent;
    bool wifi_auto_enable;
    uint8_t security_mode;
    uint8_t apps_selection;
    uint8_t apps_filter;
    uint16_t favorite_apps;
    uint32_t checksum;
} picopen_preferences_t;
void picopen_preferences_init(void);
void picopen_preferences_get(picopen_preferences_t *preferences);
bool picopen_preferences_set_skin(uint8_t skin);
bool picopen_preferences_set_menu(size_t home, size_t system, size_t files);
bool picopen_preferences_set_backlight(uint8_t percent);
bool picopen_preferences_set_wifi_auto_enable(bool enabled);
bool picopen_preferences_set_security_mode(picopen_security_mode_t mode);
bool picopen_preferences_set_apps(size_t selection, uint8_t filter,
                                  uint16_t favorites);
#endif
