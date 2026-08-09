#include "picopen/storage.h"

#include <string.h>

#include "ff.h"

static FATFS filesystem;

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
