#include "picopen/apps.h"

#include <stdio.h>
#include <string.h>

static picopen_app_catalog_t catalog;

static const picopen_app_descriptor_t builtins[PICOPEN_APP_BUILTIN_COUNT] = {
    {.id="device.inventory",.name="Device Inventory",.kind=PICOPEN_APP_DEVICE_INVENTORY,.required_provider=-1,.built_in=true,.compatible=true},
    {.id="network.discovery",.name="Network Discovery",.kind=PICOPEN_APP_NETWORK_DISCOVERY,.required_provider=-1,.built_in=true,.compatible=true},
    {.id="host.inspector",.name="Host Inspector",.kind=PICOPEN_APP_HOST_INSPECTOR,.required_provider=-1,.built_in=true,.compatible=true},
    {.id="http.inspector",.name="HTTP Inspector",.kind=PICOPEN_APP_HTTP_INSPECTOR,.required_provider=-1,.built_in=true,.compatible=true},
    {.id="ssh.banner",.name="SSH Banner",.kind=PICOPEN_APP_SSH_BANNER,.required_provider=-1,.built_in=true,.compatible=true},
    {.id="tls.inspector",.name="TLS Inspector",.kind=PICOPEN_APP_TLS_INSPECTOR,.required_provider=-1,.built_in=true,.compatible=true},
    {.id="evidence.analyzer",.name="Evidence Analyzer",.kind=PICOPEN_APP_EVIDENCE_ANALYZER,.required_provider=-1,.built_in=true,.compatible=true},
    {.id="recent.results",.name="Recent Results",.kind=PICOPEN_APP_RECENT_RESULTS,.required_provider=-1,.built_in=true,.compatible=true},
    {.id="session.reports",.name="Session Reports",.kind=PICOPEN_APP_SESSION_REPORTS,.required_provider=-1,.built_in=true,.compatible=true},
};

void picopen_apps_init(void) {
    catalog = (picopen_app_catalog_t){0};
    memcpy(catalog.apps, builtins, sizeof(builtins));
    catalog.count = PICOPEN_APP_BUILTIN_COUNT;
}

static bool manifest_name(const char *name) {
    const size_t length = name ? strlen(name) : 0u;
    return length > 8u && strcmp(&name[length - 8u], ".ppapp") == 0;
}

void picopen_apps_scan_sd(picopen_storage_service_t *storage) {
    picopen_apps_init();
    if (!storage) return;
    picopen_storage_listing_t listing;
    const picopen_storage_result_t result = picopen_storage_list_directory(
        storage, "/PicoPen/apps", &listing);
    if (result != PICOPEN_STORAGE_OK && result != PICOPEN_STORAGE_LIMIT_REACHED)
        return;
    for (size_t index = 0u; index < listing.count; ++index) {
        const picopen_storage_entry_t *entry = &listing.entries[index];
        if (entry->directory || !manifest_name(entry->name)) continue;
        if (catalog.count >= PICOPEN_APP_CAPACITY) {
            catalog.truncated = true;
            break;
        }
        picopen_app_descriptor_t *app = &catalog.apps[catalog.count++];
        snprintf(app->id, sizeof(app->id), "sd.%u", (unsigned)index);
        snprintf(app->name, sizeof(app->name), "%.23s", entry->name);
        app->kind = PICOPEN_APP_SD_PACKAGE;
        app->required_provider = -1;
        app->compatible = false;
        app->signed_package = false;
    }
    catalog.truncated |= listing.truncated;
}

void picopen_apps_snapshot(picopen_app_catalog_t *output) {
    if (output) *output = catalog;
}

const char *picopen_app_kind_name(picopen_app_kind_t kind) {
    static const char *const names[] = {"DEVICE","DISCOVERY","HOST","HTTP","SSH","TLS","EVIDENCE","RECENT","REPORTS","SD APP"};
    return (unsigned)kind < sizeof(names)/sizeof(names[0]) ? names[kind] : "UNKNOWN";
}

bool picopen_app_available(const picopen_app_descriptor_t *app) {
    if (!app || !app->compatible) return false;
    if (app->required_provider < 0) return true;
    return picopen_attachment_has_provider(
        (picopen_provider_capability_t)app->required_provider);
}
