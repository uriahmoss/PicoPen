# PicoPen

PicoPen is an instant-on, purpose-built operating system for authorized hardware
and network security work on the ClockworkPi PicoCalc with a Raspberry Pi Pico
2 W core.

The project is not a Linux distribution. It is a small embedded system built on
the Raspberry Pi Pico SDK, with a recoverable bootloader, native PicoCalc
drivers, a terminal-oriented interface, and a permission-controlled attachment
framework.

## Project status

PicoPen is in the architecture and hardware-bring-up phase. No release image is
available yet.

The first bootable milestone will:

1. Start safely on a Pico 2 W.
2. Preserve the RP2350 ROM BOOTSEL recovery path.
3. Initialize logging and report the reset reason.
4. Validate an OS image before launching it.
5. Fall back to recovery after repeated failed boots.
6. Bring up the PicoCalc display, keyboard, SD card, and battery telemetry.

## Design goals

- Sub-second path to an interactive local interface after hardware validation.
- Recovery from interrupted updates and invalid firmware.
- A small, auditable trusted core with drivers outside privileged code where
  practical.
- Explicit capability grants for USB, GPIO, radio transmit, credentials, and
  scoped network access.
- Useful offline operation with documentation and tools stored on SD.
- Modular SPI, I2C, UART, 1-Wire, infrared, NFC, and regional sub-GHz
  attachments.
- Reproducible builds and signed release artifacts.

## Non-goals

- General-purpose desktop computing.
- Covert persistence, credential theft, destructive payloads, or evasion.
- Transmitting outside configured regional limits.
- Interacting with systems or credentials outside an explicitly authorized
  engagement scope.
- Claiming hardware capabilities that are not physically installed.

## Repository layout

```text
PicoPen/
|- bootloader/       Boot policy, image validation, update, and recovery
|- kernel/           Scheduler, memory, IPC, capabilities, and system startup
|- drivers/          PicoCalc and attachment drivers
|- services/         Storage, UI, networking, audit, and device management
|- apps/             Built-in user-facing tools
|- include/picopen/  Public interfaces shared between components
|- boards/           Board descriptions and pin assignments
|- docs/             Architecture and product specifications
|- tests/            Host-side and hardware-in-the-loop tests
`- tools/            Image packaging, signing, and development utilities
```

## Documentation

- [Architecture](docs/ARCHITECTURE.md)
- [Bootloader design](docs/BOOTLOADER.md)
- [Feature list](docs/FEATURES.md)
- [Fresh-board setup and first flash](docs/GETTING_STARTED.md)
- [Hardware plan](docs/HARDWARE.md)
- [Roadmap](docs/ROADMAP.md)
- [Safety and threat model](docs/THREAT_MODEL.md)
- [Contributing](CONTRIBUTING.md)

## Building

The first target is a deliberately limited bring-up image. It verifies the
toolchain, UF2 flashing, USB logging, reset reporting, and the Pico 2 W onboard
LED without driving unverified PicoCalc peripheral pins. See the
[fresh-board setup guide](docs/GETTING_STARTED.md).

## License

No license has been selected yet. Until one is added, all rights are reserved.
Choose the project license before accepting external contributions or
publishing reusable releases.
