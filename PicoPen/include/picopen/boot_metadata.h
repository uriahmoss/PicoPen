#ifndef PICOPEN_BOOT_METADATA_H
#define PICOPEN_BOOT_METADATA_H

#include <stdbool.h>
#include <stdint.h>

#define PICOPEN_BOOT_METADATA_MAGIC_BYTES {'P','P','B','O','O','T','1','\0'}
#define PICOPEN_BOOT_METADATA_FORMAT_VERSION UINT16_C(1)
#define PICOPEN_BOOT_METADATA_RECORD_SIZE UINT16_C(256)
#define PICOPEN_BOOT_MAX_ATTEMPTS UINT8_C(3)

#define PICOPEN_BOOT_FLAG_PENDING UINT32_C(0x00000001)
#define PICOPEN_BOOT_FLAG_CONFIRMED UINT32_C(0x00000002)
#define PICOPEN_BOOT_KNOWN_FLAGS \
    (PICOPEN_BOOT_FLAG_PENDING | PICOPEN_BOOT_FLAG_CONFIRMED)

#define PICOPEN_BOOT_SLOT_PRIMARY UINT8_C(0)

typedef struct __attribute__((packed)) picopen_boot_metadata_v1 {
    uint8_t magic[8];
    uint16_t format_version;
    uint16_t record_size;
    uint32_t generation;
    uint32_t flags;
    uint8_t attempt_count;
    uint8_t selected_slot;
    uint16_t reserved16;
    uint32_t last_failure;
    uint8_t reserved[224];
    uint32_t crc32;
} picopen_boot_metadata_v1_t;

_Static_assert(sizeof(picopen_boot_metadata_v1_t) ==
                   PICOPEN_BOOT_METADATA_RECORD_SIZE,
               "PicoPen boot metadata must occupy one flash page");

bool picopen_boot_metadata_load(picopen_boot_metadata_v1_t *metadata);
bool picopen_boot_metadata_store(picopen_boot_metadata_v1_t *metadata);
void picopen_boot_metadata_default(picopen_boot_metadata_v1_t *metadata);

#endif
