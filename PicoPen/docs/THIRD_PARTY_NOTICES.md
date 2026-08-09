# Third-Party Components

This file records code incorporated into PicoPen. Reference-only firmware is
listed separately and is not a PicoPen dependency.

## Incorporated

### Raspberry Pi Pico SDK 2.2.0

- Source: Raspberry Pi Ltd.
- License: BSD 3-Clause and component-specific notices included upstream
- Purpose: RP2350 platform, hardware, boot ROM, TinyUSB, CYW43, and networking
  integration
- Pin: recorded in `tools/dependencies.lock`

## Approved for future integration

- FatFs R0.15: read-only removable FAT support; FatFs permissive license
- littlefs: internal settings and audit journal; BSD 3-Clause
- LVGL: graphical interface toolkit; MIT

Each component moves to Incorporated only in the slice that vendors or links
it, with its exact source pin and retained license text.

## Reference only — no code copied

- ClockworkPi PicoCalc firmware: hardware behavior and schematic cross-checks
- PicoMiteAllVersions: PicoCalc peripheral behavior
- Picoware: application and device behavior; GPLv3 implementation excluded
- PicoCalc MicroPython firmware collections: compatibility and timing behavior
