#ifndef PICOPEN_RECON_H
#define PICOPEN_RECON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PICOPEN_RECON_TARGET_SIZE 64u
#define PICOPEN_RECON_ADDRESS_SIZE 16u
#define PICOPEN_RECON_DETAIL_SIZE 96u
#define PICOPEN_RECON_HISTORY_CAPACITY 6u

typedef enum picopen_recon_kind {
    PICOPEN_RECON_DNS = 0,
    PICOPEN_RECON_ICMP,
    PICOPEN_RECON_TCP,
    PICOPEN_RECON_HTTP_HEAD,
    PICOPEN_RECON_SSH_BANNER,
    PICOPEN_RECON_TLS_METADATA,
} picopen_recon_kind_t;

typedef enum picopen_recon_state {
    PICOPEN_RECON_IDLE = 0,
    PICOPEN_RECON_RUNNING,
    PICOPEN_RECON_COMPLETE,
    PICOPEN_RECON_DENIED,
    PICOPEN_RECON_TIMEOUT,
    PICOPEN_RECON_CANCELLED,
    PICOPEN_RECON_REFUSED,
    PICOPEN_RECON_UNAVAILABLE,
    PICOPEN_RECON_ERROR,
} picopen_recon_state_t;

typedef struct picopen_recon_snapshot {
    picopen_recon_kind_t kind;
    picopen_recon_state_t state;
    char target[PICOPEN_RECON_TARGET_SIZE];
    char address[PICOPEN_RECON_ADDRESS_SIZE];
    char service[16];
    char detail[PICOPEN_RECON_DETAIL_SIZE];
    uint16_t port;
    uint32_t elapsed_ms;
    uint32_t bytes_received;
    int result;
} picopen_recon_snapshot_t;

void picopen_recon_init(void);
bool picopen_recon_start(picopen_recon_kind_t kind, const char *target,
                         uint16_t port, uint64_t now_ms,
                         bool locally_confirmed, bool boundary_required);
bool picopen_recon_cancel(void);
bool picopen_recon_poll(uint64_t now_ms);
void picopen_recon_snapshot(picopen_recon_snapshot_t *snapshot);
size_t picopen_recon_history_count(void);
bool picopen_recon_history_get(size_t newest_index,
                               picopen_recon_snapshot_t *snapshot);
const char *picopen_recon_kind_name(picopen_recon_kind_t kind);
const char *picopen_recon_state_name(picopen_recon_state_t state);

#endif
