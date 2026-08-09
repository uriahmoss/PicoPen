# Crayon artwork pipeline

The built-in Crayon renderer uses authored raster artwork rather than deriving
its geometry from another skin. Source artwork is retained under
`assets/skins/crayon/` for review and regeneration.

`tools/generate_crayon_screen.py` performs an offline deterministic conversion:

1. resize the approved neutral source to the physical 320x320 panel;
2. quantize it to a fixed 64-color palette;
3. quantize and crop six separately authored selected-item states; and
4. emit `crayon_screen_asset.h` as checked-in firmware data.

The device does not parse PNG files and does not require Pillow. The runtime
display API validates pointers, dimensions, stride, palette size, and panel
bounds before emitting an indexed bitmap. Transparent drawing is limited to a
single explicit palette key.

The Home renderer owns the complete illustration and its damage rectangles.
Moving focus restores the previous and next regions from the neutral base and
then blits the selected item's authored crop. Home focus contains no procedural
line, rectangle, hatch, or runtime-generated scribble geometry, and navigation
does not refresh the whole panel.

The generated source artwork was created for PicoPen with OpenAI's built-in
image-generation tool. It contains no third-party logo or copied application
interface.
