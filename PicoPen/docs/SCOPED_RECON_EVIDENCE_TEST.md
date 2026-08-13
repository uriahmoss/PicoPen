# Scoped recon and evidence hardware test

Use only a lab host or network you own or are explicitly authorized to test.
This build issues one bounded request; it does not scan address or port ranges.

## Network test

1. Flash `picopen_os.uf2`, boot PicoPen, and connect it to the authorized lab
   Wi-Fi from **System > WiFi Update**.
2. Open **System > Security**. Enter a reference, the exact lab host IPv4/CIDR
   or hostname, choose its permitted port, choose a duration, and activate it.
3. Open **Workbench** and run **DNS Lookup**, **ICMP Echo**, then **TCP
   Identify**. Each operation starts only after the local Enter key press.
4. Confirm that each result screen shows the target, resolved address, state,
   elapsed time, and result. Esc cancels a running request.
5. Deactivate or let the scope expire during a request. The request must end in
   `DENIED`. A target or port outside the active scope must not run.

Expected limits: one active request, at least one second between starts, and a
five-second deadline. No listener, payload, credential, exploit, or range scan
is present.

## Evidence test

1. Put a file no larger than 256 KiB on the FAT SD card. A small `.pcap` or
   `.pcapng` file exercises capture inventory; any file exercises hashing.
2. Select the file in **Files**, return Home, then choose **Workbench > Evidence
   File**.
3. Confirm SHA-256 completion, printable-string count, capture format, and
   packet count. Esc cancels processing.

Evidence access remains read-only. Malformed or oversized records must produce
an error without modifying the SD card.
