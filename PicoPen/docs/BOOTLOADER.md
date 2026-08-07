# Bootloader Design

## Objectives

- Reach a validated OS image with minimal delay.
- Always preserve ROM BOOTSEL recovery.
- Detect corrupt, incomplete, and incompatible images.
- Recover automatically from repeated early-boot failures.
- Support reproducible, signed production releases without making development
  builds painful.

## Flash layout and boot contract

Milestone 1 now uses the versioned layout, image header, validation order, and
recovery rules in [BOOT_CONTRACT.md](BOOT_CONTRACT.md). The constants and header
structure are mirrored in `include/picopen/boot_format.h` and will be consumed by
the bootloader, OS packager, and host-side validator.

```text
flash start
|- RP2350/Pico SDK boot support
|- PicoPen stage-1 bootloader
|- redundant boot metadata pages A/B
|- primary OS image
|- recovery or candidate image area
`- persistent crash and update journal
flash end
```

The design must not reserve offsets in source code until the linker layout is
reviewed and tested on the actual Pico 2 W flash device.

## Image manifest

Each image will carry:

- magic and manifest format version
- target board identifier
- semantic version and monotonically increasing build number
- image length and entry point
- minimum compatible bootloader version
- cryptographic content digest
- optional production signature and key identifier
- build provenance identifier

Development mode accepts locally built unsigned images only when a physical
presence condition is satisfied. Production mode accepts signed images.

## Boot flow

1. Initialize only the clocks and state required for validation.
2. Read both metadata copies and select the newest valid record.
3. Check for a physical recovery request.
4. Validate the selected image bounds, target, digest, and policy.
5. Mark a pending image attempt without falsely marking it successful.
6. Transfer control with a documented clean machine state.
7. Require the OS to confirm successful boot after core services are healthy.
8. Roll back or enter recovery after the configured attempt limit.

## Reset and failure handling

- Watchdog and fault resets increment the pending image failure counter.
- Power loss during metadata updates must leave at least one valid metadata copy.
- Update journals use write-new-then-commit semantics.
- Recovery reports failures over serial before display support is assumed.
- Crash records are bounded to prevent flash wear.

## Update sources

Planned order:

1. USB development flashing through the existing RP2350 recovery mechanisms.
2. Validated update package from SD.
3. Optional network download to a staging area.

Network-delivered data is never executed directly. It passes through the same
manifest and validation path as an SD update.

## Open decisions

- Signature scheme and key-storage policy.
- Recovery input combination available before the keyboard controller is ready.
- Exact PicoCalc USB-C routing and whether recovery should rely on the Pico 2 W
  micro-USB connector during early development.
