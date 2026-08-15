# Attachment platform foundation test

This slice validates attachment architecture without initializing, powering,
probing, or transmitting on any external connector. Leave all external modules
disconnected for this test.

1. Flash the combined OS-slot image and open Home > Devices.
2. Confirm the normal built-in device inventory remains present.
3. Under Attachments, confirm `PN532 TEST PROVIDER`, transport `MOCK`, state
   `DISABLED`, and marker `TEST` are visible.
4. Open Apps and confirm its built-in applications remain available. An app
   requiring a provider that is absent or disabled must display `MISSING`.
5. Confirm keyboard, display, SD, Wi-Fi, and the previously tested network Apps
   continue to operate normally.

Acceptance requires no external GPIO, I2C, SPI, UART, USB-host, PIO, target
power, NFC, RF, or IR activity. The mock descriptor exercises only bounded
registry, provider matching, lifecycle state, and UI paths.

Hardware result: passed on the PicoCalc. The mock PN532 provider and expected
disabled/test status were displayed, with no regression observed in the OS.
