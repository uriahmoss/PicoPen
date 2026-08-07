# Hardware Plan

## Baseline target

- ClockworkPi PicoCalc mainboard V2.0
- Raspberry Pi Pico 2 W with RP2350
- 4 MB onboard flash and 520 KB internal SRAM
- PicoCalc 8 MB PSRAM
- PicoCalc 320 x 320 SPI display
- PicoCalc I2C keyboard/power controller
- PicoCalc SPI SD card

Pin assignments are deliberately omitted until they are transcribed from the
official schematic and checked against an assembled unit. A wrong pin assignment
can contend with the display, SD card, audio, keyboard controller, or power
management circuitry.

## Bring-up order

1. SWD and serial logging
2. Reset reason and watchdog
3. Display initialization and diagnostic pattern
4. Keyboard controller and power events
5. SD card in read-only mode
6. Filesystem write and power-loss testing
7. PSRAM identification and memory test
8. Wi-Fi and Bluetooth initialization
9. Side expansion connector characterization
10. Attachment discovery and protected power

## Initial development attachments

- 38 kHz IR receiver and protected high-current IR transmitter
- Region-matched CC1101 SPI module and antenna
- Reputable PN532 breakout in SPI mode
- DS18B20 and DS1990A test devices with protected 1-Wire probe
- MCP3008 and ADS1115 ADC modules
- Logic analyzer and Raspberry Pi Debug Probe

## Electrical rules

- Treat all RP2350 GPIO as 3.3 V logic.
- Do not power radios, IR emitters, or USB peripherals directly from a GPIO.
- Do not assume a module labelled "3.3/5 V" has safe 3.3 V logic inputs.
- Share grounds before signals unless galvanic isolation is intentionally used.
- Apply current limiting, ESD protection, and backfeed protection at external
  connectors.
- Begin radio development in receive-only mode.
- Never attach probes to an unknown energized circuit before measuring it.

## Field Dock concept

Revision A will be a carrier board with replaceable modules and test points.
Revision B may integrate proven digital and RF sections. NFC and LF RFID antenna
sections remain mechanically separable until their tuning is validated in the
final enclosure.
