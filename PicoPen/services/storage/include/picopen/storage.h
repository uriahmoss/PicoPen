#ifndef PICOPEN_STORAGE_H
#define PICOPEN_STORAGE_H

#include <stdbool.h>
#include <stddef.h>

#define PICOPEN_STORAGE_MAX_ENTRIES 8u
#define PICOPEN_STORAGE_NAME_SIZE   13u

typedef struct picopen_storage_entry {
    char name[PICOPEN_STORAGE_NAME_SIZE];
    bool directory;
} picopen_storage_entry_t;

typedef struct picopen_storage_listing {
    picopen_storage_entry_t entries[PICOPEN_STORAGE_MAX_ENTRIES];
    size_t count;
    bool truncated;
    int result;
} picopen_storage_listing_t;

// Mounts the already initialized SD transport read-only, lists a bounded
// number of root entries, and unmounts before returning.
bool picopen_storage_list_root(picopen_storage_listing_t *listing);

#endif
