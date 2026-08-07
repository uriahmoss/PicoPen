# Hardware Plan

## Baseline target

- ClockworkPi PicoCalc mainboard V2.0
- Raspberry Pi Pico 2 W with RP2350
- 4 MB onboard flash and 520 KB internal SRAM
- PicoCalc 8 MB PSRAM
- PicoCalc 320 x 320 SPI display
- PicoCalc I2C keyboard/power controller
- PicoCalc SPI SD card

The display and keyboard pin map is transcribed from ClockworkPi's official
V2.0 schematic and reference firmware. The assembled unit was physically
confirmed by its `CPI 2.0` mainboard marking before GPIO driver work began.

| Function | RP2350 resource | GPIO | Verification |
|---|---|---:|---|
| Keyboard SDA | I2C1 SDA | 6 | schematic + official code + CPI 2.0 unit |
| Keyboard SCL | I2C1 SCL | 7 | schematic + official code + CPI 2.0 unit |
| Display SCK | SPI1 SCK | 10 | schematic + official code + CPI 2.0 unit |
| Display MOSI | SPI1 TX | 11 | schematic + official code + CPI 2.0 unit |
| Display MISO | SPI1 RX | 12 | schematic + official code + CPI 2.0 unit |
| Display CS | GPIO output | 13 | schematic + official code + CPI 2.0 unit |
| Display D/C | GPIO output | 14 | schematic + official code + CPI 2.0 unit |
| Display reset | GPIO output | 15 | schematic + official code + CPI 2.0 unit |

The keyboard/power controller uses address `0x1F`. Bus speed and controller
protocol will be fixed only after the official keyboard firmware version on the
assembled unit is identified. SD, audio, PSRAM, and expansion pins remain
inactive and are not yet accepted into the PicoPen board contract.

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
