# Versioned Storage Fast-Track Hardware Test

This test covers the first read-only portion of Slice 3C. It does not enable
SD writes, radios, GPIO output, attachment power, or active probing.

Status: deferred. The operator currently has no means to place the required
test files on the SD card. Build and host-test acceptance is provisional until
this procedure can be performed, potentially after authenticated Wi-Fi file
transfer is available.

## Flash

Use the already verified PicoPen bootloader and copy
`build/pico2w-debug/os/picopen_os_slot.uf2` to the RP2350 BOOTSEL volume using
the established OS-slot flashing procedure.

## Test card

Use a FAT-formatted card containing:

- a short text file in the root;
- a binary file in the root;
- a directory containing another text file; and
- optionally, more than 12 entries in one directory to exercise the bound.

Do not use the only copy of important evidence. PicoPen mounts and reads the
card without exposing a write API, but this remains development firmware.

## Acceptance checks

1. Boot to Home and open **Files**. Confirm the current path and `READ-ONLY`
   are visible.
2. Select a directory and press Enter. Confirm its bounded listing opens.
3. Press Escape. Confirm Files returns to the parent directory before leaving
   the Files application.
4. Open the text file. Confirm its size is shown and the view is labelled
   `TEXT`.
5. Open the binary file. Confirm its size is shown and the view is labelled
   `HEX (FIRST 96 BYTES)` with hexadecimal rows rather than raw control bytes.
6. If a directory has more than 12 entries, confirm the UI remains responsive
   and does not render beyond its fixed listing capacity.
7. Confirm keyboard navigation, Status, and Escape continue working after all
   file operations.

The next portion will add a visible safe-remove control and bounded passive
inventory jobs. Evidence export and all mutation remain locked pending review
of their capability and confirmation contracts.
