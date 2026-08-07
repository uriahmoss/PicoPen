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

For development artifacts, build provenance is the SHA-256 digest of the
canonical flashable payload. ELF files are deliberately excluded because debug
metadata can contain an absolute build-directory path. Production source and
builder attestations remain part of the later signing-policy work.

## Transfer contract

Before entering a validated OS, the bootloader will eventually:

- disable bootloader-owned interrupts and DMA channels
- leave external PicoCalc peripherals and transmitters inactive
- pass a small, versioned handoff record containing reset and selected-slot data
- set the vector table and stack according to the validated image
- branch only to the validated entry offset

RP2350 `rom_chain_image()` was evaluated first, but physical tests showed it did
not launch the custom partitionless slot. Slice 1D therefore uses a direct
secure Cortex-M33 vector handoff after PicoPen validation. The bootloader checks
the initial stack alignment and SRAM range plus the Thumb reset vector and its
payload bounds. It then disables and clears NVIC interrupts, stops SysTick, sets
VTOR, sets MSP/MSPLIM, and branches to the validated reset handler. A watchdog
returns to recovery if the OS does not confirm startup.

## Build artifacts

Slice 1B produces two independently linked targets:

- `picopen_bootloader`, a ROM-bootable recovery skeleton constrained to
  `0x10000000..0x1003FFFF`
- `picopen_os`, a relocated slot payload beginning at `0x10051000`

The OS UF2 is an address-bearing development payload, not a standalone ROM
image. It is not safe to flash by itself. Slice 1C adds the manifest packaging
and validation path; Slice 1D adds controlled transfer from bootloader to OS.

## Failure and recovery contract

- No valid image: stay in a bounded USB recovery console and advertise the
  validation reason without dumping secrets.
- Explicit recovery request: do not attempt the OS image.
- Repeated unconfirmed candidate boots: return to the last confirmed-good slot.
- Corrupt metadata: select a valid redundant copy; if neither is valid, recover.
- ROM BOOTSEL remains reachable even if the PicoPen bootloader is damaged.

## Boot metadata and success handshake

Slice 1E uses a 256-byte version-1 record at the start of each redundant boot
metadata sector. A record contains its identity and size, generation, selected
slot, pending or confirmed state, consecutive attempt count, last failure
checkpoint, reserved bytes, and CRC-32. Reserved bytes must be zero. The newest
CRC-valid generation is authoritative; if an erase or program operation is
interrupted, the older valid copy remains selectable. Generation comparison
supports 32-bit wraparound.

Before every transfer, stage 1 writes a pending record to the alternate sector
and increments the consecutive attempt count. The minimal OS writes a confirmed
record and resets the count only after its core runtime and USB console are
ready. A missing confirmation causes another validated attempt. Three
consecutive unconfirmed attempts enter recovery, where the counter and watchdog
checkpoint are reported. A metadata write or verification failure also enters
recovery and never transfers control.
The local recovery-console `retry` command writes a confirmed zero-attempt
record and reboots. It resets only the lockout; normal image validation still
runs before any subsequent transfer.

## Slice 1A acceptance record

- The flash regions are erase-aligned, non-overlapping, and total exactly 4 MiB.
- Both OS slots are equal-sized and keep manifests separate from payload data.
- The v1 header has a compile-time-enforced size of exactly 256 bytes.
- Validation and recovery behavior are specified before parser implementation.
- The original bring-up behavior remained unchanged for Slice 1A; later
  recovery hardening removed CYW43 so the image fits entirely in stage-1 flash.
