#ifndef PICOPEN_APPS_H
#define PICOPEN_APPS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "picopen/storage.h"

#define PICOPEN_APP_ID_SIZE 24u
#define PICOPEN_APP_NAME_SIZE 24u
#define PICOPEN_APP_CAPACITY 16u
#define PICOPEN_APP_BUILTIN_COUNT 9u

typedef enum picopen_app_kind {
    PICOPEN_APP_DEVICE_INVENTORY = 0,
    PICOPEN_APP_NETWORK_DISCOVERY,
    PICOPEN_APP_HOST_INSPECTOR,
    PICOPEN_APP_HTTP_INSPECTOR,
    PICOPEN_APP_SSH_BANNER,
    PICOPEN_APP_TLS_INSPECTOR,
    PICOPEN_APP_EVIDENCE_ANALYZER,
    PICOPEN_APP_RECENT_RESULTS,
    PICOPEN_APP_SESSION_REPORTS,
    PICOPEN_APP_SD_PACKAGE,
} picopen_app_kind_t;

typedef struct picopen_app_descriptor {
    char id[PICOPEN_APP_ID_SIZE];
    char name[PICOPEN_APP_NAME_SIZE];
    picopen_app_kind_t kind;
    uint32_t requested_capabilities;
    bool built_in;
    bool compatible;
    bool signed_package;
} picopen_app_descriptor_t;

typedef struct picopen_app_catalog {
    picopen_app_descriptor_t apps[PICOPEN_APP_CAPACITY];
    size_t count;
    bool truncated;
    size_t rejected;
} picopen_app_catalog_t;

void picopen_apps_init(void);
void picopen_apps_scan_sd(picopen_storage_service_t *storage);
void picopen_apps_snapshot(picopen_app_catalog_t *catalog);
const char *picopen_app_kind_name(picopen_app_kind_t kind);

#endif
