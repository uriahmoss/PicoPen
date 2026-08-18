# Host workflow fast-track test

This test covers the bounded onboard host inventory, cross-app host launcher,
and reusable result details. It performs no broad subnet scan.

1. Connect PicoPen to a known test Wi-Fi network.
2. Open Apps > Network Discovery. Confirm PicoPen lists its local address,
   gateway, and DNS address without sending discovery probes.
3. Select a listed host. Confirm Host Inspector, HTTP Inspector, SSH Banner,
   and TLS Inspector actions are offered.
4. Choose an action. Confirm its target is pre-filled, its expected default
   port is selected, and the normal local confirmation remains present.
5. Run the task and wait for a terminal result. Escape and confirm PicoPen
   returns to the same host action and host selection.
6. Open Apps > Recent Results, select the result, and verify the detailed state,
   target, address, service, duration, received-byte count, detail, and result.
7. Press Enter on the detail and confirm the corresponding app is populated
   with that result's address and port.
8. Reboot and confirm the volatile host/result inventories are cleared.

Acceptance requires bounded fixed-capacity records, sanitized result text,
unchanged task authorization, and no implicit network-wide operation.

Hardware result: passed. Host selection, inspector handoff, result details,
result reuse, and restoration of the selected host/action on Escape behaved as
expected on the PicoCalc.
