import pathlib

from PIL import Image, ImageDraw, ImageFont


ROOT = pathlib.Path(__file__).resolve().parents[1]
FONT = ROOT / "third_party" / "fonts" / "kalam" / "Kalam-Regular.ttf"
OUTPUT = ROOT / "services" / "appearance" / "include" / "picopen" / "crayon_font.h"
FIRST = 32
LAST = 90
WIDTH = 8
HEIGHT = 13


def glyph_rows(font, character):
    image = Image.new("1", (WIDTH, HEIGHT), 0)
    draw = ImageDraw.Draw(image)
    bounds = draw.textbbox((0, 0), character, font=font)
    glyph_width = bounds[2] - bounds[0]
    x = max(0, (WIDTH - glyph_width) // 2 - bounds[0])
    y = -bounds[1]
    draw.text((x, y), character, font=font, fill=1)
    return [
        sum((1 << (WIDTH - 1 - x)) for x in range(WIDTH) if image.getpixel((x, y)))
        for y in range(HEIGHT)
    ]


def main():
    font = ImageFont.truetype(str(FONT), 13)
    lines = [
        "// Generated deterministically by tools/generate_crayon_font.py.",
        "#ifndef PICOPEN_CRAYON_FONT_H",
        "#define PICOPEN_CRAYON_FONT_H",
        "",
        "#include <stdint.h>",
        "",
        f"#define PICOPEN_CRAYON_FIRST {FIRST}u",
        f"#define PICOPEN_CRAYON_LAST {LAST}u",
        f"#define PICOPEN_CRAYON_WIDTH {WIDTH}u",
        f"#define PICOPEN_CRAYON_HEIGHT {HEIGHT}u",
        "",
        f"static const uint16_t picopen_crayon_glyphs[{LAST - FIRST + 1}][{HEIGHT}] = {{",
    ]
    for codepoint in range(FIRST, LAST + 1):
        rows = ", ".join(f"0x{row:03X}u" for row in glyph_rows(font, chr(codepoint)))
        lines.append(f"    /* {chr(codepoint)!r} */ {{{rows}}},")
    lines.extend(["};", "", "#endif", ""])
    OUTPUT.write_text("\n".join(lines), encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
