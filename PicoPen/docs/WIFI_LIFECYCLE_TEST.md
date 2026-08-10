# Secure Wi-Fi Lifecycle Fast-Track Test

This is the first Bundle C hardware test. It initializes the pinned CYW43
driver only after a local keypress and can return it fully off. It does not
scan, collect credentials, associate, obtain an IP address, open sockets, or
start an update/file-transfer portal.

## Flash

Flash `build/pico2w-debug/os/picopen_os_slot.uf2` using the established PicoPen
OS-slot procedure. No access point, password, SD content, or attachment is
required.

## Acceptance checks

1. Boot normally and open **Devices**. Confirm Wi-Fi starts `DISABLED`.
2. Open **System > WiFi Update**. Confirm the interface reports `OFF`, zero
   credentials/association, no scan/listeners, and locked update installation.
3. Press Enter once. The first driver initialization may take a moment. Confirm
   it reaches `READY-NO-NET` with driver result `0`.
4. Return to Devices. Confirm Wi-Fi reports `READY-LOCAL`, not connected or
   generally ready.
5. Return to WiFi Update and press Enter. Confirm it returns to `OFF`.
6. Repeat enable and disable twice. Confirm the transition count increases and
   keyboard/menu response returns after each transition.
7. Open Audit after enabling or disabling. Confirm the latest lifecycle action
   is `wifi.enable` or `wifi.disable` with the expected result.
8. Reboot after leaving Wi-Fi enabled. Confirm it starts `OFF` again; radio
   state is never silently persisted.

If driver initialization reports `ERROR`, record its numeric driver result and
do not repeatedly retry. Press Enter once to return the service to `OFF`.
