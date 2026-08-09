# Keyboard-driven GUI baseline test

## Artifact

Flash `build/pico2w-debug/os/picopen_os_slot.uf2` through the existing PicoPen
bootloader recovery path. Keep the working bootloader installed.

## Navigation acceptance

1. Confirm the `PICOPEN HOME` screen replaces the command prompt after boot.
2. Use Left, Right, Up, and Down to select all six tiles. Confirm the `>` focus
   marker follows the expected two-column layout without leaving the grid.
3. Open Status, Devices, Workbench, and Audit with Enter. Confirm Escape returns
   directly to Home.
4. Open Files. Use Up and Down to select an existing root entry. Enter must
   either display at most 256 sanitized bytes or clearly deny directories and
   unavailable files. Escape returns first to Files and then Home.
5. Open System and inspect Security. Confirm scope is inactive, SD is
   read-only, and active capabilities are denied.
6. Open Wi-Fi Update. Confirm it reports `NOT CONFIGURED`, Wi-Fi disabled, and
   unsigned updates denied. It must not attempt to associate with a network.
7. Open Terminal. Confirm normal commands remain available and Escape returns
   to System without requiring a typed command.
8. Open Power. Leave the confirmation screen open for at least one minute;
   confirm it does not expire or power off. Select Cancel and confirm the system
   remains on.
9. Reopen Power, select Power Off, and press Enter once. Confirm the controller
   reports acceptance and the PicoCalc powers off about six seconds later.
10. Power on again and confirm the GUI, keyboard, and read-only SD return to
    their previous healthy state.

The initial GUI is intentionally rendered by PicoPen's lightweight terminal
renderer. It establishes navigation and safety behavior before introducing a
larger graphical toolkit or framebuffer dependency.
