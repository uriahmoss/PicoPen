#ifndef PICOPEN_IPC_H
#define PICOPEN_IPC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "picopen/capability.h"

#define PICOPEN_IPC_VERSION          1u
#define PICOPEN_IPC_PAYLOAD_SIZE    32u
#define PICOPEN_IPC_MAX_SERVICES     8u
#define PICOPEN_IPC_NO_CAPABILITY UINT16_MAX

typedef enum picopen_ipc_result {
    PICOPEN_IPC_OK = 0,
    PICOPEN_IPC_INVALID,
    PICOPEN_IPC_NO_SERVICE,
    PICOPEN_IPC_DENIED,
    PICOPEN_IPC_SERVICE_ERROR,
} picopen_ipc_result_t;

typedef struct picopen_ipc_message {
    uint16_t version;
    uint16_t service;
    uint16_t operation;
    uint32_t request_id;
    uint16_t payload_size;
    uint8_t payload[PICOPEN_IPC_PAYLOAD_SIZE];
} picopen_ipc_message_t;

typedef bool (*picopen_ipc_handler_t)(const picopen_ipc_message_t *request,
                                     picopen_ipc_message_t *response,
                                     void *owner);

typedef struct picopen_ipc_service {
    uint16_t identifier;
    uint16_t required_capability;
    picopen_ipc_handler_t handler;
    void *owner;
    bool registered;
} picopen_ipc_service_t;

typedef struct picopen_ipc_bus {
    picopen_ipc_service_t services[PICOPEN_IPC_MAX_SERVICES];
} picopen_ipc_bus_t;

void picopen_ipc_init(picopen_ipc_bus_t *bus);
bool picopen_ipc_register(picopen_ipc_bus_t *bus, uint16_t identifier,
                          uint16_t required_capability,
                          picopen_ipc_handler_t handler, void *owner);
picopen_ipc_result_t picopen_ipc_dispatch(
    const picopen_ipc_bus_t *bus, const picopen_security_context_t *security,
    bool local_confirmation, const picopen_ipc_message_t *request,
    picopen_ipc_message_t *response);

#endif
