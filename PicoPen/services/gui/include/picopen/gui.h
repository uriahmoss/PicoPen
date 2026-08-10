#ifndef PICOPEN_GUI_H
#define PICOPEN_GUI_H

#include <stdint.h>

#include "picopen/shell.h"
typedef enum picopen_gui_storage_action { PICOPEN_GUI_STORAGE_NONE=0,
    PICOPEN_GUI_STORAGE_SAFE_REMOVE, PICOPEN_GUI_STORAGE_RESCAN } picopen_gui_storage_action_t;

void picopen_gui_init(const picopen_shell_state_t *state);
void picopen_gui_show_boot_status(const char *stage, const char *status);
void picopen_gui_update_state(const picopen_shell_state_t *state);
void picopen_gui_refresh_workbench(void);
void picopen_gui_handle_key(uint8_t key);
picopen_gui_storage_action_t picopen_gui_take_storage_action(void);

#endif
