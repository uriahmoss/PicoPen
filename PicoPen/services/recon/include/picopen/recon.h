#ifndef PICOPEN_RECON_H
#define PICOPEN_RECON_H
#include <stdbool.h>
#include <stdint.h>
typedef enum picopen_recon_kind { PICOPEN_RECON_DNS=0, PICOPEN_RECON_ICMP,
    PICOPEN_RECON_TCP } picopen_recon_kind_t;
typedef enum picopen_recon_state { PICOPEN_RECON_IDLE=0, PICOPEN_RECON_RUNNING,
    PICOPEN_RECON_COMPLETE, PICOPEN_RECON_DENIED, PICOPEN_RECON_TIMEOUT,
    PICOPEN_RECON_CANCELLED, PICOPEN_RECON_ERROR } picopen_recon_state_t;
typedef struct picopen_recon_snapshot {
    picopen_recon_kind_t kind; picopen_recon_state_t state;
    char target[64]; char address[16]; uint16_t port;
    uint32_t elapsed_ms; int result;
} picopen_recon_snapshot_t;
void picopen_recon_init(void);
bool picopen_recon_start(picopen_recon_kind_t kind,const char *target,uint16_t port,
                         uint64_t now_ms,bool locally_confirmed);
bool picopen_recon_cancel(void);
bool picopen_recon_poll(uint64_t now_ms);
void picopen_recon_snapshot(picopen_recon_snapshot_t *snapshot);
const char *picopen_recon_state_name(picopen_recon_state_t state);
#endif
