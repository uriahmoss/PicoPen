# Network, persistence, and recovery fast-track test

Flash `build/pico2w-debug/os/picopen_os_slot.uf2` after the existing stage-1
bootloader. This image contains no TCP stack, sockets, listeners, portal,
probing tool, or update installer.

## Wi-Fi and scan selection

1. Open **System > WiFi Update**, enable Power, and run Passive Scan.
2. Focus SSID and press Left/Right. Confirm the bounded scan results cycle and
   the displayed SSID, RSSI, and channel change together.
3. Enter the password or unlock a saved credential, then Connect.
4. Confirm state reaches `CONNECTED`, DHCP becomes `BOUND`, and nonzero IP,
   gateway, and usually DNS values appear. Confirm RSSI updates.
5. Disconnect and confirm address fields clear. Disable Power and confirm the
   interface returns to `OFF`.

## Persistent preferences

1. Select a non-default skin, navigate to different Home/System entries, and
   power-cycle normally.
2. Confirm the selected skin and committed menu positions return after boot.
3. Confirm a missing settings record falls back to Synthwave and bounded first
   entries without preventing boot.

## SD lifecycle

1. Open Files and press `S`. Confirm Files becomes unavailable and the SD/FAT
   device state no longer reports ready.
2. Remove/reinsert the card if desired, then return to Files and press `R`.
   Confirm the media generation changes and the read-only listing returns.
3. Confirm no SD write or formatting option exists.

## Recovery health

1. Open **System > Recovery**. Confirm internal storage is `READY` and allocated
   blocks remain within the displayed total.
2. Confirm either no crash record or a bounded count and reason are shown.
3. If a record exists, select Clear Crash Record and confirm it disappears.

Do not deliberately interrupt a flash erase/program operation in this test.
Power-loss injection remains a fixture-level acceptance item.
