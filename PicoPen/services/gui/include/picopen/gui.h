#ifndef PICOPEN_GUI_H
#define PICOPEN_GUI_H

#include <stdint.h>

#include "picopen/shell.h"

void picopen_gui_init(const picopen_shell_state_t *state);
void picopen_gui_show_boot_status(const char *stage, const char *status);
void picopen_gui_update_state(const picopen_shell_state_t *state);
void picopen_gui_refresh_workbench(void);
void picopen_gui_handle_key(uint8_t key);

#endif
