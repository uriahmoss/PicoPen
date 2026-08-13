#ifndef PICOPEN_EVIDENCE_H
#define PICOPEN_EVIDENCE_H
#include <stdbool.h>
#include <stdint.h>
#include "picopen/storage.h"
typedef enum picopen_evidence_state { PICOPEN_EVIDENCE_IDLE=0,PICOPEN_EVIDENCE_HASHING,
    PICOPEN_EVIDENCE_PARSING,PICOPEN_EVIDENCE_COMPLETE,PICOPEN_EVIDENCE_CANCELLED,
    PICOPEN_EVIDENCE_DENIED,PICOPEN_EVIDENCE_ERROR } picopen_evidence_state_t;
typedef enum picopen_capture_format { PICOPEN_CAPTURE_NONE=0,PICOPEN_CAPTURE_PCAP,
    PICOPEN_CAPTURE_PCAPNG } picopen_capture_format_t;
typedef struct picopen_evidence_snapshot { picopen_evidence_state_t state;
    picopen_capture_format_t capture; char path[128]; char sha256[65];
    uint32_t size,processed,packet_count,string_count; int result;
} picopen_evidence_snapshot_t;
void picopen_evidence_init(void);
bool picopen_evidence_start(picopen_storage_service_t *storage,const char *path,
                            uint32_t size,bool locally_confirmed);
bool picopen_evidence_cancel(void);
bool picopen_evidence_poll(void);
void picopen_evidence_snapshot(picopen_evidence_snapshot_t *snapshot);
const char *picopen_evidence_state_name(picopen_evidence_state_t state);
#endif
