#ifndef PICOPEN_IMAGE_VALIDATOR_H
#define PICOPEN_IMAGE_VALIDATOR_H

#include <stdint.h>

typedef enum picopen_image_status {
    PICOPEN_IMAGE_OK = 0,
    PICOPEN_IMAGE_BAD_SLOT,
    PICOPEN_IMAGE_BAD_MAGIC,
    PICOPEN_IMAGE_BAD_FORMAT,
    PICOPEN_IMAGE_BAD_HEADER_CRC,
    PICOPEN_IMAGE_BAD_TARGET,
    PICOPEN_IMAGE_BAD_FLAGS,
    PICOPEN_IMAGE_BAD_BOUNDS,
    PICOPEN_IMAGE_BAD_ALGORITHM,
    PICOPEN_IMAGE_BAD_POLICY,
    PICOPEN_IMAGE_DIGEST_UNAVAILABLE,
    PICOPEN_IMAGE_BAD_DIGEST,
} picopen_image_status_t;

picopen_image_status_t picopen_validate_primary_image(void);
const char *picopen_image_status_string(picopen_image_status_t status);

#endif
