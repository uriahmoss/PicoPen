#ifndef PICOPEN_TERMINAL_H
#define PICOPEN_TERMINAL_H

#include <stddef.h>

void picopen_terminal_init(void);
void picopen_terminal_write(const char *text);
void picopen_terminal_render(void);

#endif
