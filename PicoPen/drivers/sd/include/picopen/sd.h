#ifndef PICOPEN_SD_H
#define PICOPEN_SD_H

#include <stdbool.h>
#include <stdint.h>

typedef enum picopen_sd_status {
    PICOPEN_SD_READY = 0,
    PICOPEN_SD_NO_RESPONSE,
    PICOPEN_SD_BAD_VOLTAGE,
    PICOPEN_SD_INIT_TIMEOUT,
    PICOPEN_SD_OCR_ERROR,
    PICOPEN_SD_READ_ERROR,
} picopen_sd_status_t;

typedef enum picopen_sd_filesystem {
    PICOPEN_SD_FILESYSTEM_UNKNOWN = 0,
    PICOPEN_SD_FILESYSTEM_FAT,
    PICOPEN_SD_FILESYSTEM_EXFAT,
} picopen_sd_filesystem_t;

typedef struct picopen_sd_info {
    picopen_sd_status_t status;
    bool version_2;
    bool high_capacity;
    bool software_spi;
    bool card_detected;
    bool sector_read;
    bool partitioned;
    uint8_t last_response;
    uint32_t ocr;
    uint32_t first_partition_lba;
    picopen_sd_filesystem_t filesystem;
} picopen_sd_info_t;

// Initializes and identifies the built-in SD card in SPI mode. This interface
// intentionally exposes no block-write, erase, or formatting operation.
bool picopen_sd_identify(picopen_sd_info_t *info);
bool picopen_sd_read_blocks(uint32_t first_lba, uint8_t *buffer,
                            uint32_t block_count);
void picopen_sd_close(void);
const char *picopen_sd_status_name(picopen_sd_status_t status);
const char *picopen_sd_filesystem_name(picopen_sd_filesystem_t filesystem);

#endif
