#include "picopen/storage.h"

#include <string.h>

#include "ff.h"

static FATFS filesystem;

static bool valid_root_name(const char *name) {
    if ((name == NULL) || (name[0] == '\0')) {
        return false;
    }
    size_t length = 0u;
    for (; name[length] != '\0'; ++length) {
        const char character = name[length];
        if ((length >= PICOPEN_STORAGE_NAME_SIZE - 1u) ||
            (character < '!') || (character > '~') ||
            (character == '/') || (character == '\\') || (character == ':')) {
            return false;
        }
    }
    return (length > 0u) && (strcmp(name, ".") != 0) &&
           (strcmp(name, "..") != 0);
}

bool picopen_storage_list_root(picopen_storage_listing_t *listing) {
    if (listing == NULL) {
        return false;
    }
    *listing = (picopen_storage_listing_t){0};
    FRESULT result = f_mount(&filesystem, "", 1u);
    if (result != FR_OK) {
        listing->result = result;
        return false;
    }

    FATFS_DIR directory;
    result = f_opendir(&directory, "/");
    if (result != FR_OK) {
        listing->result = result;
        (void)f_unmount("");
        return false;
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
        entry->directory = (info.fattrib & AM_DIR) != 0u;
    }
    listing->result = result;
    (void)f_closedir(&directory);
    (void)f_unmount("");
    return result == FR_OK;
}

bool picopen_storage_read_root_file(const char *name, uint8_t *buffer,
                                    size_t capacity, size_t *bytes_read,
                                    bool *truncated) {
    if (!valid_root_name(name) || (buffer == NULL) || (bytes_read == NULL) ||
        (truncated == NULL) || (capacity == 0u) ||
        (capacity > PICOPEN_STORAGE_READ_LIMIT)) {
        return false;
    }
    *bytes_read = 0u;
    *truncated = false;
    FRESULT result = f_mount(&filesystem, "", 1u);
    if (result != FR_OK) {
        return false;
    }
    FIL file;
    result = f_open(&file, name, FA_READ);
    if (result != FR_OK) {
        (void)f_unmount("");
        return false;
    }
    UINT read_count = 0u;
    result = f_read(&file, buffer, (UINT)capacity, &read_count);
    *bytes_read = read_count;
    *truncated = (FSIZE_t)read_count < f_size(&file);
    (void)f_close(&file);
    (void)f_unmount("");
    return result == FR_OK;
}
