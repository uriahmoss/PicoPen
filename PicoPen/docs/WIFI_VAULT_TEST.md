# Wi-Fi credential vault hardware test

Flash `build/pico2w-debug/os/picopen_os_slot.uf2` after the existing bootloader.

1. Open **System > WiFi Update**. Select **Store Initialize** once. Confirm its
   label becomes `STORE READY` and the OS remains responsive.
2. Enter an SSID, password, and a 4-16 character PIN. Select **Remember**.
   Confirm `VAULT:OK`; neither password nor PIN may appear on screen or serial.
3. Power-cycle. Return to WiFi Update. Confirm the SSID/password are not loaded.
4. Enter a wrong PIN and select **Load Saved**. Confirm it fails without filling
   either credential. After five failures confirm `RETRY` counts down from about
   30 seconds and correct-PIN attempts remain blocked during that interval.
5. After expiry, enter the correct PIN and load. Confirm the SSID appears and the
   password remains masked. Explicitly enable Wi-Fi, then select Connect.
6. Disconnect, select **Forget Saved**, and power-cycle. Confirm `Remember`
   replaces `Load Saved` and the old PIN cannot recover credentials.

Do not intentionally remove power during a flash erase/program operation in
this first test. Interrupted-update and full-media fault tests remain pending.
