#ifndef PICOPEN_STORAGE_H
#define PICOPEN_STORAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PICOPEN_STORAGE_ABI_VERSION 1u
#define PICOPEN_STORAGE_MAX_ENTRIES 12u
#define PICOPEN_STORAGE_NAME_SIZE   64u
#define PICOPEN_STORAGE_PATH_SIZE   128u
#define PICOPEN_STORAGE_MAX_DEPTH   4u
#define PICOPEN_STORAGE_READ_LIMIT  256u

typedef enum picopen_storage_media_state {
    PICOPEN_STORAGE_MEDIA_ABSENT = 0,
    PICOPEN_STORAGE_MEDIA_READY_READ_ONLY,
    PICOPEN_STORAGE_MEDIA_REMOVED_SAFE,
    PICOPEN_STORAGE_MEDIA_CHANGED,
    PICOPEN_STORAGE_MEDIA_ERROR,
} picopen_storage_media_state_t;

typedef enum picopen_storage_result {
    PICOPEN_STORAGE_OK = 0,
    PICOPEN_STORAGE_INVALID_REQUEST,
    PICOPEN_STORAGE_NOT_READY,
    PICOPEN_STORAGE_NOT_FOUND,
    PICOPEN_STORAGE_CORRUPT,
    PICOPEN_STORAGE_IO_ERROR,
    PICOPEN_STORAGE_LIMIT_REACHED,
} picopen_storage_result_t;

typedef struct picopen_storage_service {
    uint16_t abi_version;
    uint32_t media_generation;
    picopen_storage_media_state_t media_state;
    picopen_storage_result_t last_result;
    int filesystem_result;
} picopen_storage_service_t;

typedef struct picopen_storage_entry {
    char name[PICOPEN_STORAGE_NAME_SIZE];
    uint32_t size;
    bool directory;
    bool read_only;
    bool hidden;
} picopen_storage_entry_t;

typedef struct picopen_storage_listing {
    picopen_storage_entry_t entries[PICOPEN_STORAGE_MAX_ENTRIES];
    size_t count;
    bool truncated;
    int result;
    uint32_t media_generation;
    char path[PICOPEN_STORAGE_PATH_SIZE];
} picopen_storage_listing_t;

void picopen_storage_init(picopen_storage_service_t *service);
void picopen_storage_media_ready(picopen_storage_service_t *service);
void picopen_storage_media_changed(picopen_storage_service_t *service);
void picopen_storage_safe_remove(picopen_storage_service_t *service);
picopen_storage_result_t picopen_storage_list_directory(
    picopen_storage_service_t *service, const char *path,
    picopen_storage_listing_t *listing);
picopen_storage_result_t picopen_storage_read_file(
    picopen_storage_service_t *service, const char *path, uint32_t offset,
    uint8_t *buffer, size_t capacity, size_t *bytes_read, bool *truncated);
const char *picopen_storage_result_name(picopen_storage_result_t result);

// Compatibility helpers for the current shell and GUI. Both remain bounded,
// read-only, root-scoped, and never retain a mounted removable volume.
bool picopen_storage_list_root(picopen_storage_listing_t *listing);
bool picopen_storage_read_root_file(const char *name, uint8_t *buffer,
                                    size_t capacity, size_t *bytes_read,
                                    bool *truncated);

#endif
