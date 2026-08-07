#ifndef PICOPEN_CONSOLE_H
#define PICOPEN_CONSOLE_H

#include <stdbool.h>

typedef struct picopen_console_state {
    bool watchdog_reset;
} picopen_console_state_t;

void picopen_console_init(const picopen_console_state_t *initial_state);
void picopen_console_poll(void);

#endif
