#include "picopen/sd.h"

#include <stddef.h>

#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"

#include "picopen/board_pins.h"

#define SD_INIT_BAUD_HZ       400000u
#define SD_IDLE_CLOCK_BYTES   10u
#define SD_RESPONSE_ATTEMPTS  8u
#define SD_INIT_ATTEMPTS      100u
#define SD_INIT_DELAY_MS      10u

#define SD_COMMAND_GO_IDLE       0u
#define SD_COMMAND_SEND_IF_COND  8u
#define SD_COMMAND_APP          55u
#define SD_COMMAND_READ_OCR     58u
#define SD_APP_COMMAND_INIT     41u

#define SD_R1_IDLE          0x01u
#define SD_R1_ILLEGAL       0x04u
#define SD_R1_NO_RESPONSE   0xFFu
#define SD_IF_COND_ARGUMENT UINT32_C(0x000001AA)
#define SD_INIT_HCS         UINT32_C(0x40000000)
#define SD_OCR_POWERED      UINT32_C(0x80000000)
#define SD_OCR_HIGH_CAPACITY UINT32_C(0x40000000)

static spi_inst_t *const sd_spi = spi0;

static uint8_t transfer(uint8_t value) {
    uint8_t result = 0xFFu;
    spi_write_read_blocking(sd_spi, &value, &result, 1u);
    return result;
}

static void deselect(void) {
    gpio_put(PICOPEN_SD_CS_PIN, true);
    (void)transfer(0xFFu);
}

static uint8_t command(uint8_t index, uint32_t argument, uint8_t crc,
                       uint8_t *extra, size_t extra_length) {
    const uint8_t packet[] = {
        (uint8_t)(0x40u | index),
        (uint8_t)(argument >> 24u),
        (uint8_t)(argument >> 16u),
        (uint8_t)(argument >> 8u),
        (uint8_t)argument,
        crc,
    };
    deselect();
    gpio_put(PICOPEN_SD_CS_PIN, false);
    (void)transfer(0xFFu);
    spi_write_blocking(sd_spi, packet, sizeof(packet));

    uint8_t response = SD_R1_NO_RESPONSE;
    for (size_t attempt = 0u; attempt < SD_RESPONSE_ATTEMPTS; ++attempt) {
        response = transfer(0xFFu);
        if ((response & 0x80u) == 0u) {
            break;
        }
    }
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

static bool enter_idle(picopen_sd_info_t *info) {
    for (size_t attempt = 0u; attempt < SD_INIT_ATTEMPTS; ++attempt) {
        info->last_response = command(SD_COMMAND_GO_IDLE, 0u, 0x95u, NULL, 0u);
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
                                  0x87u, condition, sizeof(condition));
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
        info->last_response = command(SD_COMMAND_APP, 0u, 0x01u, NULL, 0u);
        if (info->last_response > SD_R1_IDLE) {
            break;
        }
        info->last_response = command(SD_APP_COMMAND_INIT, argument, 0x01u,
                                      NULL, 0u);
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

    gpio_init(PICOPEN_SD_CS_PIN);
    gpio_set_dir(PICOPEN_SD_CS_PIN, GPIO_OUT);
    gpio_put(PICOPEN_SD_CS_PIN, true);
    gpio_pull_up(PICOPEN_SD_MISO_PIN);
    gpio_set_function(PICOPEN_SD_MISO_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICOPEN_SD_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICOPEN_SD_MOSI_PIN, GPIO_FUNC_SPI);
    spi_init(sd_spi, SD_INIT_BAUD_HZ);
    spi_set_format(sd_spi, 8u, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    sleep_ms(1u);
    for (size_t index = 0u; index < SD_IDLE_CLOCK_BYTES; ++index) {
        (void)transfer(0xFFu);
    }

    if (!enter_idle(info) || !check_interface_condition(info) ||
        !wait_until_ready(info)) {
        deselect();
        spi_deinit(sd_spi);
        return false;
    }

    uint8_t ocr_bytes[4];
    info->last_response = command(SD_COMMAND_READ_OCR, 0u, 0x01u,
                                  ocr_bytes, sizeof(ocr_bytes));
    info->ocr = decode_u32(ocr_bytes);
    if ((info->last_response != 0u) ||
        ((info->ocr & SD_OCR_POWERED) == 0u)) {
        info->status = PICOPEN_SD_OCR_ERROR;
        spi_deinit(sd_spi);
        return false;
    }
    info->high_capacity = (info->ocr & SD_OCR_HIGH_CAPACITY) != 0u;
    info->status = PICOPEN_SD_READY;
    spi_deinit(sd_spi);
    return true;
}

const char *picopen_sd_status_name(picopen_sd_status_t status) {
    static const char *const names[] = {
        "ready", "no-response", "bad-voltage", "init-timeout", "ocr-error",
    };
    if ((unsigned int)status >= sizeof(names) / sizeof(names[0])) {
        return "unknown";
    }
    return names[status];
}
