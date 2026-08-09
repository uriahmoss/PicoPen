# Synthwave Home compositor test

Flash `build/pico2w-debug/os/picopen_os_slot.uf2` without replacing the working
bootloader.

1. Confirm Home fills the 320 x 320 screen with a dark navy background.
2. Confirm the header shows magenta `PICOPEN`, gold `SD RO`, and cyan
   `SCOPE OFF` with `LOCKED` beneath it.
3. Confirm six panels appear in two columns: Status, Files, Devices, Workbench,
   Audit, and System.
4. Confirm each panel has a small geometric icon and legible two-times-scale
   label without clipping or overlap.
5. Use all four arrow keys. The selected panel must receive a thicker bright
   cyan border and navigation must remain inside the six-item grid.
6. Confirm the footer remains visible and reads `ARROWS MOVE`, `ENTER SELECT`,
   and `ESC BACK`.
7. Open every panel. Existing interior screens and their Escape behavior must
   remain unchanged.
8. Return Home repeatedly and confirm the display fully redraws without stale
   text, partial panels, incorrect colors, or keyboard loss.

This slice changes only drawing. Service capabilities, storage policy, update
policy, and shutdown authorization remain unchanged.
