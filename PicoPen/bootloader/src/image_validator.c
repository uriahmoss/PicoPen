#include "picopen/image_validator.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "pico/sha256.h"
#include "hardware/regs/addressmap.h"

#include "picopen/boot_format.h"

static uint32_t crc32_ieee(const uint8_t *data, size_t length) {
    uint32_t crc = UINT32_C(0xFFFFFFFF);
    for (size_t index = 0u; index < length; ++index) {
        crc ^= data[index];
        for (unsigned bit = 0u; bit < 8u; ++bit) {
            const uint32_t mask = (uint32_t)-(int32_t)(crc & 1u);
            crc = (crc >> 1u) ^ (UINT32_C(0xEDB88320) & mask);
        }
    }
    return ~crc;
}

static bool bytes_are(const uint8_t *data, size_t length, uint8_t value) {
    for (size_t index = 0u; index < length; ++index) {
        if (data[index] != value) {
            return false;
        }
    }
    return true;
}

static picopen_image_status_t validate_header(
    const uint8_t *slot,
    picopen_image_header_v1_t *header) {
    static const uint8_t expected_magic[8] = PICOPEN_IMAGE_MAGIC_BYTES;

    memcpy(header, slot, sizeof(*header));
    if (memcmp(header->magic, expected_magic, sizeof(expected_magic)) != 0) {
        return PICOPEN_IMAGE_BAD_MAGIC;
    }
    if ((header->format_version != PICOPEN_IMAGE_FORMAT_VERSION) ||
        (header->header_size != PICOPEN_IMAGE_HEADER_SIZE) ||
        !bytes_are(header->reserved, sizeof(header->reserved), 0u) ||
        !bytes_are(slot + PICOPEN_IMAGE_HEADER_SIZE,
                   PICOPEN_IMAGE_MANIFEST_SIZE - PICOPEN_IMAGE_HEADER_SIZE,
                   UINT8_C(0xFF))) {
        return PICOPEN_IMAGE_BAD_FORMAT;
    }
    if (crc32_ieee(slot, offsetof(picopen_image_header_v1_t, header_crc32)) !=
        header->header_crc32) {
        return PICOPEN_IMAGE_BAD_HEADER_CRC;
    }
    if (header->target_id != PICOPEN_TARGET_PICO2_W) {
        return PICOPEN_IMAGE_BAD_TARGET;
    }
    if ((header->flags & ~PICOPEN_IMAGE_KNOWN_FLAGS) != 0u) {
        return PICOPEN_IMAGE_BAD_FLAGS;
    }
    if ((header->image_size == 0u) ||
        (header->image_size > PICOPEN_IMAGE_MAX_PAYLOAD_SIZE) ||
        ((header->image_size & 3u) != 0u) ||
        (header->payload_offset != PICOPEN_IMAGE_PAYLOAD_OFFSET) ||
        (header->entry_offset >= header->image_size) ||
        (header->vector_offset >= header->image_size)) {
        return PICOPEN_IMAGE_BAD_BOUNDS;
    }
    if (header->minimum_bootloader_version >
        PICOPEN_BOOTLOADER_FORMAT_VERSION) {
        return PICOPEN_IMAGE_BAD_FORMAT;
    }
    if (header->digest_algorithm != PICOPEN_DIGEST_SHA256) {
        return PICOPEN_IMAGE_BAD_ALGORITHM;
    }
    if ((header->signature_algorithm != PICOPEN_SIGNATURE_NONE) ||
        ((header->flags & PICOPEN_IMAGE_FLAG_DEVELOPMENT) == 0u) ||
        !bytes_are(header->key_id, sizeof(header->key_id), 0u) ||
        !bytes_are(header->signature, sizeof(header->signature), 0u)) {
        return PICOPEN_IMAGE_BAD_POLICY;
    }
    return PICOPEN_IMAGE_OK;
}

picopen_image_status_t picopen_validate_primary_image(
    picopen_validated_image_t *validated) {
    const uintptr_t slot_address =
        PICOPEN_FLASH_BASE + PICOPEN_PRIMARY_SLOT_OFFSET;
    const uint8_t *const slot = (const uint8_t *)slot_address;
    picopen_image_header_v1_t header;
    sha256_result_t calculated;
    pico_sha256_state_t sha_state;

    if ((validated == NULL) ||
        ((PICOPEN_PRIMARY_SLOT_OFFSET % PICOPEN_FLASH_ERASE_SIZE) != 0u)) {
        return PICOPEN_IMAGE_BAD_SLOT;
    }

    picopen_image_status_t status = validate_header(slot, &header);
    if (status != PICOPEN_IMAGE_OK) {
        return status;
    }

    if (pico_sha256_try_start(&sha_state, SHA256_BIG_ENDIAN, false) != PICO_OK) {
        return PICOPEN_IMAGE_DIGEST_UNAVAILABLE;
    }
    pico_sha256_update_blocking(&sha_state,
                                slot + header.payload_offset,
                                header.image_size);
    pico_sha256_finish(&sha_state, &calculated);

    if (memcmp(calculated.bytes, header.digest, sizeof(header.digest)) != 0) {
        return PICOPEN_IMAGE_BAD_DIGEST;
    }

    uint32_t initial_stack;
    uint32_t reset_handler;
    const uint8_t *const vectors =
        slot + header.payload_offset + header.vector_offset;
    memcpy(&initial_stack, vectors, sizeof(initial_stack));
    memcpy(&reset_handler, vectors + sizeof(initial_stack),
           sizeof(reset_handler));
    const uintptr_t payload_start = slot_address + header.payload_offset;
    const uintptr_t payload_end = payload_start + header.image_size;
    if ((initial_stack < SRAM_BASE) || (initial_stack > SRAM_END) ||
        ((initial_stack & 7u) != 0u) || ((reset_handler & 1u) == 0u) ||
        ((reset_handler & ~UINT32_C(1)) < payload_start) ||
        ((reset_handler & ~UINT32_C(1)) >= payload_end)) {
        return PICOPEN_IMAGE_BAD_BOUNDS;
    }

    validated->region_base = (uint32_t)payload_start;
    validated->region_size = header.image_size;
    validated->entry_address = reset_handler;
    validated->vector_address = validated->region_base + header.vector_offset;
    return PICOPEN_IMAGE_OK;
}

const char *picopen_image_status_string(picopen_image_status_t status) {
    static const char *const names[] = {
        "valid", "bad-slot", "bad-magic", "bad-format", "bad-header-crc",
        "bad-target", "bad-flags", "bad-bounds", "bad-algorithm",
        "bad-policy", "digest-unavailable", "bad-digest",
    };
    const size_t count = sizeof(names) / sizeof(names[0]);
    return ((unsigned)status < count) ? names[status] : "unknown";
}
