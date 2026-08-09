#include "picopen/storage.h"

#include <string.h>

#include "ff.h"

static FATFS filesystem;
static picopen_storage_service_t compatibility_service;
static bool compatibility_initialized;

static bool valid_path(const char *path, bool allow_root) {
    if (path == NULL) {
        return false;
    }
    if (strcmp(path, "/") == 0) {
        return allow_root;
    }
    size_t length = 0u;
    size_t component_length = 0u;
    size_t depth = 0u;
    for (; path[length] != '\0'; ++length) {
        const char character = path[length];
        if ((length >= PICOPEN_STORAGE_PATH_SIZE - 1u) ||
            (character < '!') || (character > '~') ||
            (character == '\\') || (character == ':')) {
            return false;
        }
        if (character != '/') {
            ++component_length;
            continue;
        }
        if ((component_length == 0u) && (length == 0u)) {
            continue;
        }
        if ((component_length == 0u) || (++depth > PICOPEN_STORAGE_MAX_DEPTH)) {
            return false;
        }
        component_length = 0u;
    }
    if ((component_length == 0u) || (++depth > PICOPEN_STORAGE_MAX_DEPTH)) {
        return false;
    }
    const char *component = path;
    while (*component != '\0') {
        if (*component == '/') {
            ++component;
            continue;
        }
        const char *end = strchr(component, '/');
        const size_t component_size = end == NULL ? strlen(component)
                                                   : (size_t)(end - component);
        if (((component_size == 1u) && (component[0] == '.')) ||
            ((component_size == 2u) && (component[0] == '.') &&
             (component[1] == '.'))) {
            return false;
        }
        component = end == NULL ? component + component_size : end;
    }
    return length > 0u;
}

static picopen_storage_result_t map_result(FRESULT result) {
    switch (result) {
        case FR_OK: return PICOPEN_STORAGE_OK;
        case FR_NO_FILE:
        case FR_NO_PATH: return PICOPEN_STORAGE_NOT_FOUND;
        case FR_NO_FILESYSTEM: return PICOPEN_STORAGE_CORRUPT;
        case FR_INVALID_NAME:
        case FR_INVALID_PARAMETER: return PICOPEN_STORAGE_INVALID_REQUEST;
        default: return PICOPEN_STORAGE_IO_ERROR;
    }
}

static picopen_storage_result_t finish(picopen_storage_service_t *service,
                                       FRESULT result) {
    (void)f_unmount("");
    service->filesystem_result = result;
    service->last_result = map_result(result);
    if ((service->last_result == PICOPEN_STORAGE_CORRUPT) ||
        (service->last_result == PICOPEN_STORAGE_IO_ERROR)) {
        service->media_state = PICOPEN_STORAGE_MEDIA_ERROR;
    }
    return service->last_result;
}

void picopen_storage_init(picopen_storage_service_t *service) {
    if (service != NULL) {
        *service = (picopen_storage_service_t){
            .abi_version = PICOPEN_STORAGE_ABI_VERSION,
            .media_generation = 1u,
            .media_state = PICOPEN_STORAGE_MEDIA_ABSENT,
            .last_result = PICOPEN_STORAGE_NOT_READY,
        };
    }
}

void picopen_storage_media_ready(picopen_storage_service_t *service) {
    if (service == NULL) {
        return;
    }
    ++service->media_generation;
    service->media_state = PICOPEN_STORAGE_MEDIA_READY_READ_ONLY;
    service->last_result = PICOPEN_STORAGE_OK;
}

void picopen_storage_media_changed(picopen_storage_service_t *service) {
    if (service == NULL) {
        return;
    }
    (void)f_unmount("");
    ++service->media_generation;
    service->media_state = PICOPEN_STORAGE_MEDIA_CHANGED;
    service->last_result = PICOPEN_STORAGE_NOT_READY;
}

void picopen_storage_safe_remove(picopen_storage_service_t *service) {
    if (service == NULL) {
        return;
    }
    (void)f_unmount("");
    ++service->media_generation;
    service->media_state = PICOPEN_STORAGE_MEDIA_REMOVED_SAFE;
    service->last_result = PICOPEN_STORAGE_NOT_READY;
}

picopen_storage_result_t picopen_storage_list_directory(
    picopen_storage_service_t *service, const char *path,
    picopen_storage_listing_t *listing) {
    if ((service == NULL) || (service->abi_version != PICOPEN_STORAGE_ABI_VERSION) ||
        (listing == NULL) || !valid_path(path, true)) {
        return PICOPEN_STORAGE_INVALID_REQUEST;
    }
    *listing = (picopen_storage_listing_t){0};
    listing->media_generation = service->media_generation;
    strncpy(listing->path, path, sizeof(listing->path) - 1u);
    if (service->media_state != PICOPEN_STORAGE_MEDIA_READY_READ_ONLY) {
        service->last_result = PICOPEN_STORAGE_NOT_READY;
        return service->last_result;
    }
    FRESULT result = f_mount(&filesystem, "", 1u);
    if (result != FR_OK) {
        listing->result = result;
        return finish(service, result);
    }

    FATFS_DIR directory;
    result = f_opendir(&directory, path);
    if (result != FR_OK) {
        listing->result = result;
        return finish(service, result);
    }

    FILINFO info;
    for (;;) {
        result = f_readdir(&directory, &info);
        if ((result != FR_OK) || (info.fname[0] == '\0')) {
            break;
        }
        if (listing->count == PICOPEN_STORAGE_MAX_ENTRIES) {
            listing->truncated = true;
            break;
        }
        picopen_storage_entry_t *const entry =
            &listing->entries[listing->count++];
        strncpy(entry->name, info.fname, sizeof(entry->name) - 1u);
        entry->name[sizeof(entry->name) - 1u] = '\0';
        entry->size = (uint32_t)info.fsize;
        entry->directory = (info.fattrib & AM_DIR) != 0u;
        entry->read_only = (info.fattrib & AM_RDO) != 0u;
        entry->hidden = (info.fattrib & AM_HID) != 0u;
    }
    listing->result = result;
    (void)f_closedir(&directory);
    if ((result == FR_OK) && listing->truncated) {
        (void)f_unmount("");
        service->last_result = PICOPEN_STORAGE_LIMIT_REACHED;
        return service->last_result;
    }
    return finish(service, result);
}

picopen_storage_result_t picopen_storage_read_file(
    picopen_storage_service_t *service, const char *path, uint32_t offset,
    uint8_t *buffer, size_t capacity, size_t *bytes_read, bool *truncated) {
    if ((service == NULL) || (service->abi_version != PICOPEN_STORAGE_ABI_VERSION) ||
        !valid_path(path, false) || (buffer == NULL) || (bytes_read == NULL) ||
        (truncated == NULL) || (capacity == 0u) ||
        (capacity > PICOPEN_STORAGE_READ_LIMIT)) {
        return PICOPEN_STORAGE_INVALID_REQUEST;
    }
    *bytes_read = 0u;
    *truncated = false;
    if (service->media_state != PICOPEN_STORAGE_MEDIA_READY_READ_ONLY) {
        service->last_result = PICOPEN_STORAGE_NOT_READY;
        return service->last_result;
    }
    FRESULT result = f_mount(&filesystem, "", 1u);
    if (result != FR_OK) {
        return finish(service, result);
    }
    FIL file;
    result = f_open(&file, path, FA_READ);
    if (result != FR_OK) {
        return finish(service, result);
    }
    if ((FSIZE_t)offset > f_size(&file)) {
        (void)f_close(&file);
        return finish(service, FR_INVALID_PARAMETER);
    }
    result = f_lseek(&file, offset);
    if (result != FR_OK) {
        (void)f_close(&file);
        return finish(service, result);
    }
    UINT read_count = 0u;
    result = f_read(&file, buffer, (UINT)capacity, &read_count);
    *bytes_read = read_count;
    *truncated = ((FSIZE_t)offset + read_count) < f_size(&file);
    (void)f_close(&file);
    return finish(service, result);
}

const char *picopen_storage_result_name(picopen_storage_result_t result) {
    static const char *const names[] = {
        "OK", "INVALID", "NOT-READY", "NOT-FOUND", "CORRUPT", "IO-ERROR",
        "LIMIT",
    };
    return (unsigned int)result < sizeof(names) / sizeof(names[0])
               ? names[result]
               : "UNKNOWN";
}

static picopen_storage_service_t *compatibility(void) {
    if (!compatibility_initialized) {
        picopen_storage_init(&compatibility_service);
        picopen_storage_media_ready(&compatibility_service);
        compatibility_initialized = true;
    }
    return &compatibility_service;
}

bool picopen_storage_list_root(picopen_storage_listing_t *listing) {
    const picopen_storage_result_t result =
        picopen_storage_list_directory(compatibility(), "/", listing);
    return (result == PICOPEN_STORAGE_OK) ||
           (result == PICOPEN_STORAGE_LIMIT_REACHED);
}

bool picopen_storage_read_root_file(const char *name, uint8_t *buffer,
                                    size_t capacity, size_t *bytes_read,
                                    bool *truncated) {
    if ((name == NULL) || (strchr(name, '/') != NULL) ||
        (strchr(name, '\\') != NULL) || (strchr(name, ':') != NULL)) {
        return false;
    }
    return picopen_storage_read_file(compatibility(), name, 0u, buffer,
                                     capacity, bytes_read, truncated) ==
           PICOPEN_STORAGE_OK;
}
