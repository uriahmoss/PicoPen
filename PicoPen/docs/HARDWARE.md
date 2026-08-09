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
| SD MISO | SPI0 RX | 16 | schematic + official code + user-approved contract |
| SD CS | GPIO output | 17 | schematic + official code + user-approved contract |
| SD SCK | SPI0 SCK | 18 | schematic + official code + user-approved contract |
| SD MOSI | SPI0 TX | 19 | schematic + official code + user-approved contract |
| SD detect | GPIO input, active low | 22 | schematic + official SD booter |

The keyboard/power controller uses address `0x1F`. Bus speed and controller
protocol will be fixed only after the official keyboard firmware version on the
assembled unit is identified. The user approved the SD mapping after review of
the physical Pico pin equivalents. Audio, PSRAM, and expansion pins remain
inactive and are not yet accepted into the PicoPen board contract.

## Display acceptance

The CPI 2.0 unit successfully rendered the complete 320 x 320 synthwave
diagnostic using SPI1, RGB666 transfers, and the verified GPIO 10-15 mapping.
Color bands and the 40-pixel alignment grid were visually correct. This accepts
the blocking display transport for bring-up; framebuffer ownership, terminal
glyphs, DMA, and partial redraw scheduling remain separate work.

The follow-on 40 x 20 fixed-cell terminal diagnostic rendered every expected
boot-console line legibly, with correct orientation and no screen-edge clipping.
This accepts the initial glyph and dirty-cell path. The current repository-owned
font intentionally covers boot-console characters; a complete UI font and ANSI
parser remain later UI work.

## SD bring-up

The first physical SPI0 identification attempts returned no response to CMD0
(`R1=0xFF`) while the STM32 keyboard/power controller was not responding on
I2C. After recovering the official keyboard BIOS v1.6, PicoPen identified the
inserted card over hardware SPI as SDHC/SDXC v2 with OCR `0xC0FF8000`. This
accepts the GPIO 16-19 transport and active-low GPIO 22 card detect on the CPI
2.0 unit. The current driver intentionally exposes no block write, erase,
formatting, or filesystem mutation operation.

The follow-on bounded read test successfully read LBA 0 and the first MBR
partition boot sector, identifying a FAT filesystem beginning at LBA 2048.
Keyboard input remained responsive after the probe. The implementation mounts
nothing and contains no block-write, erase, or formatting interface.

The next storage diagnostic uses FatFs R0.15 with all mutation APIs compiled
out. It mounts only long enough to enumerate at most eight short-name root
entries, never opens file content, never auto-runs SD content, and unmounts
before entering the interactive loop. The physical CPI 2.0 unit successfully
listed the existing FAT card's root entries while keyboard input remained
responsive.

The accelerated terminal baseline also passed interactive hardware use. Its
fixed 40 x 20 cell buffer now scrolls upward at the bottom row and moves a whole
word to the next row when it fits, retaining hard wrapping only for tokens
longer than the terminal width.

The V2.0 schematic powers SD VDD from the PMIC-controlled ALDO1 rail rather
than the Pico's 3V3 output. ClockworkPi's pinned SD booter waits 1.5 seconds
after active-low card detection before starting SPI; PicoPen mirrors that
bounded stabilization interval. A detected card with repeated `R1=0xFF` should
therefore be correlated with keyboard-controller availability before treating
it as an SPI or card fault, because the controller initializes the board PMIC.

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
