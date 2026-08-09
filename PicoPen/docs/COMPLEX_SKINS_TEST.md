# Complex skin renderer hardware test

Flash only `build/pico2w-debug/os/picopen_os_slot.uf2` through the working
bootloader.

For every built-in skin, inspect Home, System, Skins, Security, Wi-Fi Update,
Files, Status, Devices, Workbench, Audit, About, and the Power dialog.

## Synthwave

- Confirm neon text has a visible colored edge/glow and remains sharp.
- Confirm the background grid, layered tile borders, magenta/cyan accents, and
  unique lime focus state are visible without making navigation slower.

## Crayon

- Confirm Home is a vertical security-sketchbook page with six loose menu
  entries; it must not use Synthwave's two-column tile grid.
- Confirm the background resembles warm textured paper rather than flat white.
- Confirm labels use rounded handwritten strokes from the packed Kalam glyphs,
  not the blocky 5x7 pixel lettering used by the other compact skins.
- Confirm lettering has irregular wax-like edges and small color variation.
- Confirm tiles use different crayon colors.
- Confirm the selected tile and selected submenu row use turquoise scribble
  strokes behind text and icons while every label remains readable.
- Confirm Home selection is a dense wax scribble behind the selected artwork,
  not a rectangular outline or moving box; the original icon and label must be
  composited cleanly over the scribble.
- Confirm Crayon entries place the hand-drawn icon at the left and the label at
  the right without rectangular tile boundaries.
- Confirm Crayon submenus use their own paper/header/list composition instead
  of recoloring the Synthwave submenu.
- Confirm Crayon submenus show one large title, no duplicated `PICOPEN` header,
  no equals-sign separator, no square brackets, and no question-mark glyphs in
  place of controls.

## High Contrast and Minimal Dark

- Confirm all text and focus states remain visible and no decorative texture
  reduces contrast.

## Shared behavior

- Confirm the active skin applies to every submenu and confirmation dialog.
- Confirm Advanced Terminal remains plain and readable.
- Confirm Home and System remember their previous selections.
- Confirm switching skins remains session-only and a cold boot restores
  Synthwave.
- Confirm only affected Home tiles redraw during navigation.
- In every list menu, confirm only the previous and newly selected rows redraw;
  the header, background, other rows, and footer must remain stable.
