#include "picopen/capability.h"

#include <stddef.h>

static bool requires_engagement(picopen_capability_t capability) {
    return (capability == PICOPEN_CAP_NETWORK_PROBE) ||
           (capability == PICOPEN_CAP_RADIO_TRANSMIT) ||
           (capability == PICOPEN_CAP_GPIO_DRIVE) ||
           (capability == PICOPEN_CAP_USB_HID) ||
           (capability == PICOPEN_CAP_TARGET_POWER) ||
           (capability == PICOPEN_CAP_REMOTE_CONTROL);
}

static bool requires_local_confirmation(picopen_capability_t capability) {
    return (capability == PICOPEN_CAP_NETWORK_CONNECT) ||
           (capability == PICOPEN_CAP_RADIO_TRANSMIT) ||
           (capability == PICOPEN_CAP_GPIO_DRIVE) ||
           (capability == PICOPEN_CAP_USB_HID) ||
           (capability == PICOPEN_CAP_TARGET_POWER) ||
           (capability == PICOPEN_CAP_REMOTE_CONTROL) ||
           (capability == PICOPEN_CAP_SYSTEM_SHUTDOWN);
}

picopen_security_context_t picopen_security_default(void) {
    return (picopen_security_context_t){0};
}

bool picopen_security_authorize(const picopen_security_context_t *context,
                                picopen_capability_t capability,
                                bool local_confirmation) {
    if ((context == NULL) || (capability >= PICOPEN_CAPABILITY_COUNT)) {
        return false;
    }
    const uint64_t grant = UINT64_C(1) << (unsigned int)capability;
    if ((context->grants & grant) == 0u) {
        return false;
    }
    if (requires_engagement(capability) && !context->engagement_active) {
        return false;
    }
    if (requires_local_confirmation(capability) && !local_confirmation) {
        return false;
    }
    return true;
}

const char *picopen_capability_name(picopen_capability_t capability) {
    static const char *const names[] = {
        "storage.read", "storage.write", "network.connect", "network.probe",
        "radio.receive", "radio.transmit", "gpio.read", "gpio.drive",
        "usb.hid", "target.power", "remote.control",
        "system.shutdown",
    };
    if ((unsigned int)capability >= sizeof(names) / sizeof(names[0])) {
        return "unknown";
    }
    return names[capability];
}
