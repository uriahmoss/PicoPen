#ifndef PICOPEN_INTERNAL_FS_H
#define PICOPEN_INTERNAL_FS_H

#include <stdbool.h>
#include <stddef.h>

typedef enum picopen_internal_fs_state {
    PICOPEN_INTERNAL_FS_UNAVAILABLE = 0,
    PICOPEN_INTERNAL_FS_UNINITIALIZED,
    PICOPEN_INTERNAL_FS_READY,
    PICOPEN_INTERNAL_FS_CORRUPT,
} picopen_internal_fs_state_t;

void picopen_internal_fs_init(void);
picopen_internal_fs_state_t picopen_internal_fs_state(void);
size_t picopen_internal_fs_used_blocks(void);
size_t picopen_internal_fs_total_blocks(void);
bool picopen_internal_fs_format(bool locally_confirmed);
bool picopen_internal_fs_read(const char *path, void *data, size_t capacity,
                              size_t *length);
bool picopen_internal_fs_replace(const char *path, const void *data,
                                 size_t length);
bool picopen_internal_fs_remove(const char *path);

#endif
