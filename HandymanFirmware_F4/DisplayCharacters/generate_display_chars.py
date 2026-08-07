#!/usr/bin/env python3
"""Regenerate Core/Src/DisplayChars.c from the PNGs in this directory.

Expects two PNGs per note, named after the C identifier suffix used in
DisplayChars.h/.c:
  - horizontal: A.png, ASharp.png, B.png, ... GSharp.png
  - vertical:   A_vertical.png, ASharp_vertical.png, ... GSharp_vertical.png
Source PNGs are tightly cropped to each glyph's own bounding box, so each
is scaled to fit inside its target canvas (DISPLAY_CHAR_W/H_HORIZONTAL for
the plain names, DISPLAY_CHAR_W/H_VERTICAL for the "_vertical" ones),
preserving aspect ratio, and centered on a transparent canvas of that size
before packing. Vertical PNGs are packed into arrays named with a
"_Vertical" suffix (DisplayChar_A_Vertical, etc).

Opaque pixels (alpha >= WHITE_THRESHOLD) become set bits; transparent
pixels become 0, matching the "White pixels in the source PNG are 1;
transparent pixels are 0" convention documented in DisplayChars.h.

Requires Pillow: pip install pillow
"""
import math
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    sys.exit("Pillow is required: pip install pillow")

DISPLAY_CHAR_W_HORIZONTAL = 86
DISPLAY_CHAR_H_HORIZONTAL = 42
DISPLAY_CHAR_STRIDE_HORIZONTAL = 11  # bytes per row = ceil(86 / 8)

DISPLAY_CHAR_W_VERTICAL = 49
DISPLAY_CHAR_H_VERTICAL = 42
DISPLAY_CHAR_STRIDE_VERTICAL = math.ceil(DISPLAY_CHAR_W_VERTICAL / 8)  # 7

NOTE_NAMES = [
    "A", "ASharp", "B", "C", "CSharp", "D",
    "DSharp", "E", "F", "FSharp", "G", "GSharp",
]

WHITE_THRESHOLD = 128  # alpha cutoff for "this pixel is on"

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent  # HandymanFirmware_F4/
SOURCE_PATH = REPO_ROOT / "Core" / "Src" / "DisplayChars.c"


def fit_to_canvas(img: Image.Image, canvas_w: int, canvas_h: int) -> Image.Image:
    """Scale img to fit within canvas_w x canvas_h (aspect preserved) and
    center it on a transparent canvas of exactly that size."""
    img = img.convert("RGBA")
    scale = min(canvas_w / img.width, canvas_h / img.height)
    new_w = max(1, round(img.width * scale))
    new_h = max(1, round(img.height * scale))
    resized = img.resize((new_w, new_h), Image.LANCZOS)

    canvas = Image.new("RGBA", (canvas_w, canvas_h), (0, 0, 0, 0))
    offset = ((canvas_w - new_w) // 2, (canvas_h - new_h) // 2)
    canvas.alpha_composite(resized, offset)
    return canvas


def pack_bitmap(img: Image.Image, w: int, h: int, stride: int) -> bytes:
    img = fit_to_canvas(img, w, h)
    out = bytearray(stride * h)

    for y in range(h):
        for x in range(w):
            _, _, _, a = img.getpixel((x, y))
            if a >= WHITE_THRESHOLD:
                byte_idx = y * stride + x // 8
                out[byte_idx] |= 0x80 >> (x % 8)

    return bytes(out)


def format_c_array(name: str, data: bytes) -> str:
    lines = [f"const uint8_t DisplayChar_{name}[{len(data)}] = {{"]
    for i in range(0, len(data), 16):
        chunk = data[i:i + 16]
        lines.append("    " + ", ".join(f"0x{b:02X}" for b in chunk) + ",")
    lines.append("};")
    return "\n".join(lines)


def main() -> int:
    arrays = []
    missing = []

    for name in NOTE_NAMES:
        png_path = SCRIPT_DIR / f"{name}.png"
        if not png_path.exists():
            missing.append(png_path.name)
            continue
        data = pack_bitmap(
            Image.open(png_path),
            DISPLAY_CHAR_W_HORIZONTAL, DISPLAY_CHAR_H_HORIZONTAL, DISPLAY_CHAR_STRIDE_HORIZONTAL,
        )
        arrays.append(format_c_array(name, data))
        print(f"packed {png_path.name} -> DisplayChar_{name} ({len(data)} bytes)")

    for name in NOTE_NAMES:
        png_path = SCRIPT_DIR / f"{name}_vertical.png"
        if not png_path.exists():
            missing.append(png_path.name)
            continue
        data = pack_bitmap(
            Image.open(png_path),
            DISPLAY_CHAR_W_VERTICAL, DISPLAY_CHAR_H_VERTICAL, DISPLAY_CHAR_STRIDE_VERTICAL,
        )
        vert_name = f"{name}_Vertical"
        arrays.append(format_c_array(vert_name, data))
        print(f"packed {png_path.name} -> DisplayChar_{vert_name} ({len(data)} bytes)")

    if missing:
        print(
            "Missing PNGs, aborting without touching DisplayChars.c: "
            + ", ".join(missing),
            file=sys.stderr,
        )
        return 1

    body = '#include "DisplayChars.h"\n\n\n' + "\n\n".join(arrays) + "\n"
    SOURCE_PATH.write_text(body)
    print(f"wrote {SOURCE_PATH}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
