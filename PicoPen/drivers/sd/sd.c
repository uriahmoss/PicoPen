#include "picopen/sd.h"

#include <stddef.h>

#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"

#include "picopen/board_pins.h"

#define SD_INIT_BAUD_HZ       400000u
#define SD_IDLE_CLOCK_BYTES   16u
#define SD_RESPONSE_ATTEMPTS  100u
#define SD_IDLE_ATTEMPTS      5u
#define SD_INIT_ATTEMPTS      100u
#define SD_INIT_DELAY_MS      10u
#define SD_SOFTWARE_HALF_PERIOD_US 2u
#define SD_READY_TIMEOUT_MS   500u
#define SD_DATA_TIMEOUT_MS    500u
#define SD_POWER_STABILIZATION_MS 1500u
#define SD_SECTOR_SIZE        512u
#define SD_MAX_READ_BLOCKS    4u

#define SD_COMMAND_GO_IDLE       0u
#define SD_COMMAND_SEND_IF_COND  8u
#define SD_COMMAND_APP          55u
#define SD_COMMAND_READ_OCR     58u
#define SD_COMMAND_READ_SINGLE  17u
#define SD_APP_COMMAND_INIT     41u

#define SD_R1_IDLE          0x01u
#define SD_R1_ILLEGAL       0x04u
#define SD_R1_NO_RESPONSE   0xFFu
#define SD_IF_COND_ARGUMENT UINT32_C(0x000001AA)
#define SD_INIT_HCS         UINT32_C(0x40000000)
#define SD_OCR_POWERED      UINT32_C(0x80000000)
#define SD_OCR_HIGH_CAPACITY UINT32_C(0x40000000)
#define SD_DATA_TOKEN        0xFEu
#define SD_BOOT_SIGNATURE_LO 0x55u
#define SD_BOOT_SIGNATURE_HI 0xAAu

static spi_inst_t *const sd_spi = spi0;
static bool software_spi;
static bool transport_ready;
static bool transport_high_capacity;

static uint8_t crc7(const uint8_t *bytes, size_t length) {
    const uint8_t polynomial = 0x89u;
    uint8_t crc = 0u;
    for (size_t index = 0u; index < length; ++index) {
        crc ^= bytes[index];
        for (unsigned int bit = 0u; bit < 8u; ++bit) {
            crc = (crc & 0x80u) != 0u
                      ? (uint8_t)((crc << 1u) ^ (polynomial << 1u))
                      : (uint8_t)(crc << 1u);
        }
    }
    return (uint8_t)(crc >> 1u);
}

static uint8_t transfer(uint8_t value) {
    if (software_spi) {
        uint8_t result = 0u;
        for (uint8_t mask = 0x80u; mask != 0u; mask >>= 1u) {
            gpio_put(PICOPEN_SD_MOSI_PIN, (value & mask) != 0u);
            sleep_us(SD_SOFTWARE_HALF_PERIOD_US);
            gpio_put(PICOPEN_SD_SCK_PIN, true);
            result = (uint8_t)((result << 1u) |
                               (gpio_get(PICOPEN_SD_MISO_PIN) ? 1u : 0u));
            sleep_us(SD_SOFTWARE_HALF_PERIOD_US);
            gpio_put(PICOPEN_SD_SCK_PIN, false);
        }
        return result;
    }
    uint8_t result = 0xFFu;
    spi_write_read_blocking(sd_spi, &value, &result, 1u);
    return result;
}

static void configure_software_spi(void) {
    spi_deinit(sd_spi);
    software_spi = true;
    gpio_set_function(PICOPEN_SD_MISO_PIN, GPIO_FUNC_SIO);
    gpio_set_function(PICOPEN_SD_SCK_PIN, GPIO_FUNC_SIO);
    gpio_set_function(PICOPEN_SD_MOSI_PIN, GPIO_FUNC_SIO);
    gpio_set_dir(PICOPEN_SD_MISO_PIN, GPIO_IN);
    gpio_set_dir(PICOPEN_SD_SCK_PIN, GPIO_OUT);
    gpio_set_dir(PICOPEN_SD_MOSI_PIN, GPIO_OUT);
    gpio_put(PICOPEN_SD_SCK_PIN, false);
    gpio_put(PICOPEN_SD_MOSI_PIN, true);
    gpio_put(PICOPEN_SD_CS_PIN, true);
    for (size_t index = 0u; index < SD_IDLE_CLOCK_BYTES; ++index) {
        (void)transfer(0xFFu);
    }
}

static void deselect(void) {
    gpio_put(PICOPEN_SD_CS_PIN, true);
    (void)transfer(0xFFu);
}

static bool select_and_wait_ready(void) {
    gpio_put(PICOPEN_SD_CS_PIN, false);
    (void)transfer(0xFFu);
    const absolute_time_t deadline = make_timeout_time_ms(SD_READY_TIMEOUT_MS);
    do {
        if (transfer(0xFFu) == 0xFFu) {
            return true;
        }
    } while (!time_reached(deadline));
    deselect();
    return false;
}

static void finish_transport(void) {
    deselect();
    if (!software_spi) {
        spi_deinit(sd_spi);
    }
    transport_ready = false;
}

static uint8_t command_begin(uint8_t index, uint32_t argument) {
    uint8_t packet[] = {
        (uint8_t)(0x40u | index),
        (uint8_t)(argument >> 24u),
        (uint8_t)(argument >> 16u),
        (uint8_t)(argument >> 8u),
        (uint8_t)argument,
        0u,
    };
    packet[5] = (uint8_t)((crc7(packet, 5u) << 1u) | 1u);
    if (!select_and_wait_ready()) {
        return SD_R1_NO_RESPONSE;
    }
    for (size_t index_packet = 0u; index_packet < sizeof(packet);
         ++index_packet) {
        (void)transfer(packet[index_packet]);
    }

    uint8_t response = SD_R1_NO_RESPONSE;
    for (size_t attempt = 0u; attempt < SD_RESPONSE_ATTEMPTS; ++attempt) {
        response = transfer(0xFFu);
        if ((response & 0x80u) == 0u) {
            break;
        }
    }
    return response;
}

static uint8_t command(uint8_t index, uint32_t argument,
                       uint8_t *extra, size_t extra_length) {
    const uint8_t response = command_begin(index, argument);
    for (size_t index_extra = 0u; index_extra < extra_length; ++index_extra) {
        extra[index_extra] = transfer(0xFFu);
    }
    deselect();
    return response;
}

static uint32_t decode_u32(const uint8_t bytes[4]) {
    return ((uint32_t)bytes[0] << 24u) |
           ((uint32_t)bytes[1] << 16u) |
           ((uint32_t)bytes[2] << 8u) |
           bytes[3];
}

static uint32_t decode_le_u32(const uint8_t bytes[4]) {
    return ((uint32_t)bytes[3] << 24u) |
           ((uint32_t)bytes[2] << 16u) |
           ((uint32_t)bytes[1] << 8u) |
           bytes[0];
}

static bool bytes_equal(const uint8_t *left, const char *right,
                        size_t length) {
    for (size_t index = 0u; index < length; ++index) {
        if (left[index] != (uint8_t)right[index]) {
            return false;
        }
    }
    return true;
}

static picopen_sd_filesystem_t detect_filesystem(const uint8_t sector[512]) {
    if (bytes_equal(&sector[3], "EXFAT   ", 8u)) {
        return PICOPEN_SD_FILESYSTEM_EXFAT;
    }
    if (bytes_equal(&sector[54], "FAT12   ", 8u) ||
        bytes_equal(&sector[54], "FAT16   ", 8u) ||
        bytes_equal(&sector[82], "FAT32   ", 8u)) {
        return PICOPEN_SD_FILESYSTEM_FAT;
    }
    return PICOPEN_SD_FILESYSTEM_UNKNOWN;
}

static bool read_sector(uint32_t lba, bool high_capacity,
                        uint8_t sector[SD_SECTOR_SIZE], uint8_t *response) {
    if (!high_capacity && (lba > (UINT32_MAX / SD_SECTOR_SIZE))) {
        return false;
    }
    const uint32_t address = high_capacity ? lba : lba * SD_SECTOR_SIZE;
    *response = command_begin(SD_COMMAND_READ_SINGLE, address);
    if (*response != 0u) {
        deselect();
        return false;
    }
    const absolute_time_t deadline = make_timeout_time_ms(SD_DATA_TIMEOUT_MS);
    uint8_t token;
    do {
        token = transfer(0xFFu);
        if (token == SD_DATA_TOKEN) {
            break;
        }
    } while (!time_reached(deadline));
    if (token != SD_DATA_TOKEN) {
        deselect();
        return false;
    }
    for (size_t index = 0u; index < SD_SECTOR_SIZE; ++index) {
        sector[index] = transfer(0xFFu);
    }
    (void)transfer(0xFFu);
    (void)transfer(0xFFu);
    deselect();
    return true;
}

static bool inspect_boot_sectors(picopen_sd_info_t *info) {
    uint8_t sector[SD_SECTOR_SIZE];
    if (!read_sector(0u, info->high_capacity, sector, &info->last_response)) {
        info->status = PICOPEN_SD_READ_ERROR;
        return false;
    }
    info->sector_read = true;
    info->filesystem = detect_filesystem(sector);
    if (info->filesystem != PICOPEN_SD_FILESYSTEM_UNKNOWN) {
        return true;
    }
    if ((sector[510] != SD_BOOT_SIGNATURE_LO) ||
        (sector[511] != SD_BOOT_SIGNATURE_HI)) {
        return true;
    }
    const uint8_t *const first_partition = &sector[446];
    info->first_partition_lba = decode_le_u32(&first_partition[8]);
    info->partitioned = (first_partition[4] != 0u) &&
                        (info->first_partition_lba != 0u);
    if (!info->partitioned) {
        return true;
    }
    if (!read_sector(info->first_partition_lba, info->high_capacity, sector,
                     &info->last_response)) {
        info->status = PICOPEN_SD_READ_ERROR;
        return false;
    }
    info->filesystem = detect_filesystem(sector);
    return true;
}

static bool enter_idle(picopen_sd_info_t *info) {
    for (size_t attempt = 0u; attempt < SD_IDLE_ATTEMPTS; ++attempt) {
        info->last_response = command(SD_COMMAND_GO_IDLE, 0u, NULL, 0u);
        if (info->last_response == SD_R1_IDLE) {
            return true;
        }
        sleep_ms(SD_INIT_DELAY_MS);
    }
    info->status = PICOPEN_SD_NO_RESPONSE;
    return false;
}

static bool check_interface_condition(picopen_sd_info_t *info) {
    uint8_t condition[4];
    info->last_response = command(SD_COMMAND_SEND_IF_COND, SD_IF_COND_ARGUMENT,
                                  condition, sizeof(condition));
    if ((info->last_response & SD_R1_ILLEGAL) != 0u) {
        info->version_2 = false;
        return true;
    }
    info->version_2 = true;
    if ((info->last_response != SD_R1_IDLE) ||
        (condition[2] != 0x01u) || (condition[3] != 0xAAu)) {
        info->status = PICOPEN_SD_BAD_VOLTAGE;
        return false;
    }
    return true;
}

static bool wait_until_ready(picopen_sd_info_t *info) {
    const uint32_t argument = info->version_2 ? SD_INIT_HCS : 0u;
    for (size_t attempt = 0u; attempt < SD_INIT_ATTEMPTS; ++attempt) {
        info->last_response = command(SD_COMMAND_APP, 0u, NULL, 0u);
        if (info->last_response > SD_R1_IDLE) {
            break;
        }
        info->last_response = command(SD_APP_COMMAND_INIT, argument, NULL, 0u);
        if (info->last_response == 0u) {
            return true;
        }
        sleep_ms(SD_INIT_DELAY_MS);
    }
    info->status = PICOPEN_SD_INIT_TIMEOUT;
    return false;
}

bool picopen_sd_identify(picopen_sd_info_t *info) {
    if (info == NULL) {
        return false;
    }
    *info = (picopen_sd_info_t){0};
    info->last_response = SD_R1_NO_RESPONSE;
    software_spi = false;
    transport_ready = false;

    gpio_init(PICOPEN_SD_DETECT_PIN);
    gpio_set_dir(PICOPEN_SD_DETECT_PIN, GPIO_IN);
    gpio_pull_up(PICOPEN_SD_DETECT_PIN);
    info->card_detected = !gpio_get(PICOPEN_SD_DETECT_PIN);
    if (info->card_detected) {
        sleep_ms(SD_POWER_STABILIZATION_MS);
    }

    gpio_init(PICOPEN_SD_CS_PIN);
    gpio_put(PICOPEN_SD_CS_PIN, true);
    gpio_set_dir(PICOPEN_SD_CS_PIN, GPIO_OUT);
    gpio_pull_up(PICOPEN_SD_MISO_PIN);
    gpio_set_function(PICOPEN_SD_MISO_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICOPEN_SD_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICOPEN_SD_MOSI_PIN, GPIO_FUNC_SPI);
    gpio_set_drive_strength(PICOPEN_SD_SCK_PIN, GPIO_DRIVE_STRENGTH_8MA);
    gpio_set_drive_strength(PICOPEN_SD_MOSI_PIN, GPIO_DRIVE_STRENGTH_8MA);
    gpio_set_drive_strength(PICOPEN_SD_CS_PIN, GPIO_DRIVE_STRENGTH_8MA);
    gpio_set_slew_rate(PICOPEN_SD_CS_PIN, GPIO_SLEW_RATE_SLOW);
    gpio_set_slew_rate(PICOPEN_SD_SCK_PIN, GPIO_SLEW_RATE_FAST);
    gpio_set_slew_rate(PICOPEN_SD_MOSI_PIN, GPIO_SLEW_RATE_FAST);
    gpio_set_input_hysteresis_enabled(PICOPEN_SD_MISO_PIN, true);
    spi_init(sd_spi, SD_INIT_BAUD_HZ);
    spi_set_format(sd_spi, 8u, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    sleep_ms(1u);
    for (size_t index = 0u; index < SD_IDLE_CLOCK_BYTES; ++index) {
        (void)transfer(0xFFu);
    }

    if (!enter_idle(info)) {
        configure_software_spi();
        info->software_spi = true;
        if (!enter_idle(info)) {
            finish_transport();
            return false;
        }
    }
    if (!check_interface_condition(info) ||
        !wait_until_ready(info)) {
        finish_transport();
        return false;
    }

    uint8_t ocr_bytes[4];
    info->last_response = command(SD_COMMAND_READ_OCR, 0u,
                                  ocr_bytes, sizeof(ocr_bytes));
    info->ocr = decode_u32(ocr_bytes);
    if ((info->last_response != 0u) ||
        ((info->ocr & SD_OCR_POWERED) == 0u)) {
        info->status = PICOPEN_SD_OCR_ERROR;
        finish_transport();
        return false;
    }
    info->high_capacity = (info->ocr & SD_OCR_HIGH_CAPACITY) != 0u;
    if (!inspect_boot_sectors(info)) {
        finish_transport();
        return false;
    }
    info->status = PICOPEN_SD_READY;
    transport_high_capacity = info->high_capacity;
    transport_ready = true;
    return true;
}

bool picopen_sd_read_blocks(uint32_t first_lba, uint8_t *buffer,
                            uint32_t block_count) {
    if (!transport_ready || (buffer == NULL) || (block_count == 0u) ||
        (block_count > SD_MAX_READ_BLOCKS) ||
        (first_lba > UINT32_MAX - (block_count - 1u))) {
        return false;
    }
    for (uint32_t index = 0u; index < block_count; ++index) {
        uint8_t response = SD_R1_NO_RESPONSE;
        if (!read_sector(first_lba + index, transport_high_capacity,
                         &buffer[index * SD_SECTOR_SIZE], &response)) {
            return false;
        }
    }
    return true;
}

void picopen_sd_close(void) {
    if (transport_ready) {
        finish_transport();
    }
}

const char *picopen_sd_status_name(picopen_sd_status_t status) {
    static const char *const names[] = {
        "ready", "no-response", "bad-voltage", "init-timeout", "ocr-error",
        "read-error",
    };
    if ((unsigned int)status >= sizeof(names) / sizeof(names[0])) {
        return "unknown";
    }
    return names[status];
}

const char *picopen_sd_filesystem_name(picopen_sd_filesystem_t filesystem) {
    static const char *const names[] = {"unknown", "FAT", "exFAT"};
    if ((unsigned int)filesystem >= sizeof(names) / sizeof(names[0])) {
        return "unknown";
    }
    return names[filesystem];
}
