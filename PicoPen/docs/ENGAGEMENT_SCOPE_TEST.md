# Engagement Scope Fast-Track Test

This test covers the volatile engagement-profile editor and visible scope
state. Activating a scope does not grant GPIO drive, radio transmission, target
power, USB HID, remote control, or any other capability. Profiles intentionally
disappear on reboot until authenticated internal settings are designed.

## Flash

Use the verified PicoPen bootloader and flash
`build/pico2w-debug/os/picopen_os_slot.uf2` with the established OS-slot
procedure. No SD card content or attachments are required.

## Acceptance checks

1. Boot to Home. Confirm the header badge says `SCOPE OFF`.
2. Open **System > Security**. Confirm the scope is `INACTIVE` and marked
   `SESSION ONLY`.
3. With `REF` selected, type a reference containing at least three letters,
   digits, `-`, `_`, or `.`. Backspace should edit it.
4. Move to Duration and use Left or Right. Confirm the bounded choices are 15,
   60, and 240 minutes.
5. Select `ACTIVATE SCOPE` and press Enter. Confirm the page shows `ACTIVE`,
   the reference, and remaining minutes.
6. Return Home. Confirm the persistent badge now says `SCOPE ON`. Open another
   application and confirm its header also says `SCOPE:ON`.
7. Return to Security and select `END SCOPE`. Confirm the state and Home badge
   immediately return to off.
8. Try activating with an empty or one-character reference. Confirm activation
   is denied and Audit records the denied `scope.start` action.
9. Activate a valid scope and reboot. Confirm it returns inactive; development
   firmware must not silently persist authorization state.

The normal test does not need to wait for expiry. The service also clears a
profile automatically at its monotonic deadline and synchronizes the separate
capability context without adding any grants.
