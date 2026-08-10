# Wi-Fi Connection Bundle Test

This combined test covers four accelerated slices: bounded passive inventory,
volatile credential entry, locally confirmed association/disconnection, and
read-only link diagnostics. It still has no IP stack, sockets, listeners,
portal, file transfer, or update installation.

## Test

1. Flash `build/pico2w-debug/os/picopen_os_slot.uf2` normally.
2. Open **System > WiFi Update**, select Power, and enable it. Confirm `READY`.
3. Select Passive Scan and press Enter. Confirm `SCANNING`, then `READY`, with
   at most six APs showing SSID, RSSI, and channel. The scan is passive.
4. Select SSID and type the exact network name. Select Pass and type its WPA2
   password. Confirm only `*` characters appear for the password.
5. Select Connect and press Enter. Confirm `CONNECTING` and, within 15 seconds,
   either `CONNECTED` or a bounded `ERROR` with link/driver codes.
6. On success, select Disconnect and confirm it returns to `READY`.
7. Disable Power and confirm `OFF`. Re-enable it and confirm the password field
   is empty; reboot and confirm all Wi-Fi state and credentials are gone.
8. Check Audit for scan, connect, disconnect, enable, and disable lifecycle
   results. No credential text may appear anywhere in Audit or USB output.

This image joins only the named WPA2 network. It cannot obtain an IP address or
transfer data yet; those require the separately configured and reviewed lwIP
boundary in the next networking bundle.
