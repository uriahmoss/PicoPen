#ifndef PICOPEN_BOARD_PINS_H
#define PICOPEN_BOARD_PINS_H

#include <stdint.h>

// ClockworkPi PicoCalc mainboard V2.0 pin map, verified against an assembled
// unit marked CPI 2.0. These constants do not themselves initialize GPIO.
#define PICOPEN_PICOCALC_MAINBOARD_REVISION UINT32_C(0x00020000)

#define PICOPEN_DISPLAY_SPI_INDEX 1u
#define PICOPEN_DISPLAY_SCK_PIN   10u
#define PICOPEN_DISPLAY_MOSI_PIN  11u
#define PICOPEN_DISPLAY_MISO_PIN  12u
#define PICOPEN_DISPLAY_CS_PIN    13u
#define PICOPEN_DISPLAY_DC_PIN    14u
#define PICOPEN_DISPLAY_RESET_PIN 15u

#define PICOPEN_KEYBOARD_I2C_INDEX 1u
#define PICOPEN_KEYBOARD_SDA_PIN   6u
#define PICOPEN_KEYBOARD_SCL_PIN   7u
#define PICOPEN_KEYBOARD_ADDRESS   UINT8_C(0x1F)

_Static_assert(PICOPEN_DISPLAY_SCK_PIN != PICOPEN_DISPLAY_MOSI_PIN,
               "display pins must be unique");
_Static_assert(PICOPEN_DISPLAY_MOSI_PIN != PICOPEN_DISPLAY_MISO_PIN,
               "display pins must be unique");
_Static_assert(PICOPEN_DISPLAY_MISO_PIN != PICOPEN_DISPLAY_CS_PIN,
               "display pins must be unique");
_Static_assert(PICOPEN_DISPLAY_CS_PIN != PICOPEN_DISPLAY_DC_PIN,
               "display pins must be unique");
_Static_assert(PICOPEN_DISPLAY_DC_PIN != PICOPEN_DISPLAY_RESET_PIN,
               "display pins must be unique");
_Static_assert(PICOPEN_KEYBOARD_SDA_PIN != PICOPEN_KEYBOARD_SCL_PIN,
               "keyboard pins must be unique");

#endif
