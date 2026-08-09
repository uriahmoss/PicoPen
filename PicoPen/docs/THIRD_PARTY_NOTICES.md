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

### FatFs R0.15

- Author: ChaN
- License: FatFs permissive license retained in the vendored source headers
- Source snapshot: `oyama/pico-vfs` commit
  `4b71f274acae7de9a3696d3345992294fa9e034e`
- Purpose: read-only FAT volume and directory parsing
- PicoPen configuration: writes, mkfs, chmod, labels, expansion, exFAT, long
  filenames, and dynamic LFN allocation disabled

## Approved for future integration

- littlefs: internal settings and audit journal; BSD 3-Clause
- LVGL: graphical interface toolkit; MIT

Each component moves to Incorporated only in the slice that vendors or links
it, with its exact source pin and retained license text.

## Reference only — no code copied

- ClockworkPi PicoCalc firmware: hardware behavior and schematic cross-checks
- PicoMiteAllVersions: PicoCalc peripheral behavior
- Picoware: application and device behavior; GPLv3 implementation excluded
- PicoCalc MicroPython firmware collections: compatibility and timing behavior
