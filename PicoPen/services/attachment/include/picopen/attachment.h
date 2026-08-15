#ifndef PICOPEN_ATTACHMENT_H
#define PICOPEN_ATTACHMENT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PICOPEN_ATTACHMENT_ABI_VERSION 1u
#define PICOPEN_ATTACHMENT_CAPACITY 8u
#define PICOPEN_ATTACHMENT_ID_SIZE 24u
#define PICOPEN_ATTACHMENT_NAME_SIZE 24u

typedef enum picopen_provider_capability {
    PICOPEN_PROVIDER_NFC_READ = 0,
    PICOPEN_PROVIDER_NFC_EMULATE,
    PICOPEN_PROVIDER_SUBGHZ_RECEIVE,
    PICOPEN_PROVIDER_SUBGHZ_TRANSMIT,
    PICOPEN_PROVIDER_IR_RECEIVE,
    PICOPEN_PROVIDER_IR_TRANSMIT,
    PICOPEN_PROVIDER_UART_MONITOR,
    PICOPEN_PROVIDER_I2C_CONTROLLER,
    PICOPEN_PROVIDER_SPI_CONTROLLER,
    PICOPEN_PROVIDER_GPIO_SAMPLE,
    PICOPEN_PROVIDER_LOGIC_CAPTURE,
    PICOPEN_PROVIDER_ONEWIRE,
    PICOPEN_PROVIDER_GPS,
    PICOPEN_PROVIDER_BLE_SCAN,
    PICOPEN_PROVIDER_CAPABILITY_COUNT,
} picopen_provider_capability_t;

typedef enum picopen_attachment_transport {
    PICOPEN_ATTACHMENT_TRANSPORT_NONE = 0,
    PICOPEN_ATTACHMENT_TRANSPORT_I2C,
    PICOPEN_ATTACHMENT_TRANSPORT_SPI,
    PICOPEN_ATTACHMENT_TRANSPORT_UART,
    PICOPEN_ATTACHMENT_TRANSPORT_USB,
    PICOPEN_ATTACHMENT_TRANSPORT_GPIO,
    PICOPEN_ATTACHMENT_TRANSPORT_PIO,
    PICOPEN_ATTACHMENT_TRANSPORT_MOCK,
} picopen_attachment_transport_t;

typedef enum picopen_attachment_state {
    PICOPEN_ATTACHMENT_ABSENT = 0,
    PICOPEN_ATTACHMENT_REGISTERED,
    PICOPEN_ATTACHMENT_DISABLED_POLICY,
    PICOPEN_ATTACHMENT_READY_RECEIVE_ONLY,
    PICOPEN_ATTACHMENT_READY,
    PICOPEN_ATTACHMENT_DEGRADED,
    PICOPEN_ATTACHMENT_ERROR,
} picopen_attachment_state_t;

typedef struct picopen_attachment_descriptor {
    uint16_t abi_version;
    char id[PICOPEN_ATTACHMENT_ID_SIZE];
    char name[PICOPEN_ATTACHMENT_NAME_SIZE];
    picopen_attachment_transport_t transport;
    uint64_t capabilities;
    uint32_t max_current_ma;
    bool external_power_required;
    bool mock;
} picopen_attachment_descriptor_t;

typedef struct picopen_attachment_record {
    picopen_attachment_descriptor_t descriptor;
    picopen_attachment_state_t state;
    uint32_t generation;
    int last_error;
} picopen_attachment_record_t;

typedef struct picopen_attachment_registry {
    picopen_attachment_record_t records[PICOPEN_ATTACHMENT_CAPACITY];
    size_t count;
    bool truncated;
} picopen_attachment_registry_t;

void picopen_attachment_init(void);
bool picopen_attachment_register(const picopen_attachment_descriptor_t *descriptor,
                                 picopen_attachment_state_t state);
bool picopen_attachment_set_state(const char *id,
                                  picopen_attachment_state_t state,
                                  int error);
bool picopen_attachment_has_provider(picopen_provider_capability_t capability);
void picopen_attachment_snapshot(picopen_attachment_registry_t *registry);
const char *picopen_attachment_state_name(picopen_attachment_state_t state);
const char *picopen_attachment_transport_name(
    picopen_attachment_transport_t transport);

#endif
