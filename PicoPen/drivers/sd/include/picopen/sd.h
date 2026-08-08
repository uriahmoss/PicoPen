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
} picopen_sd_status_t;

typedef struct picopen_sd_info {
    picopen_sd_status_t status;
    bool version_2;
    bool high_capacity;
    uint8_t last_response;
    uint32_t ocr;
} picopen_sd_info_t;

// Initializes and identifies the built-in SD card in SPI mode. This interface
// intentionally exposes no block-write, erase, or formatting operation.
bool picopen_sd_identify(picopen_sd_info_t *info);
const char *picopen_sd_status_name(picopen_sd_status_t status);

#endif
