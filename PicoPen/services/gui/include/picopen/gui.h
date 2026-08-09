#ifndef PICOPEN_GUI_H
#define PICOPEN_GUI_H

#include <stdint.h>

#include "picopen/shell.h"

void picopen_gui_init(const picopen_shell_state_t *state);
void picopen_gui_handle_key(uint8_t key);

#endif
