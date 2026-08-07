# PicoPen Boot Contract v0.1

Status: accepted for Milestone 1 implementation. Changes to this document,
`include/picopen/boot_format.h`, or the linker layouts must be reviewed together.

## Assumptions

- Target: Raspberry Pi Pico 2 W (`pico2_w`, RP2350 ARM secure platform).
- XIP flash base: `0x10000000`.
- Flash capacity: 4 MiB, as defined by Pico SDK 2.2.0's `pico2_w` board file.
- Minimum erase unit: 4 KiB. No metadata record crosses an erase boundary.
- RP2350 ROM BOOTSEL remains the final recovery path.

## Flash layout

Offsets below are relative to the XIP flash base.

| Region | Offset | Size | Purpose |
|---|---:|---:|---|
| Stage-1 bootloader | `0x000000` | 256 KiB | Validation, selection, recovery |
| Boot metadata A | `0x040000` | 4 KiB | Redundant boot state copy A |
| Boot metadata B | `0x041000` | 4 KiB | Redundant boot state copy B |
| Reserved | `0x042000` | 56 KiB | Future boot metadata growth |
| Primary image slot | `0x050000` | 1792 KiB | Last confirmed-good OS |
| Candidate image slot | `0x210000` | 1792 KiB | Update candidate or recovery image |
| Persistent boot data | `0x3D0000` | 192 KiB | Bounded crash/update journal |

Each image slot reserves its first 4 KiB erase sector for the manifest. The
executable payload begins at slot offset `0x1000`, leaving a maximum payload of
`0x1BF000` bytes. Slot names describe policy, not hard-coded boot priority.

## Image header

The first 256 bytes of a slot contain `picopen_image_header_v1_t`. Remaining
bytes in the 4 KiB manifest sector must be `0xFF`. Multibyte fields use
little-endian encoding. The structure is packed to match disk bytes; parsers
must copy multibyte fields into aligned local values rather than dereference
potentially unaligned members.

Required fields include:

- eight-byte magic `PICOPEN\0`, format version, and header size
- target identifier and bounded payload size
- payload-relative entry and vector offsets
- semantic version, monotonic build number, and minimum bootloader version
- SHA-256 payload digest
- signature algorithm, key identifier, and fixed 64-byte signature field
- build-provenance digest
- CRC-32 over header bytes 0 through 251, with the CRC field excluded

The CRC detects malformed manifests; it is not an authenticity control. During
development, signature algorithm `none` is permitted only under the development
policy defined below. Production signing policy is intentionally deferred until
the signing approach is selected.

## Validation order

The bootloader must reject an image at the first failed check:

1. Slot and manifest addresses lie wholly inside the declared flash layout.
2. Magic, format version, header size, reserved bytes, and header CRC are valid.
3. Target is `PICOPEN_TARGET_PICO2_W` and flags contain no unknown required bit.
4. Payload size is nonzero and no arithmetic operation wraps.
5. Payload, vector, and entry ranges lie wholly inside the selected slot.
6. Minimum bootloader version is supported.
7. SHA-256 of exactly `image_size` payload bytes matches the manifest.
8. The signature satisfies the active development or production policy.

Validation never follows addresses supplied by an invalid image and never reads
beyond its selected slot.

## Development policy

Milestone 1 permits unsigned locally built images so the boot chain can be
brought up. Such builds must identify themselves as development builds in the
console and later UI. Enabling unsigned images in a production build will
require a physical-presence mechanism; that mechanism is not yet selected.

## Transfer contract

Before entering a validated OS, the bootloader will eventually:

- disable bootloader-owned interrupts and DMA channels
- leave external PicoCalc peripherals and transmitters inactive
- pass a small, versioned handoff record containing reset and selected-slot data
- set the vector table and stack according to the validated image
- branch only to the validated entry offset

Exact RP2350 handoff mechanics belong to Slice 1D and are not implemented by
this specification slice.

## Failure and recovery contract

- No valid image: stay in a bounded USB recovery console and advertise the
  validation reason without dumping secrets.
- Explicit recovery request: do not attempt the OS image.
- Repeated unconfirmed candidate boots: return to the last confirmed-good slot.
- Corrupt metadata: select a valid redundant copy; if neither is valid, recover.
- ROM BOOTSEL remains reachable even if the PicoPen bootloader is damaged.

Boot metadata records, attempt limits, and the boot-success handshake are
defined in Slice 1E.

## Slice 1A acceptance record

- The flash regions are erase-aligned, non-overlapping, and total exactly 4 MiB.
- Both OS slots are equal-sized and keep manifests separate from payload data.
- The v1 header has a compile-time-enforced size of exactly 256 bytes.
- Validation and recovery behavior are specified before parser implementation.
- Existing bring-up firmware behavior remains unchanged.
