# Standalone boot and peripheral recovery test

This test intentionally varies power and USB ordering. Keep the working
PicoPen bootloader installed and flash only
`build/pico2w-debug/os/picopen_os_slot.uf2`.

## Required cases

1. Disconnect Pico USB, install the batteries, and power on normally. Confirm
   the GUI reaches Home without a PC or USB serial connection.
2. From a complete power-off, connect Pico USB and then power on. Confirm the
   GUI and devices initialize without requiring battery removal.
3. Boot on batteries, wait for Home, and then connect Pico USB. Confirm a COM
   port appears without resetting the GUI or losing keyboard/SD state.
4. Remove and reconnect Pico USB while the PicoCalc remains powered. Confirm
   local operation continues throughout.
5. Perform a warm reset through the established boot path. Confirm peripherals
   return without a full power removal.
6. Repeat a cold boot with the SD card installed. The boot-progress screen may
   show `WAITING FOR CPI 2.0` for at most five seconds before degraded Home.
7. If the keyboard controller is initially unavailable, leave the system
   powered. Confirm the five-second runtime health cycle can recover keyboard,
   battery, SD, filesystem, and Devices-screen state without rebooting.
8. Confirm BOOTSEL recovery remains reachable and the boot-attempt count stays
   confirmed after a battery-only successful boot.

Report the exact case and visible status if any ordering still requires removal
of both USB and batteries.
