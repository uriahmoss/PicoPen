#ifndef PICOPEN_CAPABILITY_H
#define PICOPEN_CAPABILITY_H

#include <stdbool.h>
#include <stdint.h>

typedef enum picopen_capability {
    PICOPEN_CAP_STORAGE_READ = 0,
    PICOPEN_CAP_STORAGE_WRITE,
    PICOPEN_CAP_NETWORK_CONNECT,
    PICOPEN_CAP_NETWORK_PROBE,
    PICOPEN_CAP_RADIO_RECEIVE,
    PICOPEN_CAP_RADIO_TRANSMIT,
    PICOPEN_CAP_GPIO_READ,
    PICOPEN_CAP_GPIO_DRIVE,
    PICOPEN_CAP_USB_HID,
    PICOPEN_CAP_TARGET_POWER,
    PICOPEN_CAP_REMOTE_CONTROL,
    PICOPEN_CAPABILITY_COUNT,
} picopen_capability_t;

typedef struct picopen_security_context {
    uint64_t grants;
    bool engagement_active;
} picopen_security_context_t;

picopen_security_context_t picopen_security_default(void);
bool picopen_security_authorize(const picopen_security_context_t *context,
                                picopen_capability_t capability,
                                bool local_confirmation);
const char *picopen_capability_name(picopen_capability_t capability);

#endif
