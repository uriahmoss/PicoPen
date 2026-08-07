#ifndef PICOPEN_IMAGE_VALIDATOR_H
#define PICOPEN_IMAGE_VALIDATOR_H

#include <stdint.h>

typedef struct picopen_validated_image {
    uint32_t region_base;
    uint32_t region_size;
    uint32_t entry_address;
    uint32_t vector_address;
} picopen_validated_image_t;

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

picopen_image_status_t picopen_validate_primary_image(
    picopen_validated_image_t *validated);
const char *picopen_image_status_string(picopen_image_status_t status);

#endif
