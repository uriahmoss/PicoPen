# Built-in skins and menu-memory test

Flash only `build/pico2w-debug/os/picopen_os_slot.uf2` through the working
PicoPen bootloader.

1. Cold boot and confirm Synthwave remains the factory default.
2. Move to System, open it, move to Skins, and press Enter. Confirm the list
   contains Synthwave, Crayon, High Contrast, and Minimal Dark.
3. Select Crayon and press Enter. Confirm Home uses a warm paper background,
   dark lettering, six distinct crayon-like accent colors, and a turquoise
   scribble behind the selected tile. Labels must remain readable.
4. Return to System. Confirm System remains selected on Home and Skins remains
   selected within System rather than resetting to the first entries.
5. Apply High Contrast and Minimal Dark in turn. Confirm every Home label,
   status item, border, and focus state remains visible.
6. Reapply Synthwave and confirm its original palette and unique lime focus
   state return.
7. Power-cycle. Confirm Synthwave returns because persistent settings are not
   enabled yet. The menu must describe selections as session defaults.
8. Repeatedly switch skins and navigate Home. Confirm only the old and new
   focus tiles redraw and keyboard input remains responsive.

Imported skins, arbitrary assets, scripts, and persistent flash writes are not
part of this slice.
