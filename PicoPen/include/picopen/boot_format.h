#ifndef PICOPEN_BOOT_FORMAT_H
#define PICOPEN_BOOT_FORMAT_H

#include <stdint.h>

// PicoPen boot contract v0.1. All offsets are relative to XIP flash at
// 0x10000000 and all multibyte on-flash values are little-endian.
#define PICOPEN_FLASH_BASE                 UINT32_C(0x10000000)
#define PICOPEN_FLASH_SIZE                 UINT32_C(0x00400000)
#define PICOPEN_FLASH_ERASE_SIZE           UINT32_C(0x00001000)

#define PICOPEN_BOOTLOADER_OFFSET          UINT32_C(0x00000000)
#define PICOPEN_BOOTLOADER_SIZE            UINT32_C(0x00040000)
#define PICOPEN_BOOT_METADATA_A_OFFSET     UINT32_C(0x00040000)
#define PICOPEN_BOOT_METADATA_B_OFFSET     UINT32_C(0x00041000)
#define PICOPEN_BOOT_RESERVED_OFFSET       UINT32_C(0x00042000)
#define PICOPEN_BOOT_RESERVED_SIZE         UINT32_C(0x0000E000)
#define PICOPEN_PRIMARY_SLOT_OFFSET        UINT32_C(0x00050000)
#define PICOPEN_CANDIDATE_SLOT_OFFSET      UINT32_C(0x00210000)
#define PICOPEN_IMAGE_SLOT_SIZE            UINT32_C(0x001C0000)
#define PICOPEN_PERSISTENT_OFFSET          UINT32_C(0x003D0000)
#define PICOPEN_PERSISTENT_SIZE            UINT32_C(0x00030000)

#define PICOPEN_IMAGE_MANIFEST_SIZE        UINT32_C(0x00001000)
#define PICOPEN_IMAGE_HEADER_SIZE          UINT32_C(0x00000100)
#define PICOPEN_IMAGE_PAYLOAD_OFFSET       PICOPEN_IMAGE_MANIFEST_SIZE
#define PICOPEN_IMAGE_MAX_PAYLOAD_SIZE     \
    (PICOPEN_IMAGE_SLOT_SIZE - PICOPEN_IMAGE_PAYLOAD_OFFSET)

#define PICOPEN_IMAGE_FORMAT_VERSION       UINT16_C(1)
#define PICOPEN_BOOTLOADER_FORMAT_VERSION  UINT32_C(1)
#define PICOPEN_IMAGE_MAGIC_BYTES          {'P','I','C','O','P','E','N','\0'}
#define PICOPEN_TARGET_PICO2_W             UINT32_C(0x50325700)
#define PICOPEN_DIGEST_SHA256              UINT16_C(1)
#define PICOPEN_SIGNATURE_NONE             UINT16_C(0)
#define PICOPEN_SIGNATURE_ED25519           UINT16_C(1)
#define PICOPEN_IMAGE_FLAG_DEVELOPMENT      UINT32_C(0x00000001)
#define PICOPEN_IMAGE_KNOWN_FLAGS            PICOPEN_IMAGE_FLAG_DEVELOPMENT

// Watchdog scratch request used to enter ROM BOOTSEL after a clean hardware
// reset, before firmware initializes USB.
#define PICOPEN_BOOTSEL_REQUEST_MAGIC        UINT32_C(0x50424F54)
#define PICOPEN_BOOTSEL_REQUEST_SCRATCH      0u
#define PICOPEN_BOOT_ATTEMPT_SCRATCH         1u
#define PICOPEN_BOOT_ATTEMPT_CHAINING        UINT32_C(0x5043484E)
#define PICOPEN_BOOT_ATTEMPT_OS_ENTERED      UINT32_C(0x504F5345)
#define PICOPEN_BOOT_ATTEMPT_USB_TIMEOUT     UINT32_C(0x50555342)
#define PICOPEN_BOOT_ATTEMPT_METADATA_ERROR  UINT32_C(0x504D4554)
#define PICOPEN_BOOT_ATTEMPT_TIMEOUT_MS      20000u
#define PICOPEN_OS_USB_WAIT_MS               15000u

typedef struct __attribute__((packed)) picopen_image_header_v1 {
    uint8_t magic[8];
    uint16_t format_version;
    uint16_t header_size;
    uint32_t flags;
    uint32_t target_id;
    uint32_t image_size;
    uint32_t payload_offset;
    uint32_t entry_offset;
    uint32_t vector_offset;
    uint16_t version_major;
    uint16_t version_minor;
    uint16_t version_patch;
    uint16_t version_reserved;
    uint64_t build_number;
    uint32_t minimum_bootloader_version;
    uint16_t digest_algorithm;
    uint16_t signature_algorithm;
    uint8_t key_id[16];
    uint8_t digest[32];
    uint8_t signature[64];
    uint8_t provenance[32];
    uint8_t reserved[48];
    uint32_t header_crc32;
} picopen_image_header_v1_t;

_Static_assert(sizeof(picopen_image_header_v1_t) == PICOPEN_IMAGE_HEADER_SIZE,
               "PicoPen image header must remain exactly 256 bytes");
_Static_assert(PICOPEN_PERSISTENT_OFFSET + PICOPEN_PERSISTENT_SIZE ==
                   PICOPEN_FLASH_SIZE,
               "PicoPen flash regions must cover the configured flash");

#endif
