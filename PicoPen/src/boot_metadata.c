#include "picopen/boot_metadata.h"

#include <stddef.h>
#include <string.h>

#include "hardware/flash.h"
#include "hardware/sync.h"

#include "picopen/boot_format.h"

static uint32_t crc32(const uint8_t *data, size_t length) {
    uint32_t crc = UINT32_MAX;
    for (size_t index = 0u; index < length; ++index) {
        crc ^= data[index];
        for (uint bit = 0u; bit < 8u; ++bit) {
            const uint32_t mask = (uint32_t)-(int32_t)(crc & 1u);
            crc = (crc >> 1u) ^ (UINT32_C(0xEDB88320) & mask);
        }
    }
    return ~crc;
}

static bool metadata_valid(const picopen_boot_metadata_v1_t *metadata) {
    static const uint8_t magic[8] = PICOPEN_BOOT_METADATA_MAGIC_BYTES;
    static const uint8_t zero_reserved[224] = {0};
    if ((memcmp(metadata->magic, magic, sizeof(magic)) != 0) ||
        (metadata->format_version != PICOPEN_BOOT_METADATA_FORMAT_VERSION) ||
        (metadata->record_size != PICOPEN_BOOT_METADATA_RECORD_SIZE) ||
        ((metadata->flags & ~PICOPEN_BOOT_KNOWN_FLAGS) != 0u) ||
        ((metadata->flags & PICOPEN_BOOT_FLAG_PENDING) != 0u &&
         (metadata->flags & PICOPEN_BOOT_FLAG_CONFIRMED) != 0u) ||
        (metadata->attempt_count > PICOPEN_BOOT_MAX_ATTEMPTS) ||
        (metadata->selected_slot != PICOPEN_BOOT_SLOT_PRIMARY) ||
        (metadata->reserved16 != 0u) ||
        (memcmp(metadata->reserved, zero_reserved,
                sizeof(zero_reserved)) != 0)) {
        return false;
    }
    return metadata->crc32 ==
        crc32((const uint8_t *)metadata,
              offsetof(picopen_boot_metadata_v1_t, crc32));
}

static bool generation_newer(uint32_t candidate, uint32_t current) {
    return (int32_t)(candidate - current) > 0;
}

void picopen_boot_metadata_default(picopen_boot_metadata_v1_t *metadata) {
    static const uint8_t magic[8] = PICOPEN_BOOT_METADATA_MAGIC_BYTES;
    memset(metadata, 0, sizeof(*metadata));
    memcpy(metadata->magic, magic, sizeof(magic));
    metadata->format_version = PICOPEN_BOOT_METADATA_FORMAT_VERSION;
    metadata->record_size = PICOPEN_BOOT_METADATA_RECORD_SIZE;
    metadata->flags = PICOPEN_BOOT_FLAG_CONFIRMED;
    metadata->selected_slot = PICOPEN_BOOT_SLOT_PRIMARY;
}

bool picopen_boot_metadata_load(picopen_boot_metadata_v1_t *metadata) {
    const picopen_boot_metadata_v1_t *const copy_a =
        (const picopen_boot_metadata_v1_t *)(uintptr_t)
            (PICOPEN_FLASH_BASE + PICOPEN_BOOT_METADATA_A_OFFSET);
    const picopen_boot_metadata_v1_t *const copy_b =
        (const picopen_boot_metadata_v1_t *)(uintptr_t)
            (PICOPEN_FLASH_BASE + PICOPEN_BOOT_METADATA_B_OFFSET);
    const bool valid_a = metadata_valid(copy_a);
    const bool valid_b = metadata_valid(copy_b);
    if (!valid_a && !valid_b) {
        picopen_boot_metadata_default(metadata);
        return false;
    }
    const picopen_boot_metadata_v1_t *selected = copy_a;
    if (!valid_a || (valid_b && generation_newer(copy_b->generation,
                                                 copy_a->generation))) {
        selected = copy_b;
    }
    memcpy(metadata, selected, sizeof(*metadata));
    return true;
}

bool picopen_boot_metadata_store(picopen_boot_metadata_v1_t *metadata) {
    const uint32_t next_generation = metadata->generation + 1u;
    const uint32_t target_offset = (next_generation & 1u) != 0u
        ? PICOPEN_BOOT_METADATA_A_OFFSET
        : PICOPEN_BOOT_METADATA_B_OFFSET;
    metadata->generation = next_generation;
    metadata->crc32 = crc32((const uint8_t *)metadata,
                            offsetof(picopen_boot_metadata_v1_t, crc32));

    const uint32_t interrupt_state = save_and_disable_interrupts();
    flash_range_erase(target_offset, PICOPEN_FLASH_ERASE_SIZE);
    flash_range_program(target_offset, (const uint8_t *)metadata,
                        PICOPEN_BOOT_METADATA_RECORD_SIZE);
    restore_interrupts(interrupt_state);

    const picopen_boot_metadata_v1_t *const written =
        (const picopen_boot_metadata_v1_t *)(uintptr_t)
            (PICOPEN_FLASH_BASE + target_offset);
    return metadata_valid(written) &&
           (written->generation == metadata->generation);
}
