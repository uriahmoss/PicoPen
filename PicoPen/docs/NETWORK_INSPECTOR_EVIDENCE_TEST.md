# Network inspector and evidence suite test

Use only a lab host or network you own or are explicitly authorized to test.

## Navigation and confirmation

1. Open Workbench and move through all nine rows with single arrow presses.
2. Hold an arrow and confirm movement repeats at a controlled pace. Enter and
   Escape must never repeat while held.
3. Leave and reopen Workbench, Security, and Wi-Fi. Each should remember its
   most recent row for the current boot.
4. Select a network operation. Verify the confirmation page shows operation,
   exact target, port, scope reference, and active state. Escape must cancel
   without network activity.

## Authorized network inspection

1. Connect Wi-Fi and activate a scope for one owned lab host and permitted port.
2. DNS and ICMP should retain their previous behavior.
3. TCP Identify should report the common service hint and open/refused/timeout.
4. On an owned HTTP server, select HTTP HEAD with port 80 scope. Confirm once;
   the result should show a bounded response header preview.
5. On an owned SSH server, select SSH Banner with port 22 scope. Confirm once;
   the result should show only the server banner. No login is attempted.
6. TLS Metadata currently must return `UNAVAILABLE` with the RAM-review message.
   It must not claim a successful TLS handshake.
7. Open Recent Results and confirm up to six sanitized results are shown. They
   disappear after reboot.
8. Cancel an active operation and expire a scope during another. Both must stop.

## Read-only evidence

1. Open Workbench > Evidence File and select a file directly.
2. Confirm SHA-256, strings count, bounded strings preview, and completion.
3. For an Ethernet PCAP/PCAPNG, confirm packet and IPv4/IPv6/ARP/TCP/UDP/ICMP
   counts. Unsupported link types may show packet totals but zero protocol totals.
4. Confirm malformed/truncated input reports `ERROR`, Escape cancels work, and
   the SD card remains unchanged.
