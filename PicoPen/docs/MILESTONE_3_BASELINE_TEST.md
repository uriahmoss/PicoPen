# Milestone 3 accelerated baseline hardware test

This image advances only baseline services that do not depend on unresolved
PSRAM wiring, writable internal storage, LVGL, or enabled radios. All removable
storage remains read-only and every active hardware capability remains denied.

## Artifact

Flash `build/pico2w-debug/os/picopen_os_slot.uf2` after entering the existing
PicoPen bootloader's BOOTSEL recovery path. Do not replace the bootloader.

## Acceptance sequence

1. Confirm the normal terminal appears and the keyboard, SD, filesystem, and
   battery lines remain healthy.
2. Run `help`. Confirm `CAT`, `SCOPE`, `WORKBENCH`, and `SHUTDOWN` are present.
3. Run `devices`. Confirm `IPC V1 READY`, `SD READY-RO`, `FATFS READY-RO`,
   `PSRAM UNVERIFIED`, and Wi-Fi/BLE/attachments `DISABLED`.
4. Run `scope`. Confirm the engagement is inactive and active operations are
   denied.
5. Run `workbench`. Confirm the three board buses are claimed as documented,
   while GPIO drive and transmission are denied.
6. Run `security`. Confirm transmit, GPIO output, USB HID, and remote control
   are denied and storage is read-only.
7. Run `ls`. If the card contains a root-level 8.3 text file, run
   `cat NAME.TXT`. Confirm at most 256 bytes appear and longer content reports
   truncation. Subdirectories and paths are intentionally rejected.
8. Run `audit` after a command and confirm the bounded in-memory audit summary
   updates without exposing command contents.
9. Run `shutdown`, followed by `shutdown cancel`, and confirm power remains on.
10. Optional final test: run `shutdown`, then `shutdown confirm` within 15
    seconds. Confirm the PicoCalc powers off approximately six seconds later.

After a power-on, repeat `status`, `devices`, and `ls` to confirm the controlled
shutdown did not affect boot metadata or the read-only SD card.
