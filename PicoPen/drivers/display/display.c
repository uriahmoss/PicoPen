#include "picopen/display.h"

#include <stddef.h>
#include <stdint.h>

#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"

#include "picopen/board_pins.h"

#define DISPLAY_WIDTH  320u
#define DISPLAY_HEIGHT 320u
#define DISPLAY_INIT_BAUD_HZ 6000000u
#define DISPLAY_DATA_BAUD_HZ 25000000u

#define DISPLAY_COMMAND_SLEEP_OUT  0x11u
#define DISPLAY_COMMAND_INVERT_ON  0x21u
#define DISPLAY_COMMAND_ON         0x29u
#define DISPLAY_COMMAND_COLUMN     0x2Au
#define DISPLAY_COMMAND_ROW        0x2Bu
#define DISPLAY_COMMAND_WRITE      0x2Cu
#define DISPLAY_COMMAND_ACCESS     0x36u
#define DISPLAY_COMMAND_PIXEL      0x3Au

typedef struct display_init_command {
    uint8_t command;
    uint8_t length;
    uint8_t data[15];
} display_init_command_t;

static spi_inst_t *const display_spi = spi1;

static void write_command(uint8_t command, const uint8_t *data,
                          size_t length) {
    gpio_put(PICOPEN_DISPLAY_DC_PIN, false);
    gpio_put(PICOPEN_DISPLAY_CS_PIN, false);
    spi_write_blocking(display_spi, &command, 1u);
    if (length != 0u) {
        gpio_put(PICOPEN_DISPLAY_DC_PIN, true);
        spi_write_blocking(display_spi, data, length);
    }
    gpio_put(PICOPEN_DISPLAY_CS_PIN, true);
}

static void set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    const uint8_t columns[] = {
        (uint8_t)(x0 >> 8u), (uint8_t)x0,
        (uint8_t)(x1 >> 8u), (uint8_t)x1,
    };
    const uint8_t rows[] = {
        (uint8_t)(y0 >> 8u), (uint8_t)y0,
        (uint8_t)(y1 >> 8u), (uint8_t)y1,
    };
    write_command(DISPLAY_COMMAND_COLUMN, columns, sizeof(columns));
    write_command(DISPLAY_COMMAND_ROW, rows, sizeof(rows));
    write_command(DISPLAY_COMMAND_WRITE, NULL, 0u);
}

static void fill_rectangle(uint16_t x, uint16_t y, uint16_t width,
                           uint16_t height, uint32_t rgb) {
    if ((width == 0u) || (height == 0u) ||
        ((uint32_t)x + width > DISPLAY_WIDTH) ||
        ((uint32_t)y + height > DISPLAY_HEIGHT)) {
        return;
    }
    set_window(x, y, (uint16_t)(x + width - 1u),
               (uint16_t)(y + height - 1u));
    uint8_t pixels[96];
    for (size_t offset = 0u; offset < sizeof(pixels); offset += 3u) {
        pixels[offset] = (uint8_t)((rgb >> 16u) & 0xFCu);
        pixels[offset + 1u] = (uint8_t)((rgb >> 8u) & 0xFCu);
        pixels[offset + 2u] = (uint8_t)(rgb & 0xFCu);
    }
    uint32_t remaining = (uint32_t)width * height;
    gpio_put(PICOPEN_DISPLAY_DC_PIN, true);
    gpio_put(PICOPEN_DISPLAY_CS_PIN, false);
    while (remaining != 0u) {
        const uint32_t count = remaining > 32u ? 32u : remaining;
        spi_write_blocking(display_spi, pixels, count * 3u);
        remaining -= count;
    }
    gpio_put(PICOPEN_DISPLAY_CS_PIN, true);
}

bool picopen_display_init(void) {
    static const display_init_command_t commands[] = {
        {0xE0u, 15u, {0x00u, 0x03u, 0x09u, 0x08u, 0x16u, 0x0Au, 0x3Fu,
                      0x78u, 0x4Cu, 0x09u, 0x0Au, 0x08u, 0x16u, 0x1Au, 0x0Fu}},
        {0xE1u, 15u, {0x00u, 0x16u, 0x19u, 0x03u, 0x0Fu, 0x05u, 0x32u,
                      0x45u, 0x46u, 0x04u, 0x0Eu, 0x0Du, 0x35u, 0x37u, 0x0Fu}},
        {0xC0u, 2u, {0x17u, 0x15u}},
        {0xC1u, 1u, {0x41u}},
        {0xC5u, 3u, {0x00u, 0x12u, 0x80u}},
        {DISPLAY_COMMAND_ACCESS, 1u, {0x48u}},
        {DISPLAY_COMMAND_PIXEL, 1u, {0x66u}},
        {0xB0u, 1u, {0x00u}},
        {0xB1u, 1u, {0xA0u}},
        {DISPLAY_COMMAND_INVERT_ON, 0u, {0u}},
        {0xB4u, 1u, {0x02u}},
        {0xB6u, 3u, {0x02u, 0x02u, 0x3Bu}},
        {0xB7u, 1u, {0xC6u}},
        {0xE9u, 1u, {0x00u}},
        {0xF7u, 4u, {0xA9u, 0x51u, 0x2Cu, 0x82u}},
    };

    gpio_init(PICOPEN_DISPLAY_CS_PIN);
    gpio_init(PICOPEN_DISPLAY_DC_PIN);
    gpio_init(PICOPEN_DISPLAY_RESET_PIN);
    gpio_put(PICOPEN_DISPLAY_CS_PIN, true);
    gpio_put(PICOPEN_DISPLAY_DC_PIN, true);
    gpio_put(PICOPEN_DISPLAY_RESET_PIN, true);
    gpio_set_dir(PICOPEN_DISPLAY_CS_PIN, GPIO_OUT);
    gpio_set_dir(PICOPEN_DISPLAY_DC_PIN, GPIO_OUT);
    gpio_set_dir(PICOPEN_DISPLAY_RESET_PIN, GPIO_OUT);

    spi_init(display_spi, DISPLAY_INIT_BAUD_HZ);
    gpio_set_function(PICOPEN_DISPLAY_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICOPEN_DISPLAY_MOSI_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICOPEN_DISPLAY_MISO_PIN, GPIO_FUNC_SPI);

    sleep_ms(10u);
    gpio_put(PICOPEN_DISPLAY_RESET_PIN, false);
    sleep_ms(10u);
    gpio_put(PICOPEN_DISPLAY_RESET_PIN, true);
    sleep_ms(200u);

    for (size_t index = 0u; index <
         (sizeof(commands) / sizeof(commands[0])); ++index) {
        write_command(commands[index].command, commands[index].data,
                      commands[index].length);
    }
    write_command(DISPLAY_COMMAND_SLEEP_OUT, NULL, 0u);
    sleep_ms(120u);
    write_command(DISPLAY_COMMAND_ON, NULL, 0u);
    sleep_ms(120u);
    spi_set_baudrate(display_spi, DISPLAY_DATA_BAUD_HZ);
    return true;
}

void picopen_display_draw_diagnostic(void) {
    static const uint32_t bands[] = {
        UINT32_C(0x080020), UINT32_C(0x240046), UINT32_C(0x5C087D),
        UINT32_C(0xB5179E), UINT32_C(0xFF2BD6), UINT32_C(0x00E5FF),
    };
    const uint16_t band_height = DISPLAY_HEIGHT /
        (uint16_t)(sizeof(bands) / sizeof(bands[0]));
    for (size_t index = 0u; index < sizeof(bands) / sizeof(bands[0]); ++index) {
        const uint16_t y = (uint16_t)(index * band_height);
        const uint16_t height = index == 5u
            ? (uint16_t)(DISPLAY_HEIGHT - y)
            : band_height;
        fill_rectangle(0u, y, DISPLAY_WIDTH, height, bands[index]);
    }
    for (uint16_t coordinate = 0u; coordinate < DISPLAY_WIDTH;
         coordinate += 40u) {
        fill_rectangle(coordinate, 0u, 1u, DISPLAY_HEIGHT,
                       UINT32_C(0xFFCC00));
        fill_rectangle(0u, coordinate, DISPLAY_WIDTH, 1u,
                       UINT32_C(0xFFCC00));
    }
}
