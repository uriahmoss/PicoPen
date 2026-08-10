#include "picopen/internal_fs.h"

#include <string.h>

#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"
#include "lfs.h"
#include "picopen/boot_format.h"

#define FS_BLOCK_SIZE 4096u
#define FS_PROG_SIZE 256u
#define FS_BLOCK_COUNT (PICOPEN_PERSISTENT_SIZE / FS_BLOCK_SIZE)

static lfs_t fs;
static uint8_t read_buffer[FS_PROG_SIZE];
static uint8_t program_buffer[FS_PROG_SIZE];
static uint8_t lookahead_buffer[16u];
static picopen_internal_fs_state_t state;

static int block_read(const struct lfs_config *config, lfs_block_t block,
                      lfs_off_t offset, void *buffer, lfs_size_t size) {
    (void)config;
    if ((block >= FS_BLOCK_COUNT) || (offset + size > FS_BLOCK_SIZE)) return LFS_ERR_INVAL;
    const uintptr_t address = XIP_BASE + PICOPEN_PERSISTENT_OFFSET +
                              block * FS_BLOCK_SIZE + offset;
    memcpy(buffer, (const void *)address, size);
    return LFS_ERR_OK;
}

static int block_program(const struct lfs_config *config, lfs_block_t block,
                         lfs_off_t offset, const void *buffer, lfs_size_t size) {
    (void)config;
    if ((block >= FS_BLOCK_COUNT) || (offset + size > FS_BLOCK_SIZE) ||
        (offset % FS_PROG_SIZE) || (size % FS_PROG_SIZE)) return LFS_ERR_INVAL;
    const uint32_t interrupts = save_and_disable_interrupts();
    flash_range_program(PICOPEN_PERSISTENT_OFFSET + block * FS_BLOCK_SIZE + offset,
                        buffer, size);
    restore_interrupts(interrupts);
    return LFS_ERR_OK;
}

static int block_erase(const struct lfs_config *config, lfs_block_t block) {
    (void)config;
    if (block >= FS_BLOCK_COUNT) return LFS_ERR_INVAL;
    const uint32_t interrupts = save_and_disable_interrupts();
    flash_range_erase(PICOPEN_PERSISTENT_OFFSET + block * FS_BLOCK_SIZE, FS_BLOCK_SIZE);
    restore_interrupts(interrupts);
    return LFS_ERR_OK;
}

static int block_sync(const struct lfs_config *config) { (void)config; return LFS_ERR_OK; }

static const struct lfs_config config = {
    .read = block_read, .prog = block_program, .erase = block_erase,
    .sync = block_sync, .read_size = 1u, .prog_size = FS_PROG_SIZE,
    .block_size = FS_BLOCK_SIZE, .block_count = FS_BLOCK_COUNT,
    .block_cycles = 500, .cache_size = FS_PROG_SIZE,
    .lookahead_size = sizeof(lookahead_buffer),
    .read_buffer = read_buffer, .prog_buffer = program_buffer,
    .lookahead_buffer = lookahead_buffer,
};

void picopen_internal_fs_init(void) {
    const int result = lfs_mount(&fs, &config);
    state = result == LFS_ERR_OK ? PICOPEN_INTERNAL_FS_READY
          : result == LFS_ERR_CORRUPT ? PICOPEN_INTERNAL_FS_UNINITIALIZED
          : PICOPEN_INTERNAL_FS_CORRUPT;
}

picopen_internal_fs_state_t picopen_internal_fs_state(void) { return state; }

bool picopen_internal_fs_format(bool locally_confirmed) {
    if (!locally_confirmed || state == PICOPEN_INTERNAL_FS_READY) return false;
    if (lfs_format(&fs, &config) != LFS_ERR_OK || lfs_mount(&fs, &config) != LFS_ERR_OK) {
        state = PICOPEN_INTERNAL_FS_CORRUPT;
        return false;
    }
    state = PICOPEN_INTERNAL_FS_READY;
    return true;
}

bool picopen_internal_fs_read(const char *path, void *data, size_t capacity,
                              size_t *length) {
    if (state != PICOPEN_INTERNAL_FS_READY || !path || !data || !length) return false;
    lfs_file_t file;
    if (lfs_file_open(&fs, &file, path, LFS_O_RDONLY) != LFS_ERR_OK) return false;
    const lfs_soff_t size = lfs_file_size(&fs, &file);
    bool ok = size >= 0 && (size_t)size <= capacity &&
              lfs_file_read(&fs, &file, data, (lfs_size_t)size) == size;
    (void)lfs_file_close(&fs, &file);
    if (ok) *length = (size_t)size;
    return ok;
}

bool picopen_internal_fs_replace(const char *path, const void *data, size_t length) {
    if (state != PICOPEN_INTERNAL_FS_READY || !path || !data || length > 1024u) return false;
    const char *temporary = "/wifi.tmp";
    lfs_file_t file;
    if (lfs_file_open(&fs, &file, temporary, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) != LFS_ERR_OK) return false;
    bool ok = lfs_file_write(&fs, &file, data, length) == (lfs_ssize_t)length &&
              lfs_file_sync(&fs, &file) == LFS_ERR_OK;
    ok = lfs_file_close(&fs, &file) == LFS_ERR_OK && ok;
    if (!ok) { (void)lfs_remove(&fs, temporary); return false; }
    (void)lfs_remove(&fs, path);
    return lfs_rename(&fs, temporary, path) == LFS_ERR_OK;
}

bool picopen_internal_fs_remove(const char *path) {
    if (state != PICOPEN_INTERNAL_FS_READY || !path) return false;
    const int result = lfs_remove(&fs, path);
    return result == LFS_ERR_OK || result == LFS_ERR_NOENT;
}
