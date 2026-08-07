#!/usr/bin/env python3
"""Regenerate Core/Src/DisplayChars.c from the PNGs in this directory.

Expects one PNG per note, named after the C identifier suffix used in
DisplayChars.h/.c (A.png, ASharp.png, B.png, C.png, CSharp.png, D.png,
DSharp.png, E.png, F.png, FSharp.png, G.png, GSharp.png). Source PNGs
are tightly cropped to each glyph's own bounding box (naturals are
narrow/tall, sharps are wide/short) so each is scaled to fit inside a
DISPLAY_CHAR_W x DISPLAY_CHAR_H box, preserving aspect ratio, and
centered on a transparent canvas of that size before packing.

Opaque pixels (alpha >= WHITE_THRESHOLD) become set bits; transparent
pixels become 0, matching the "White pixels in the source PNG are 1;
transparent pixels are 0" convention documented in DisplayChars.h.

Requires Pillow: pip install pillow
"""
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    sys.exit("Pillow is required: pip install pillow")

DISPLAY_CHAR_W = 86
DISPLAY_CHAR_H = 42
DISPLAY_CHAR_STRIDE = 11  # bytes per row = ceil(86 / 8)

NOTE_NAMES = [
    "A", "ASharp", "B", "C", "CSharp", "D",
    "DSharp", "E", "F", "FSharp", "G", "GSharp",
]

WHITE_THRESHOLD = 128  # alpha cutoff for "this pixel is on"

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent  # HandymanFirmware_F4/
SOURCE_PATH = REPO_ROOT / "Core" / "Src" / "DisplayChars.c"


def fit_to_canvas(img: Image.Image) -> Image.Image:
    """Scale img to fit within DISPLAY_CHAR_W x DISPLAY_CHAR_H (aspect preserved)
    and center it on a transparent canvas of exactly that size."""
    img = img.convert("RGBA")
    scale = min(DISPLAY_CHAR_W / img.width, DISPLAY_CHAR_H / img.height)
    new_w = max(1, round(img.width * scale))
    new_h = max(1, round(img.height * scale))
    resized = img.resize((new_w, new_h), Image.LANCZOS)

    canvas = Image.new("RGBA", (DISPLAY_CHAR_W, DISPLAY_CHAR_H), (0, 0, 0, 0))
    offset = ((DISPLAY_CHAR_W - new_w) // 2, (DISPLAY_CHAR_H - new_h) // 2)
    canvas.alpha_composite(resized, offset)
    return canvas


def pack_bitmap(img: Image.Image) -> bytes:
    img = fit_to_canvas(img)
    out = bytearray(DISPLAY_CHAR_STRIDE * DISPLAY_CHAR_H)

    for y in range(DISPLAY_CHAR_H):
        for x in range(DISPLAY_CHAR_W):
            _, _, _, a = img.getpixel((x, y))
            if a >= WHITE_THRESHOLD:
                byte_idx = y * DISPLAY_CHAR_STRIDE + x // 8
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
        data = pack_bitmap(Image.open(png_path))
        arrays.append(format_c_array(name, data))
        print(f"packed {png_path.name} -> DisplayChar_{name} ({len(data)} bytes)")

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
