#include "picopen/ipc.h"

#include <stddef.h>

void picopen_ipc_init(picopen_ipc_bus_t *bus) {
    if (bus != NULL) {
        *bus = (picopen_ipc_bus_t){0};
    }
}

bool picopen_ipc_register(picopen_ipc_bus_t *bus, uint16_t identifier,
                          uint16_t required_capability,
                          picopen_ipc_handler_t handler, void *owner) {
    if ((bus == NULL) || (identifier == 0u) || (handler == NULL)) {
        return false;
    }
    if ((required_capability != PICOPEN_IPC_NO_CAPABILITY) &&
        (required_capability >= PICOPEN_CAPABILITY_COUNT)) {
        return false;
    }
    for (size_t index = 0u; index < PICOPEN_IPC_MAX_SERVICES; ++index) {
        if (bus->services[index].registered &&
            (bus->services[index].identifier == identifier)) {
            return false;
        }
    }
    for (size_t index = 0u; index < PICOPEN_IPC_MAX_SERVICES; ++index) {
        if (bus->services[index].registered) {
            continue;
        }
        bus->services[index] = (picopen_ipc_service_t){
            .identifier = identifier,
            .required_capability = required_capability,
            .handler = handler,
            .owner = owner,
            .registered = true,
        };
        return true;
    }
    return false;
}

picopen_ipc_result_t picopen_ipc_dispatch(
    const picopen_ipc_bus_t *bus, const picopen_security_context_t *security,
    bool local_confirmation, const picopen_ipc_message_t *request,
    picopen_ipc_message_t *response) {
    if ((bus == NULL) || (request == NULL) || (response == NULL) ||
        (request->version != PICOPEN_IPC_VERSION) ||
        (request->request_id == 0u) ||
        (request->payload_size > PICOPEN_IPC_PAYLOAD_SIZE)) {
        return PICOPEN_IPC_INVALID;
    }
    for (size_t index = 0u; index < PICOPEN_IPC_MAX_SERVICES; ++index) {
        const picopen_ipc_service_t *const service = &bus->services[index];
        if (!service->registered ||
            (service->identifier != request->service)) {
            continue;
        }
        if ((service->required_capability != PICOPEN_IPC_NO_CAPABILITY) &&
            !picopen_security_authorize(
                security, (picopen_capability_t)service->required_capability,
                local_confirmation)) {
            return PICOPEN_IPC_DENIED;
        }
        *response = (picopen_ipc_message_t){
            .version = PICOPEN_IPC_VERSION,
            .service = request->service,
            .operation = request->operation,
            .request_id = request->request_id,
        };
        return service->handler(request, response, service->owner)
                   ? PICOPEN_IPC_OK
                   : PICOPEN_IPC_SERVICE_ERROR;
    }
    return PICOPEN_IPC_NO_SERVICE;
}
