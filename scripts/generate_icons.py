"""
Generates .ico files from .svg sources in the resources/ folder.
Uses Pillow to draw icons programmatically based on SVG geometry.

Usage:
    python scripts/generate_icons.py
"""

import os
import sys
from PIL import Image, ImageDraw

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(SCRIPT_DIR)
RESOURCES_DIR = os.path.join(REPO_ROOT, "resources")
SIZES = [16, 24, 32, 48, 64, 128, 256]


def draw_translator(size):
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    s = size / 64.0

    draw.rounded_rectangle(
        [int(4 * s), int(4 * s), int(60 * s), int(60 * s)],
        radius=int(10 * s),
        fill=(58, 42, 26),
        outline=(42, 26, 10),
        width=max(1, int(1.5 * s)),
    )

    draw.rounded_rectangle(
        [int(14 * s), int(12 * s), int(40 * s), int(52 * s)],
        radius=int(2 * s),
        fill=(245, 237, 216),
        outline=(139, 115, 85),
        width=max(1, int(s)),
    )

    for line_y, line_w in [(20, 18), (26, 16), (32, 18), (38, 12), (44, 16)]:
        draw.line(
            [int(18 * s), int(line_y * s), int((18 + line_w) * s), int(line_y * s)],
            fill=(107, 91, 74),
            width=max(1, int(1.5 * s)),
        )

    draw.line(
        [int(44 * s), int(26 * s), int(54 * s), int(26 * s)],
        fill=(212, 164, 74),
        width=max(1, int(2.5 * s)),
    )
    draw.line(
        [int(54 * s), int(36 * s), int(44 * s), int(36 * s)],
        fill=(212, 164, 74),
        width=max(1, int(2.5 * s)),
    )

    return img


def draw_editor(size):
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    s = size / 64.0

    draw.rounded_rectangle(
        [int(4 * s), int(4 * s), int(60 * s), int(60 * s)],
        radius=int(10 * s),
        fill=(26, 42, 58),
        outline=(10, 26, 42),
        width=max(1, int(1.5 * s)),
    )

    draw.rounded_rectangle(
        [int(12 * s), int(16 * s), int(30 * s), int(40 * s)],
        radius=int(2 * s),
        fill=(232, 224, 208),
        outline=(107, 91, 74),
        width=max(1, int(s)),
    )
    for line_y in [26, 30, 34]:
        draw.line(
            [int(15 * s), int(line_y * s), int(27 * s), int(line_y * s)],
            fill=(139, 123, 106),
            width=max(1, int(s)),
        )

    draw.rounded_rectangle(
        [int(34 * s), int(16 * s), int(52 * s), int(40 * s)],
        radius=int(2 * s),
        fill=(232, 224, 208),
        outline=(107, 91, 74),
        width=max(1, int(s)),
    )
    for line_y in [22, 26, 30, 34]:
        draw.line(
            [int(37 * s), int(line_y * s), int(49 * s), int(line_y * s)],
            fill=(139, 123, 106),
            width=max(1, int(s)),
        )

    draw.line(
        [int(21 * s), int(44 * s), int(32 * s), int(52 * s)],
        fill=(160, 184, 208),
        width=max(1, int(2.5 * s)),
    )
    draw.line(
        [int(43 * s), int(44 * s), int(32 * s), int(52 * s)],
        fill=(160, 184, 208),
        width=max(1, int(2.5 * s)),
    )

    return img


def generate_ico(draw_func, output_name):
    ico_path = os.path.join(RESOURCES_DIR, output_name)
    images = [draw_func(size) for size in SIZES]
    images[-1].save(
        ico_path,
        format="ICO",
        sizes=[(s, s) for s in SIZES],
        append_images=images[:-1],
    )
    print(f"Created {ico_path}")


def main():
    generate_ico(draw_translator, "yampt-translator.ico")
    generate_ico(draw_editor, "yampt-editor.ico")


if __name__ == "__main__":
    main()
